/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QObject>
#include <QStringList>
#include "morfsensor/SensorConfig.h"

namespace morfbeacon { class Heartbeat; }

namespace morfsensor {

class SensorRegistry;
class SensorHttpServer;

// -----------------------------------------------------------------------------
// SensorService : facade qui cable tout le service a partir d'une SensorConfig.
//
//   config JSON
//     -> capteurs (SensorFactory) -> SensorRegistry
//     -> SensorHttpServer (API /presence, /sensors, /status)
//     -> morfbeacon::Heartbeat (annonce de presence sur le LAN)
//
// C'est le seul objet que le demon (service/main.cpp) manipule :
//
//   morfsensor::SensorService service(config);
//   if (!service.start()) { ... }
//   return app.exec();
// -----------------------------------------------------------------------------
class SensorService : public QObject {
    Q_OBJECT
public:
    explicit SensorService(SensorConfig config, QObject* parent = nullptr);
    ~SensorService() override;

    // Demarre les capteurs, le serveur HTTP puis le heartbeat. Renvoie true si
    // le serveur HTTP ecoute (toujours true si httpPort == 0).
    bool start();
    void stop();

    int              sensorCount() const;
    quint16          httpPort() const;      // port HTTP reellement ouvert (0 si arret)
    QStringList      warnings() const;       // erreurs de construction des capteurs

    SensorRegistry*   registry() const;
    SensorHttpServer* httpServer() const;

private:
    SensorConfig         m_config;
    SensorRegistry*      m_registry;
    SensorHttpServer*    m_http;
    morfbeacon::Heartbeat* m_heartbeat = nullptr;
    QStringList          m_warnings;
};

} // namespace morfsensor
