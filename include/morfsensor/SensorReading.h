/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QJsonObject>
#include <QString>
#include <QDateTime>

namespace morfsensor {

// -----------------------------------------------------------------------------
// SensorReading : instantane d'un capteur, independant de son type.
//
// La structure est volontairement generique : chaque type de capteur remplit
// le champ libre `values` avec SES grandeurs (un radar de presence : present,
// distances, energie ; une sonde meteo : celsius, percent ; un capteur de
// lumiere : lux ; etc.). Le reste (id, kind, disponibilite, etat, horodatage)
// est commun a tous et permet une serialisation uniforme dans l'API HTTP.
// -----------------------------------------------------------------------------
struct SensorReading {
    QString     id;                              // identifiant unique du capteur
    QString     kind;                            // "presence" | "temperature" | ...
    bool        available = false;               // le capteur repond-il ?
    QString     state = QStringLiteral("starting"); // ok | warning | error | starting
    QJsonObject values;                          // grandeurs propres au type
    qint64      ts = 0;                          // epoch (s) de la mesure

    // Serialisation homogene utilisee par /sensors et /sensors/{id}.
    QJsonObject toJson() const {
        QJsonObject o;
        o["id"]        = id;
        o["kind"]      = kind;
        o["available"] = available;
        o["state"]     = state;
        o["values"]    = values;
        o["ts"]        = static_cast<double>(ts);
        return o;
    }

    // Vrai si ce capteur est de type presence ET annonce une presence detectee.
    // Utilise par l'agregation de /presence (voir SensorRegistry).
    bool indicatesPresence() const {
        return kind == QLatin1String("presence")
            && available
            && values.value(QStringLiteral("present")).toBool(false);
    }
};

} // namespace morfsensor
