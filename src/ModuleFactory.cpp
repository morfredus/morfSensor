/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "morfsensor/ModuleFactory.h"
#include "morfsensor/ISensor.h"
#include "morfsensor/MockSensor.h"
#ifdef MORFSENSOR_HAVE_SERIALPORT
#  include "morfsensor/Ld2410Sensor.h"
#endif

namespace morfsensor {
namespace ModuleFactory {

// -----------------------------------------------------------------------------
// POUR AJOUTER UN TYPE DE CAPTEUR :
//   1. ecrire la classe (heriter d'ISensor) ;
//   2. ajouter une branche dans create() qui lit ses parametres ;
//   3. ajouter son nom dans knownTypes().
// Aucune autre partie du code (registre, serveur HTTP, service) ne change.
// -----------------------------------------------------------------------------

ISensor* create(const ModuleDef& def, QString* error, QObject* parent) {
    const QString type = def.type.toLower();

    if (type == QLatin1String("ld2410") || type == QLatin1String("ld2410c")) {
#ifdef MORFSENSOR_HAVE_SERIALPORT
        const QString port = def.params.value("port").toString();
        if (port.isEmpty()) {
            if (error) *error = QStringLiteral("capteur '%1' : parametre 'port' manquant").arg(def.id);
            return nullptr;
        }
        const int baud    = def.params.value("baud").toInt(256000);
        const int hold    = def.params.value("presence_hold_ms").toInt(2000);
        const int staleMs = def.params.value("stale_ms").toInt(3000);
        return new Ld2410Sensor(def.id, port, baud, hold, staleMs, parent);
#else
        if (error) *error = QStringLiteral("capteur '%1' : type 'ld2410' indisponible "
                                           "(compile sans Qt SerialPort)").arg(def.id);
        return nullptr;
#endif
    }

    if (type == QLatin1String("mock")) {
        const int periodMs = def.params.value("period_ms").toInt(8000);
        return new MockSensor(def.id, periodMs, parent);
    }

    if (error)
        *error = QStringLiteral("type de capteur inconnu : '%1'").arg(def.type);
    return nullptr;
}

QStringList knownTypes() {
    return { QStringLiteral("ld2410"), QStringLiteral("mock") };
}

} // namespace ModuleFactory
} // namespace morfsensor
