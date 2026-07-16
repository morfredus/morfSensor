/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "morfsensor/SensorRegistry.h"
#include "morfsensor/ISensor.h"

#include <QDateTime>

namespace morfsensor {

SensorRegistry::SensorRegistry(QObject* parent) : QObject(parent) {}
SensorRegistry::~SensorRegistry() = default;

void SensorRegistry::add(ISensor* sensor) {
    if (!sensor)
        return;
    sensor->setParent(this);
    m_sensors.push_back(sensor);
    connect(sensor, &ISensor::readingUpdated,
            this, &SensorRegistry::readingUpdated);
}

void SensorRegistry::startAll() {
    for (ISensor* s : m_sensors)
        s->start();
}

void SensorRegistry::stopAll() {
    for (ISensor* s : m_sensors)
        s->stop();
}

int SensorRegistry::count() const {
    return m_sensors.size();
}

QJsonArray SensorRegistry::sensorsJson() const {
    QJsonArray arr;
    for (const ISensor* s : m_sensors)
        arr.append(s->lastReading().toJson());
    return arr;
}

QJsonObject SensorRegistry::sensorJson(const QString& id, bool* found) const {
    for (const ISensor* s : m_sensors) {
        if (s->id() == id) {
            if (found) *found = true;
            return s->lastReading().toJson();
        }
    }
    if (found) *found = false;
    return QJsonObject{};
}

bool SensorRegistry::anyPresence() const {
    for (const ISensor* s : m_sensors) {
        if (s->lastReading().indicatesPresence())
            return true;
    }
    return false;
}

QJsonObject SensorRegistry::presenceJson() const {
    // Liste des seuls capteurs de presence, avec leur verdict individuel : le
    // consommateur (dashboard) lit `present` (le OU global) et peut, s'il le
    // souhaite, savoir QUEL capteur a declenche.
    QJsonArray sources;
    bool present = false;
    for (const ISensor* s : m_sensors) {
        const SensorReading r = s->lastReading();
        if (r.kind != QLatin1String("presence"))
            continue;
        const bool p = r.indicatesPresence();
        present = present || p;

        QJsonObject src;
        src["id"]        = r.id;
        src["present"]   = p;
        src["available"] = r.available;
        sources.append(src);
    }

    QJsonObject o;
    o["present"] = present;
    o["sources"] = sources;
    o["ts"]      = static_cast<double>(QDateTime::currentSecsSinceEpoch());
    return o;
}

QJsonObject SensorRegistry::metrics() const {
    // Resume compact expose via /status et discute avec le reste du parc.
    int available = 0;
    for (const ISensor* s : m_sensors)
        if (s->lastReading().available)
            ++available;

    QJsonObject m;
    m["sensors_total"]     = m_sensors.size();
    m["sensors_available"] = available;
    m["presence"]          = anyPresence();
    return m;
}

QString SensorRegistry::state() const {
    if (m_sensors.isEmpty())
        return QStringLiteral("starting");
    // "warning" si un capteur declare mais indisponible (ex. LD2410 debranche).
    for (const ISensor* s : m_sensors)
        if (!s->lastReading().available)
            return QStringLiteral("warning");
    return QStringLiteral("ok");
}

} // namespace morfsensor
