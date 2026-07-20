/*
 * morfSensor — exemple de demonstration
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Demarre le service avec un unique capteur SIMULE (aucun materiel requis),
 * puis expose l'API HTTP. A tester avec :
 *
 *   curl http://localhost:8788/presence
 *   curl http://localhost:8788/sensors
 *   curl http://localhost:8788/status
 *
 * Le capteur simule fait basculer "present" toutes les 3 secondes.
 */

#include <QCoreApplication>

#include <morfsensor/Service.h>
#include <morfsensor/ServiceConfig.h>
#include <morfsensor/Version.h>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    morfsensor::ServiceConfig cfg;
    cfg.httpPort         = 8788;
    cfg.beaconIntervalMs = 5000;   // annonce rapide pour une demo reactive

    morfsensor::ModuleDef mock;
    mock.type   = QStringLiteral("mock");
    mock.id     = QStringLiteral("presence-demo");
    mock.params = QJsonObject{ {"type", "mock"}, {"period_ms", 3000} };
    cfg.sensors.push_back(mock);

    morfsensor::Service service(cfg);
    if (!service.start()) {
        qWarning("API HTTP non demarree (port %u occupe ?)", cfg.httpPort);
        return 1;
    }

    qInfo("morfSensor demo v%s : %d capteur(s) ; GET http://localhost:%u/presence",
          qUtf8Printable(morfsensor::version()), service.sensorCount(), service.httpPort());

    return app.exec();
}
