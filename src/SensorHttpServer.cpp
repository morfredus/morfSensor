/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "morfsensor/SensorHttpServer.h"
#include "morfsensor/SensorRegistry.h"
#include "morfsensor/Version.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QHostInfo>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QUrl>

#include <utility>

namespace morfsensor {

namespace {
constexpr int kMaxRequestBytes = 8192; // garde-fou : une requete GET est minuscule

QByteArray toJson(const QJsonObject& o) {
    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}
} // namespace

SensorHttpServer::SensorHttpServer(SensorConfig config, SensorRegistry* registry, QObject* parent)
    : QObject(parent),
      m_config(std::move(config)),
      m_registry(registry),
      m_server(new QTcpServer(this)) {
    connect(m_server, &QTcpServer::newConnection, this, &SensorHttpServer::onNewConnection);
}

SensorHttpServer::~SensorHttpServer() = default;

bool SensorHttpServer::start() {
    if (m_config.httpPort == 0)
        return false;

    m_uptime.start();

    QHostAddress addr(m_config.bindAddress);
    if (addr.isNull())
        addr = QHostAddress(QHostAddress::AnyIPv4);

    return m_server->listen(addr, m_config.httpPort);
}

void SensorHttpServer::stop() {
    m_server->close();
}

bool SensorHttpServer::isListening() const {
    return m_server->isListening();
}

quint16 SensorHttpServer::port() const {
    return m_server->isListening() ? m_server->serverPort() : 0;
}

void SensorHttpServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket* sock = m_server->nextPendingConnection();

        connect(sock, &QTcpSocket::readyRead, this, [this, sock]() {
            QByteArray buf = sock->property("buf").toByteArray();
            buf += sock->readAll();

            const int headerEnd = buf.indexOf("\r\n\r\n");
            if (headerEnd < 0) {
                if (buf.size() > kMaxRequestBytes) {
                    sock->abort();
                    return;
                }
                sock->setProperty("buf", buf); // en-tetes incomplets : on attend la suite
                return;
            }

            const int lineEnd = buf.indexOf("\r\n");
            const QByteArray requestLine = buf.left(lineEnd);
            handleRequest(sock, requestLine);
        });

        connect(sock, &QTcpSocket::disconnected, sock, &QObject::deleteLater);
    }
}

void SensorHttpServer::handleRequest(QTcpSocket* sock, const QByteArray& requestLine) {
    const QList<QByteArray> parts = requestLine.split(' ');
    const QByteArray method = parts.value(0);
    const QByteArray path   = parts.value(1);

    int        code   = 200;
    QByteArray reason = "OK";
    QByteArray body;

    if (method != "GET") {
        code = 405; reason = "Method Not Allowed";
        body = "{\"error\":\"method not allowed\"}";
    } else {
        body = routeBody(path, code, reason);
    }

    QByteArray resp;
    resp += "HTTP/1.1 " + QByteArray::number(code) + " " + reason + "\r\n";
    resp += "Content-Type: application/json; charset=utf-8\r\n";
    resp += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    resp += "Access-Control-Allow-Origin: *\r\n"; // autorise un futur dashboard web
    resp += "Connection: close\r\n";
    resp += "\r\n";
    resp += body;

    sock->write(resp);
    sock->flush();
    sock->disconnectFromHost();
}

QByteArray SensorHttpServer::routeBody(const QByteArray& rawPath, int& code, QByteArray& reason) const {
    // Isole le chemin de l'eventuelle chaine de requete (?...).
    const QByteArray path = rawPath.left(rawPath.indexOf('?') < 0 ? rawPath.size()
                                                                  : rawPath.indexOf('?'));

    if (path == "/healthz")
        return "{\"status\":\"ok\"}";

    if (path == "/status")
        return buildStatusJson();

    if (path == "/presence")
        return toJson(m_registry ? m_registry->presenceJson() : QJsonObject{});

    if (path == "/sensors") {
        QJsonObject o;
        o["sensors"] = m_registry ? m_registry->sensorsJson() : QJsonArray{};
        o["count"]   = m_registry ? m_registry->count() : 0;
        o["ts"]      = static_cast<double>(QDateTime::currentSecsSinceEpoch());
        return toJson(o);
    }

    if (path.startsWith("/sensors/")) {
        const QString id = QUrl::fromPercentEncoding(path.mid(9));
        if (m_registry) {
            bool found = false;
            const QJsonObject o = m_registry->sensorJson(id, &found);
            if (found)
                return toJson(o);
        }
        code = 404; reason = "Not Found";
        return "{\"error\":\"sensor not found\"}";
    }

    code = 404; reason = "Not Found";
    return "{\"error\":\"not found\"}";
}

QByteArray SensorHttpServer::buildStatusJson() const {
    // Format compatible morfBeacon : les outils du parc (beacon_status.py)
    // savent deja lire app/host/version/state/uptime_s/metrics/ts.
    QJsonObject o;
    o["app"]      = m_config.appName;
    o["host"]     = QHostInfo::localHostName();
    o["version"]  = morfsensor::version();
    o["proto"]    = QString::fromLatin1(morfsensor::kProtocol);
    o["state"]    = m_registry ? m_registry->state() : QStringLiteral("ok");
    o["uptime_s"] = static_cast<double>(m_uptime.isValid() ? m_uptime.elapsed() / 1000 : 0);
    o["ts"]       = static_cast<double>(QDateTime::currentSecsSinceEpoch());
    o["metrics"]  = m_registry ? m_registry->metrics() : QJsonObject{};

    // Etat de l'annonce LAN, rapporte par morfSensor lui-meme (morfBeacon est
    // embarque, il n'y a pas de service externe a interroger : le heartbeat vit
    // DANS ce processus). Portable : un superviseur lit ce champ de la meme
    // facon sous Linux et Windows.
    //   enabled : l'annonce est-elle demandee dans la config ?
    //   active  : est-elle reellement emise ? (= enabled ici)
    QJsonObject beacon;
    beacon["enabled"] = m_config.beaconEnabled;
    beacon["active"]  = m_config.beaconEnabled;
    if (m_config.beaconEnabled)
        beacon["udp_port"] = static_cast<int>(m_config.beaconUdpPort);
    o["beacon"] = beacon;

    return toJson(o);
}

} // namespace morfsensor
