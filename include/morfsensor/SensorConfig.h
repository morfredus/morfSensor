/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QtGlobal>

namespace morfsensor {

// -----------------------------------------------------------------------------
// SensorDef : declaration d'UN capteur a activer (lue depuis la config JSON).
//
//   type   : identifiant de fabrique ("ld2410", "mock", ...) -> SensorFactory
//   id     : identifiant unique dans le service ("presence-salon")
//   params : objet JSON libre, propre au type (port, baud, seuils, periode...)
// -----------------------------------------------------------------------------
struct SensorDef {
    QString     type;
    QString     id;
    QJsonObject params;   // reprend l'objet JSON complet du capteur
};

// -----------------------------------------------------------------------------
// SensorConfig : configuration complete du service.
//
// Charge depuis un fichier JSON (voir config/morfsensor.example.json). Definit
// le serveur HTTP local, l'annonce reseau morfBeacon, et la liste des capteurs.
// -----------------------------------------------------------------------------
struct SensorConfig {
    // Identite annoncee (heartbeat morfBeacon + /status).
    QString appName    = QStringLiteral("morfSensor");
    QString instanceId;                              // defaut = appName@hostname

    // Serveur HTTP local (API des capteurs).
    quint16 httpPort    = 8788;                      // 0 => pas de serveur HTTP
    QString bindAddress = QStringLiteral("0.0.0.0"); // interfaces ecoutees

    // Annonce de presence sur le LAN via morfBeacon.
    bool    beaconEnabled    = true;
    quint16 beaconUdpPort    = 45454;                // port du parc (cf. dashboard)
    int     beaconIntervalMs = 15000;

    // Capteurs a activer.
    QVector<SensorDef> sensors;

    // Construit une config depuis un objet JSON deja parse.
    static SensorConfig fromJson(const QJsonObject& root) {
        SensorConfig c;
        if (root.contains("app_name"))     c.appName    = root.value("app_name").toString(c.appName);
        if (root.contains("instance_id"))  c.instanceId = root.value("instance_id").toString();
        if (root.contains("http_port"))    c.httpPort   = static_cast<quint16>(root.value("http_port").toInt(c.httpPort));
        if (root.contains("bind_address")) c.bindAddress = root.value("bind_address").toString(c.bindAddress);

        const QJsonObject beacon = root.value("beacon").toObject();
        if (beacon.contains("enabled"))     c.beaconEnabled    = beacon.value("enabled").toBool(c.beaconEnabled);
        if (beacon.contains("udp_port"))    c.beaconUdpPort    = static_cast<quint16>(beacon.value("udp_port").toInt(c.beaconUdpPort));
        if (beacon.contains("interval_ms")) c.beaconIntervalMs = beacon.value("interval_ms").toInt(c.beaconIntervalMs);

        const QJsonArray sensors = root.value("sensors").toArray();
        for (const QJsonValue& v : sensors) {
            const QJsonObject o = v.toObject();
            const QString type = o.value("type").toString();
            if (type.isEmpty())
                continue;                            // entree invalide : ignoree
            SensorDef d;
            d.type   = type;
            d.id     = o.value("id").toString(type); // id par defaut = type
            d.params = o;
            c.sensors.push_back(d);
        }
        return c;
    }
};

} // namespace morfsensor
