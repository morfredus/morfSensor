/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include "morfsensor/ISensor.h"

class QTimer;

namespace morfsensor {

// -----------------------------------------------------------------------------
// MockSensor : capteur de presence SIMULE, sans materiel.
//
// Fait basculer `present` a intervalle regulier (parametre period_ms). Sert a
// deux choses :
//   - tester l'API HTTP et l'integration dashboard sur une machine sans capteur
//     (typiquement le poste Windows de developpement) ;
//   - servir d'exemple minimal du pattern d'extension ISensor.
//
// Parametres (dans ModuleDef::params) :
//   "period_ms" : demi-periode du basculement present/absent (defaut 8000)
// -----------------------------------------------------------------------------
class MockSensor : public ISensor {
    Q_OBJECT
public:
    MockSensor(const QString& id, int periodMs = 8000, QObject* parent = nullptr);
    ~MockSensor() override;

    bool start() override;
    void stop() override;
    SensorReading lastReading() const override;

private:
    void tick();

    int           m_periodMs;
    QTimer*       m_timer;
    bool          m_present = false;
    SensorReading m_last;
};

} // namespace morfsensor
