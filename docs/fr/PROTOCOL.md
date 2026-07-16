# Protocole HTTP — morfSensor

Retour à l'[index de la documentation](README.md).

---

morfSensor expose une petite API HTTP/1.1 en lecture seule (méthode `GET`
uniquement ; toute autre méthode répond `405`). Réponses en JSON UTF-8, en-tête
`Access-Control-Allow-Origin: *`, connexion fermée après chaque réponse.

Port par défaut : **8788** (`http_port` dans la config). Version du protocole :
`morfsensor/1` (champ `proto` de `/status`).

## `GET /presence`

Endpoint **léger**, conçu pour être sondé fréquemment (le dashboard l'interroge
pour réveiller l'écran). Agrège tous les capteurs de type `presence` (logique
OU).

```json
{
  "present": true,
  "sources": [
    { "id": "presence-salon", "present": true, "available": true }
  ],
  "ts": 1784206069
}
```

- `present` — **le booléen à lire** : vrai dès qu'au moins un capteur détecte.
- `sources` — détail par capteur de présence (facultatif à exploiter).

## `GET /sensors`

Toutes les lectures, tous types confondus.

```json
{
  "count": 2,
  "sensors": [ { /* SensorReading */ }, { /* SensorReading */ } ],
  "ts": 1784206069
}
```

## `GET /sensors/{id}`

La lecture d'un seul capteur (par son `id`). `404` si l'`id` est inconnu.

## Schéma d'une lecture (`SensorReading`)

Commun à tous les capteurs ; seul `values` change selon le `kind`.

```json
{
  "id": "presence-salon",
  "kind": "presence",
  "available": true,
  "state": "ok",
  "ts": 1784206069,
  "values": {
    "present": true,
    "target_state": 1,
    "target_label": "moving",
    "moving_cm": 142,
    "moving_energy": 78,
    "static_cm": 0,
    "static_energy": 0,
    "detect_cm": 300
  }
}
```

| Champ | Sens |
|---|---|
| `id` | identifiant unique du capteur |
| `kind` | famille : `presence`, `temperature`, `humidity`, `distance`, `light`… |
| `available` | le capteur répond-il ? (faux si port fermé, flux tari…) |
| `state` | `ok` \| `warning` \| `error` \| `starting` |
| `ts` | epoch (s) de la mesure |
| `values` | grandeurs propres au type (voir ci-dessous) |

### `values` selon le `kind`

- **presence** (LD2410C) : `present` (bool), `target_state`/`target_label`,
  `moving_cm`, `moving_energy`, `static_cm`, `static_energy`, `detect_cm`.
- **presence** (mock) : `present`, `simulated: true`.
- *À venir* — temperature : `celsius` ; humidity : `percent` ; distance : `cm` ;
  light : `lux`.

## `GET /status`

Compatible **morfBeacon** : les outils du parc (ex. `beacon_status.py` du
dashboard) le lisent tel quel.

```json
{
  "app": "morfSensor", "host": "pi4fred", "version": "0.1.0",
  "proto": "morfsensor/1", "state": "ok", "uptime_s": 3600,
  "ts": 1784206069,
  "metrics": { "sensors_total": 2, "sensors_available": 2, "presence": true }
}
```

## `GET /healthz`

Sonde de vie légère : `{ "status": "ok" }`.

## Annonce réseau (morfBeacon)

Quand morfSensor est compilé **avec** morfBeacon (dépendance facultative), il
diffuse en parallèle de l'API un heartbeat UDP `morfbeacon/1` sur le port du parc
(45454 par défaut), avec `app: "morfSensor"` et `status_port` égal au port HTTP
réel. Un superviseur découvre ainsi le service sans configuration. Voir la
documentation de morfBeacon pour le format du datagramme.

Compilé **sans** morfBeacon, morfSensor ne diffuse pas de heartbeat : seule
l'API HTTP est disponible (le superviseur doit alors connaître son URL).
