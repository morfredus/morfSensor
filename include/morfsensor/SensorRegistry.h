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

// morfBeacon est FACULTATIF (voir CMakeLists.txt). Quand il est present, le
// registre implemente son interface IMetricsProvider pour alimenter le heartbeat
// et /status. Sinon, metrics()/state() restent de simples methodes (utilisees
// par le serveur HTTP), et aucune dependance a morfBeacon n'est requise.
#ifdef MORFSENSOR_HAVE_MORFBEACON
#  include "morfbeacon/IMetricsProvider.h"
#endif

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
class SensorRegistry : public QObject
#ifdef MORFSENSOR_HAVE_MORFBEACON
                     , public morfbeacon::IMetricsProvider
#endif
{
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

    // Resume pour /status. Sert aussi de morfbeacon::IMetricsProvider quand
    // morfBeacon est present (surcharge automatique ; pas de mot-cle 'override'
    // pour rester valide quand la base est absente).
    QJsonObject metrics() const;                     // resume pour /status
    QString     state() const;                       // ok | warning | starting

signals:
    // Relaye toute mise a jour d'un capteur (utile pour du log ou une reaction).
    void readingUpdated(const morfsensor::SensorReading& reading);

private:
    QVector<ISensor*> m_sensors;
};

} // namespace morfsensor
