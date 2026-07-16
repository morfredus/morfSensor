/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "morfsensor/Ld2410Sensor.h"

#include <QSerialPort>
#include <QTimer>
#include <QJsonObject>
#include <QDateTime>

namespace morfsensor {

namespace {

// Entete et pied des trames de rapport du LD2410 (mode "target data").
const QByteArray kFrameHeader = QByteArray::fromHex("F4F3F2F1");
const QByteArray kFrameFooter = QByteArray::fromHex("F8F7F6F5");

constexpr int kMaxBufferBytes = 4096;   // garde-fou anti-emballement du tampon
constexpr int kReconnectMs    = 3000;   // periode de reessai d'ouverture du port

inline quint8 u8(char c) { return static_cast<quint8>(c); }
inline int    le16(const QByteArray& p, int i) {
    return u8(p[i]) | (u8(p[i + 1]) << 8);
}

// Libelle lisible de l'etat de cible du LD2410.
QString targetLabel(quint8 state) {
    switch (state) {
    case 0x00: return QStringLiteral("none");
    case 0x01: return QStringLiteral("moving");
    case 0x02: return QStringLiteral("static");
    case 0x03: return QStringLiteral("moving+static");
    default:   return QStringLiteral("unknown");
    }
}

} // namespace

Ld2410Sensor::Ld2410Sensor(const QString& id, const QString& portName, int baud,
                           int presenceHoldMs, int staleMs, QObject* parent)
    : ISensor(id, QStringLiteral("presence"), parent),
      m_portName(portName),
      m_baud(baud > 0 ? baud : 256000),
      m_presenceHoldMs(presenceHoldMs >= 0 ? presenceHoldMs : 2000),
      m_staleMs(staleMs > 0 ? staleMs : 3000),
      m_port(new QSerialPort(this)),
      m_reconnectTimer(new QTimer(this)),
      m_staleTimer(new QTimer(this)) {

    m_reconnectTimer->setInterval(kReconnectMs);
    connect(m_reconnectTimer, &QTimer::timeout, this, &Ld2410Sensor::onReconnectTick);

    m_staleTimer->setInterval(m_staleMs);
    m_staleTimer->setSingleShot(true);
    connect(m_staleTimer, &QTimer::timeout, this, &Ld2410Sensor::onStaleTick);

    connect(m_port, &QSerialPort::readyRead, this, &Ld2410Sensor::onReadyRead);
    connect(m_port, &QSerialPort::errorOccurred, this,
            [this](QSerialPort::SerialPortError err) {
                if (err == QSerialPort::NoError)
                    return;
                // Erreur materielle (capteur debranche, port disparu) :
                // on ferme et on bascule en reessai periodique.
                m_port->close();
                setUnavailable(QStringLiteral("error"),
                               QStringLiteral("serial error: ") + m_port->errorString());
                if (!m_reconnectTimer->isActive())
                    m_reconnectTimer->start();
            });

    // Instantane initial : capteur declare mais pas encore de trame.
    m_last = makeReading();
    m_last.available = false;
    m_last.state     = QStringLiteral("starting");
    QJsonObject v;
    v["present"] = false;
    m_last.values = v;
}

Ld2410Sensor::~Ld2410Sensor() = default;

bool Ld2410Sensor::start() {
    if (!openPort()) {
        // Le port n'est pas la maintenant : on reessaiera en tache de fond.
        setUnavailable(QStringLiteral("error"),
                       QStringLiteral("cannot open ") + m_portName);
        m_reconnectTimer->start();
    }
    return true;   // le service continue : le capteur peut arriver plus tard.
}

void Ld2410Sensor::stop() {
    m_reconnectTimer->stop();
    m_staleTimer->stop();
    if (m_port->isOpen())
        m_port->close();
}

SensorReading Ld2410Sensor::lastReading() const {
    return m_last;
}

bool Ld2410Sensor::openPort() {
    if (m_port->isOpen())
        return true;

    m_port->setPortName(m_portName);
    m_port->setBaudRate(m_baud);
    m_port->setDataBits(QSerialPort::Data8);
    m_port->setParity(QSerialPort::NoParity);
    m_port->setStopBits(QSerialPort::OneStop);
    m_port->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_port->open(QIODevice::ReadOnly))
        return false;

    m_buffer.clear();
    m_reconnectTimer->stop();
    m_staleTimer->start();     // arme la surveillance de fraicheur
    return true;
}

