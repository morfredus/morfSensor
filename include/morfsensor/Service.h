/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QObject>
#include <QStringList>
#include "morfsensor/ServiceConfig.h"

namespace morfbeacon { class Heartbeat; }

namespace morfsensor {

class ModuleRegistry;
class HttpServer;

// -----------------------------------------------------------------------------
// Service : facade qui cable tout le service a partir d'une ServiceConfig.
//
//   config JSON
//     -> capteurs (ModuleFactory) -> ModuleRegistry
//     -> HttpServer (API /presence, /sensors, /status)
//     -> morfbeacon::Heartbeat (annonce de presence sur le LAN)
//
// C'est le seul objet que le demon (service/main.cpp) manipule :
//
//   morfsensor::Service service(config);
//   if (!service.start()) { ... }
//   return app.exec();
// -----------------------------------------------------------------------------
class Service : public QObject {
    Q_OBJECT
public:
    explicit Service(ServiceConfig config, QObject* parent = nullptr);
    ~Service() override;

    // Demarre les capteurs, le serveur HTTP puis le heartbeat. Renvoie true si
    // le serveur HTTP ecoute (toujours true si httpPort == 0).
    bool start();
    void stop();

    int              sensorCount() const;
    quint16          httpPort() const;      // port HTTP reellement ouvert (0 si arret)
    QStringList      warnings() const;       // erreurs de construction des capteurs

    ModuleRegistry*   registry() const;
    HttpServer* httpServer() const;

private:
    ServiceConfig         m_config;
    ModuleRegistry*      m_registry;
    HttpServer*    m_http;
    morfbeacon::Heartbeat* m_heartbeat = nullptr;
    QStringList          m_warnings;
};

} // namespace morfsensor
