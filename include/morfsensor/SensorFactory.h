/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QString>
#include <QStringList>
#include "morfsensor/SensorConfig.h"

class QObject;

namespace morfsensor {

class ISensor;

// -----------------------------------------------------------------------------
// SensorFactory : fabrique un ISensor a partir d'une declaration (SensorDef).
//
// C'est le point d'extension COMPILE-TIME : pour prendre en charge un nouveau
// materiel (une sonde de temperature, un capteur de lumiere, ...), on ecrit sa
// classe ISensor puis on ajoute une branche dans SensorFactory::create.
//
// La liste des types reconnus est exposee par knownTypes() (pour l'aide et les
// messages d'erreur).
// -----------------------------------------------------------------------------
namespace SensorFactory {

// Cree le capteur correspondant a `def`. Renvoie nullptr si le type est inconnu
// ou si les parametres sont invalides ; `error` (optionnel) est alors renseigne.
ISensor* create(const SensorDef& def, QString* error = nullptr, QObject* parent = nullptr);

// Types de capteurs connus (ex. {"ld2410", "mock"}).
QStringList knownTypes();

} // namespace SensorFactory

} // namespace morfsensor
