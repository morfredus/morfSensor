/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "morfsensor/Service.h"
#include "morfsensor/ModuleRegistry.h"
#include "morfsensor/HttpServer.h"
#include "morfsensor/ModuleFactory.h"
#include "morfsensor/ISensor.h"
#include "morfsensor/Version.h"

#include "morfbeacon/Heartbeat.h"
#include "morfbeacon/PresenceConfig.h"

#include <utility>

namespace morfsensor {

Service::Service(ServiceConfig config, QObject* parent)
    : QObject(parent),
      m_config(std::move(config)),
      m_registry(new ModuleRegistry(this)),
      m_http(nullptr) {

    // Construit les capteurs declares. Une erreur sur un capteur (type inconnu,
    // parametre manquant) n'empeche pas les autres de fonctionner : on la note.
    for (const ModuleDef& def : m_config.sensors) {
        QString error;
        ISensor* sensor = ModuleFactory::create(def, &error);
        if (!sensor) {
            m_warnings << error;
            continue;
        }
        m_registry->add(sensor);
    }

    m_http = new HttpServer(m_config, m_registry, this);
}

Service::~Service() = default;

bool Service::start() {
    m_registry->startAll();

    const bool httpOk = (m_config.httpPort == 0) ? true : m_http->start();

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

    return httpOk;
}

void Service::stop() {
    if (m_heartbeat)
        m_heartbeat->stop();
    if (m_http)
        m_http->stop();
    m_registry->stopAll();
}

int Service::sensorCount() const {
    return m_registry->count();
}

quint16 Service::httpPort() const {
    return m_http ? m_http->port() : 0;
}

QStringList Service::warnings() const {
    return m_warnings;
}

ModuleRegistry* Service::registry() const {
    return m_registry;
}

HttpServer* Service::httpServer() const {
    return m_http;
}

} // namespace morfsensor
