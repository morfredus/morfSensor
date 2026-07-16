/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QObject>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include "morfbeacon/IMetricsProvider.h"

namespace morfsensor {

class ISensor;
struct SensorReading;

// -----------------------------------------------------------------------------
// SensorRegistry : collection des capteurs actifs et point d'agregation.
//
// - detient les ISensor (en devient le parent Qt),
// - fabrique les vues JSON servies par l'API HTTP (/sensors, /presence),
// - implemente morfbeacon::IMetricsProvider : le heartbeat et /status exposent
//   ainsi un resume (nombre de capteurs, presence globale) sans que le reste du
//   code ait a le calculer.
//
// La presence GLOBALE est vraie des qu'AU MOINS un capteur de type "presence"
// detecte quelqu'un (logique OU) : c'est ce que le dashboard interroge pour
// reveiller l'ecran.
// -----------------------------------------------------------------------------
class SensorRegistry : public QObject, public morfbeacon::IMetricsProvider {
    Q_OBJECT
public:
    explicit SensorRegistry(QObject* parent = nullptr);
    ~SensorRegistry() override;

    // Ajoute un capteur (le registre en prend possession). A appeler avant start().
    void add(ISensor* sensor);

    // Demarre / arrete tous les capteurs.
    void startAll();
    void stopAll();

    int count() const;

    // --- Vues JSON pour l'API HTTP ---------------------------------------
    QJsonArray  sensorsJson() const;                 // toutes les lectures
    QJsonObject sensorJson(const QString& id, bool* found) const; // un capteur
    QJsonObject presenceJson() const;                // { present, sources, ts }

    // Vrai si au moins un capteur de presence detecte quelqu'un.
    bool anyPresence() const;

    // --- morfbeacon::IMetricsProvider ------------------------------------
    QJsonObject metrics() const override;            // resume pour /status
    QString     state() const override;              // ok | warning | starting

signals:
    // Relaye toute mise a jour d'un capteur (utile pour du log ou une reaction).
    void readingUpdated(const morfsensor::SensorReading& reading);

private:
    QVector<ISensor*> m_sensors;
};

} // namespace morfsensor
