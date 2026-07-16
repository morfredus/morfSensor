/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QObject>
#include <QElapsedTimer>
#include "morfsensor/SensorConfig.h"

class QTcpServer;
class QTcpSocket;

namespace morfsensor {

class SensorRegistry;

// -----------------------------------------------------------------------------
// SensorHttpServer : serveur HTTP/1.1 local exposant l'etat des capteurs.
//
// Calque sur le StatusServer de morfBeacon (parsing minimal, une requete / une
// reponse, connexion fermee), mais avec des routes plus riches :
//
//   GET /presence      -> { present, sources[], ts }   (interroge par le dashboard)
//   GET /sensors       -> { sensors[], count, ts }
//   GET /sensors/{id}  -> lecture d'un capteur (404 si inconnu)
//   GET /status        -> compatible morfBeacon (app, host, version, state,
//                          uptime_s, metrics, ts) : les outils du parc marchent
//   GET /healthz       -> { status: "ok" }
//
// Interroge a la demande par un superviseur (RaspberryDashboard), jamais en flux.
// -----------------------------------------------------------------------------
class SensorHttpServer : public QObject {
    Q_OBJECT
public:
    SensorHttpServer(SensorConfig config, SensorRegistry* registry,
                     QObject* parent = nullptr);
    ~SensorHttpServer() override;

    bool start();            // false si le port ne peut etre ouvert
    void stop();
    bool isListening() const;
    quint16 port() const;    // port reellement ecoute (0 si arrete)

private:
    void onNewConnection();
    void handleRequest(QTcpSocket* sock, const QByteArray& requestLine);
    QByteArray routeBody(const QByteArray& path, int& code, QByteArray& reason) const;
    QByteArray buildStatusJson() const;

    SensorConfig    m_config;
    SensorRegistry* m_registry;
    QTcpServer*     m_server;
    QElapsedTimer   m_uptime;
};

} // namespace morfsensor
