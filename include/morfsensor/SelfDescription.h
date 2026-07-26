/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include "morfbeacon/PresenceConfig.h"

namespace morfsensor {

// -----------------------------------------------------------------------------
// fillAnnouncedDetail : renseigne le DETAIL annonce du service dans un
// PresenceConfig, pour que morfbeacon::describeService le serialise dans /status.
//
// morfSensor est un service SANS interface web : il n'annonce donc pas de
// `web_ui` (aucun `webUiPath`), seulement la liste de son API. describeService
// n'emettra que le bloc `api` -- pas de cle `web_ui` fantome.
//
// Point UNIQUE : la meme fonction sert de source a tout observateur du parc.
//
// Le heartbeat morfBeacon reste maigre : il annonce des CAPACITES, pas l'API.
// C'est pourquoi Service.cpp n'appelle pas cette fonction : le detail ne vit que
// dans /status, jamais dans le datagramme de presence.
//
// En-tete (inline) : aucun fichier source ni entree CMake supplementaires.
inline void fillAnnouncedDetail(morfbeacon::PresenceConfig& pc) {
    // API metier. Les routes de cadre -- /status, /healthz -- ne sont pas
    // listees : un observateur les connait deja par le protocole.
    pc.api = {
        {QStringLiteral("GET"), QStringLiteral("/presence"),
         QStringLiteral("etat de presence agrege des capteurs")},
        {QStringLiteral("GET"), QStringLiteral("/sensors"),
         QStringLiteral("liste des capteurs et leur derniere mesure")},
        {QStringLiteral("GET"), QStringLiteral("/sensors/{id}"),
         QStringLiteral("detail d'un capteur designe par son identifiant")},
    };
}

} // namespace morfsensor
