/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QString>

namespace morfsensor {

// Version de morfSensor, injectee par CMake depuis le fichier VERSION.
#ifndef MORFSENSOR_VERSION
#  define MORFSENSOR_VERSION "dev"
#endif

inline QString version() { return QStringLiteral(MORFSENSOR_VERSION); }

// Version du protocole HTTP/JSON expose. A incrementer si le format des
// reponses (/sensors, /presence) change de facon incompatible.
inline constexpr const char* kProtocol = "morfsensor/1";

} // namespace morfsensor
