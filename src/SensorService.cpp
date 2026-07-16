/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "morfsensor/SensorService.h"
#include "morfsensor/SensorRegistry.h"
#include "morfsensor/SensorHttpServer.h"
#include "morfsensor/SensorFactory.h"
#include "morfsensor/ISensor.h"
#include "morfsensor/Version.h"

#ifdef MORFSENSOR_HAVE_MORFBEACON
#  include "morfbeacon/Heartbeat.h"
#  include "morfbeacon/PresenceConfig.h"
#else
#  include <QLoggingCategory>
#endif

#include <utility>

namespace morfsensor {

SensorService::SensorService(SensorConfig config, QObject* parent)
    : QObject(parent),
      m_config(std::move(config)),
      m_registry(new SensorRegistry(this)),
      m_http(nullptr) {

    // Construit les capteurs declares. Une erreur sur un capteur (type inconnu,
    // parametre manquant) n'empeche pas les autres de fonctionner : on la note.
    for (const SensorDef& def : m_config.sensors) {
        QString error;
        ISensor* sensor = SensorFactory::create(def, &error);
        if (!sensor) {
            m_warnings << error;
            continue;
        }
        m_registry->add(sensor);
    }

    m_http = new SensorHttpServer(m_config, m_registry, this);
}

SensorService::~SensorService() = default;

bool SensorService::start() {
    m_registry->startAll();

    const bool httpOk = (m_config.httpPort == 0) ? true : m_http->start();

#ifdef MORFSENSOR_HAVE_MORFBEACON
    if (m_config.beaconEnabled) {
        morfbeacon::PresenceConfig pc;
        pc.appName             = m_config.appName;
        pc.version             = morfsensor::version();
        pc.instanceId          = m_config.instanceId;
        pc.udpPort             = m_config.beaconUdpPort;
        pc.broadcastIntervalMs = m_config.beaconIntervalMs;
        // Annonce le port HTTP reellement ouvert (0 si le serveur n'a pas demarre).
        pc.statusPort          = m_http ? m_http->port() : 0;
        pc.statusBindAddress   = m_config.bindAddress;

        m_heartbeat = new morfbeacon::Heartbeat(pc, m_registry, this);
        m_heartbeat->start();
    }
#else
    // Compile sans morfBeacon : pas d'annonce LAN. Le service reste pleinement
    // utilisable via son API HTTP ; on le signale une fois au demarrage.
    if (m_config.beaconEnabled)
        qWarning("morfSensor compile sans morfBeacon : annonce LAN desactivee "
                 "(API HTTP active).");
#endif

    return httpOk;
}

void SensorService::stop() {
#ifdef MORFSENSOR_HAVE_MORFBEACON
    if (m_heartbeat)
        m_heartbeat->stop();
#endif
    if (m_http)
        m_http->stop();
    m_registry->stopAll();
}

int SensorService::sensorCount() const {
    return m_registry->count();
}

quint16 SensorService::httpPort() const {
    return m_http ? m_http->port() : 0;
}

QStringList SensorService::warnings() const {
    return m_warnings;
}

SensorRegistry* SensorService::registry() const {
    return m_registry;
}

SensorHttpServer* SensorService::httpServer() const {
    return m_http;
}

} // namespace morfsensor
