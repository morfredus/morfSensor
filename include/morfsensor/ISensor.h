/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QObject>
#include <QString>
#include "morfsensor/SensorReading.h"

namespace morfsensor {

// -----------------------------------------------------------------------------
// ISensor : LE point d'extension de morfSensor.
//
// Chaque capteur physique (ou simule) est une classe qui herite d'ISensor et
// pousse ses mesures de facon ASYNCHRONE. Le service ne bloque jamais sur une
// lecture materielle : il consulte `lastReading()` (instantane mis en cache)
// et reagit au signal `readingUpdated`.
//
// Ajouter un nouveau type de capteur (temperature, humidite, distance, lumiere,
// ...) = ecrire une sous-classe ici + l'enregistrer dans SensorFactory. Rien
// d'autre a modifier dans le service ni dans le serveur HTTP.
// -----------------------------------------------------------------------------
class ISensor : public QObject {
    Q_OBJECT
public:
    // id    : identifiant unique dans le service (ex. "presence-salon").
    // kind  : famille de mesure ("presence", "temperature", "humidity",
    //         "distance", "light", ...) — libre, mais "presence" a un sens
    //         particulier pour l'agregation /presence.
    ISensor(QString id, QString kind, QObject* parent = nullptr)
        : QObject(parent), m_id(std::move(id)), m_kind(std::move(kind)) {}
    ~ISensor() override = default;

    QString id() const   { return m_id; }
    QString kind() const { return m_kind; }

    // Demarre l'acquisition. Renvoie false si l'initialisation echoue
    // immediatement (port introuvable, etc.). Un capteur peut demarrer "en
    // attente" (available=false) puis devenir disponible plus tard.
    virtual bool start() = 0;

    // Arrete proprement l'acquisition (ferme le port, stoppe les timers).
    virtual void stop() = 0;

    // Dernier instantane connu. Toujours sur, meme avant la 1re mesure
    // (renvoie alors available=false, state="starting").
    virtual SensorReading lastReading() const = 0;

signals:
    // Emis a chaque nouvelle mesure. Permet une reaction immediate (le service
    // n'a pas besoin de sonder en boucle).
    void readingUpdated(const morfsensor::SensorReading& reading);

protected:
    // Fabrique un SensorReading pre-rempli (id/kind/ts) que les sous-classes
    // completent avec `available`, `state` et `values`.
    SensorReading makeReading() const {
        SensorReading r;
        r.id   = m_id;
        r.kind = m_kind;
        r.ts   = QDateTime::currentSecsSinceEpoch();
        return r;
    }

private:
    QString m_id;
    QString m_kind;
};

} // namespace morfsensor
