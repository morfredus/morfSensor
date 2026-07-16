/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "morfsensor/MockSensor.h"

#include <QTimer>
#include <QJsonObject>

namespace morfsensor {

MockSensor::MockSensor(const QString& id, int periodMs, QObject* parent)
    : ISensor(id, QStringLiteral("presence"), parent),
      m_periodMs(periodMs > 0 ? periodMs : 8000),
      m_timer(new QTimer(this)) {
    m_timer->setInterval(m_periodMs);
    connect(m_timer, &QTimer::timeout, this, &MockSensor::tick);

    // Instantane initial : disponible tout de suite (capteur simule), absent.
    m_last = makeReading();
    m_last.available = true;
    m_last.state     = QStringLiteral("ok");
    QJsonObject v;
    v["present"]   = false;
    v["simulated"] = true;
    m_last.values  = v;
}

MockSensor::~MockSensor() = default;

bool MockSensor::start() {
    m_timer->start();
    return true;
}

void MockSensor::stop() {
    m_timer->stop();
}

SensorReading MockSensor::lastReading() const {
    return m_last;
}

void MockSensor::tick() {
    m_present = !m_present;

    m_last = makeReading();
    m_last.available = true;
    m_last.state     = QStringLiteral("ok");
    QJsonObject v;
    v["present"]   = m_present;
    v["simulated"] = true;
    m_last.values  = v;

    emit readingUpdated(m_last);
}

} // namespace morfsensor