void Ld2410Sensor::onReconnectTick() {
    if (openPort()) {
        // Reste indisponible jusqu'a la 1re trame valide, mais on note l'avancee.
        setUnavailable(QStringLiteral("warning"),
                       QStringLiteral("port ouvert, en attente de trame"));
    }
}

void Ld2410Sensor::onReadyRead() {
    m_buffer += m_port->readAll();
    if (m_buffer.size() > kMaxBufferBytes)               // anti-emballement
        m_buffer = m_buffer.right(kMaxBufferBytes);
    parseBuffer();
}

void Ld2410Sensor::parseBuffer() {
    for (;;) {
        const int start = m_buffer.indexOf(kFrameHeader);
        if (start < 0) {
            // Pas d'entete complet : on ne garde qu'une eventuelle entete partielle.
            if (m_buffer.size() > 3)
                m_buffer = m_buffer.right(3);
            return;
        }
        if (start > 0)
            m_buffer.remove(0, start);                   // resync sur l'entete

        // Il faut l'entete (4) + la longueur (2) pour connaitre la taille.
        if (m_buffer.size() < 6)
            return;

        const int len   = le16(m_buffer, 4);
        const int total = 4 + 2 + len + 4;               // + pied de trame
        if (len <= 0 || total > kMaxBufferBytes) {
            m_buffer.remove(0, 1);                        // longueur absurde : on glisse
            continue;
        }
        if (m_buffer.size() < total)
            return;                                       // trame incomplete : on attend

        const QByteArray payload = m_buffer.mid(6, len);
        const QByteArray footer  = m_buffer.mid(6 + len, 4);
        if (footer != kFrameFooter) {
            m_buffer.remove(0, 1);                        // faux positif : on glisse
            continue;
        }

        applyFrame(payload);
        m_buffer.remove(0, total);
    }
}

void Ld2410Sensor::applyFrame(const QByteArray& payload) {
    // Trames "basic" (0x02) et "engineering" (0x01) partagent le meme debut :
    //   type(1) 0xAA state(1) movDist(2) movEner(1) staDist(2) staEner(1) detDist(2)
    if (payload.size() < 13)
        return;
    const quint8 type = u8(payload[0]);
    if ((type != 0x02 && type != 0x01) || u8(payload[1]) != 0xAA)
        return;

    const quint8 state        = u8(payload[2]);
    const int    movingCm     = le16(payload, 3);
    const quint8 movingEnergy = u8(payload[5]);
    const int    staticCm     = le16(payload, 6);
    const quint8 staticEnergy = u8(payload[8]);
    const int    detectCm     = le16(payload, 9);

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const bool rawPresent = (state != 0x00);
    if (rawPresent)
        m_lastPresenceMs = now;

    // Lissage : on maintient present=true un court instant apres la derniere
    // detection, pour eviter le clignotement en limite de portee.
    const bool present = rawPresent
        || (m_presenceHoldMs > 0 && m_lastPresenceMs > 0
            && (now - m_lastPresenceMs) < m_presenceHoldMs);

    m_last = makeReading();
    m_last.available = true;
    m_last.state     = QStringLiteral("ok");
    QJsonObject v;
    v["present"]       = present;
    v["target_state"]  = static_cast<int>(state);
    v["target_label"]  = targetLabel(state);
    v["moving_cm"]     = movingCm;
    v["moving_energy"] = static_cast<int>(movingEnergy);
    v["static_cm"]     = staticCm;
    v["static_energy"] = static_cast<int>(staticEnergy);
    v["detect_cm"]     = detectCm;
    m_last.values = v;

    m_staleTimer->start();     // trame fraiche : on rearme le chien de garde
    emit readingUpdated(m_last);
}

void Ld2410Sensor::onStaleTick() {
    // Plus de trame depuis m_staleMs alors que le port est ouvert : le flux
    // s'est tari (cable coupe, capteur muet). On le declare indisponible.
    setUnavailable(QStringLiteral("warning"),
                   QStringLiteral("aucune trame depuis ") + QString::number(m_staleMs) + " ms");
}

void Ld2410Sensor::setUnavailable(const QString& state, const QString& reason) {
    m_last = makeReading();
    m_last.available = false;
    m_last.state     = state;
    QJsonObject v;
    v["present"] = false;
    v["error"]   = reason;
    m_last.values = v;
    emit readingUpdated(m_last);
}

} // namespace morfsensor
