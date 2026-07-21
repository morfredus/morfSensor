# morfSensor

*Lire dans une autre langue : [English](README.md) · **Français** (ce document).*

[![Version](https://img.shields.io/badge/version-0.2.0-blue)](CHANGELOG.md)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?logo=qt)
![Build](https://img.shields.io/badge/CMake-3.21+-064F8C?logo=cmake)
![License](https://img.shields.io/badge/License-GPL--3.0--only-blue)

**Service autonome de capteurs — présence (LD2410C) et bien d'autres — exposé via une API HTTP locale et annoncé sur le LAN (morfBeacon).**

morfSensor gère le matériel (radar de présence, sondes météo, distance,
lumière…) dans un **service séparé**, pour qu'une application (comme
RaspberryDashboard) n'ait qu'à **interroger une API HTTP** au lieu de piloter le
capteur elle-même. Ajouter un nouveau type de capteur ne touche ni le serveur
HTTP ni les consommateurs : on écrit une classe et on l'enregistre.

## Principe

```
    Capteur matériel                 morfSensor (service)              Consommateur
  ┌──────────────────┐            ┌───────────────────────┐         ┌──────────────┐
  │ LD2410C (UART)   │──trames──▶ │ ISensor → ModuleRegistry│─HTTP─▶ │ RaspberryDash│
  │ (et autres…)     │            │ HttpServer        │        │ (réveil écran)│
  └──────────────────┘            │ + heartbeat morfBeacon  │─UDP──▶ │ (découverte) │
                                  └───────────────────────┘         └──────────────┘
```

- **Le consommateur ne pilote aucun capteur** : il fait un `GET /presence` (ou
  `/sensors`) et lit du JSON.
- **Extensible** : chaque capteur est une classe `ISensor`. Le LD2410C et un
  capteur simulé (`mock`) sont fournis ; température, humidité, distance,
  lumière… suivent le même patron.
- **Multiplateforme** : Linux x86-64, Windows, ARM64 (Raspberry Pi), comme
  morfBeacon / morfUpdate.

## API HTTP (port 8788 par défaut)

| Route | Réponse |
|---|---|
| `GET /presence` | `{ "present": bool, "sources": [...], "ts": ... }` — **interrogé par le dashboard** |
| `GET /sensors` | `{ "sensors": [...], "count": N, "ts": ... }` |
| `GET /sensors/{id}` | lecture d'un capteur (404 si inconnu) |
| `GET /status` | compatible morfBeacon (app, host, version, state, uptime_s, metrics, ts) |
| `GET /healthz` | `{ "status": "ok" }` |

Exemple de lecture (générique, quel que soit le type) :

```json
{ "id": "presence-salon", "kind": "presence", "available": true,
  "state": "ok", "ts": 1784206069,
  "values": { "present": true, "target_label": "moving",
              "moving_cm": 142, "static_cm": 0, "moving_energy": 78 } }
```

## Compiler

Nécessite seulement **Qt 6** (Core, Network ; **SerialPort** pour les capteurs
UART). **morfBeacon est embarqué** (vendoré dans `third_party/morf/beacon`, lié
statiquement) : le build ne dépend d'**aucun dépôt externe** — parfait et
fonctionnel du premier coup, sous Windows, Linux x64 et Raspberry Pi (ARM64).

```sh
cmake --preset mingw        # ou linux / linux-arm64
cmake --build --preset mingw
```

> **Qt SerialPort** est la seule dépendance facultative : absente, le driver
> LD2410C est désactivé (cœur + capteur `mock` compilent) — une alerte, jamais un
> échec. Sur le Raspberry : `sudo apt install libqt6serialport6-dev`.

Pour resynchroniser la copie vendorée de morfBeacon depuis son dépôt source :
`scripts/sync-morf.sh` (ou `scripts\sync-morf.ps1` sous Windows).

## Lancer

```sh
# Démo immédiate, sans matériel ni config (capteur simulé) :
./build-mingw/service/morfsensor.exe
curl http://127.0.0.1:8788/presence

# Avec une vraie configuration :
./build-mingw/service/morfsensor.exe --config config/morfsensor.json
```

Voir [`config/morfsensor.example.json`](config/morfsensor.example.json) pour la
configuration (port HTTP, annonce morfBeacon, liste des capteurs).

## Installer en service (Raspberry Pi)

```sh
# Toutes plateformes : Linux, Windows, Raspberry Pi
sudo ./service.py install      # compile si besoin, installe, demarre
sudo ./service.py update       # recompile, remplace le binaire, redemarre
sudo ./service.py uninstall    # desinscrit, en conservant votre configuration
./service.py status            # ce que le systeme en dit
```

Un seul point d'entree partout. Ce qu'est ce service — son nom, son dossier,
ses configurations — est declare dans `service.json` a cote. Les quatre etapes
d'installation vivent une seule fois pour tout le parc ; seul le gestionnaire
de services change selon la plateforme.

Les anciens scripts `scripts/linux/` et `scripts/windows/` fonctionnent
toujours, inchanges.

## Documentation

- [Architecture](docs/fr/ARCHITECTURE.md) — les classes et le fil d'exécution.
- [Protocole HTTP](docs/fr/PROTOCOL.md) — routes et schéma JSON des lectures.
- [Intégration](docs/fr/INTEGRATION.md) — **ajouter un type de capteur** ;
  brancher un consommateur.
- [Câblage LD2410C](docs/fr/CABLAGE.md) — raccordement à l'UART du Pi.

## Licence

GPL-3.0-only — © 2026 morfredus (Frédéric Biron).
