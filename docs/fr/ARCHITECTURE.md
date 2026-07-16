# Architecture — morfSensor

Retour à l'[index de la documentation](README.md).

---

morfSensor est un **service Qt (Core + Network, SerialPort optionnel)**, sans
interface graphique. Le cœur ne connaît **aucun** capteur en particulier : toute
la connaissance matérielle est isolée derrière l'interface `ISensor`.

## Les pièces

```
SensorService  (façade : câble tout à partir d'une SensorConfig)
├── SensorRegistry     -> collectionne les ISensor, agrège les lectures
│     └── ISensor (interface, QObject)   ◀── POINT D'EXTENSION
│            ├── Ld2410Sensor   (kind="presence", QSerialPort)
│            └── MockSensor     (kind="presence", simulé)
├── SensorHttpServer   -> API HTTP (/presence, /sensors, /status, /healthz)
└── morfbeacon::Heartbeat -> annonce UDP "morfSensor" (découverte LAN)
        ▲ IMetricsProvider
        └── SensorRegistry expose un résumé (nombre de capteurs, présence)
```

### `SensorConfig` (struct)

Chargée depuis un fichier JSON. Définit l'identité annoncée (`appName`), le
serveur HTTP (`httpPort`, `bindAddress`), l'annonce morfBeacon (`beaconEnabled`,
`beaconUdpPort`, `beaconIntervalMs`) et la **liste des capteurs** (`SensorDef` :
`type`, `id`, `params`). `SensorConfig::fromJson` fait le parsing.

### `ISensor` (interface, QObject)

Le **seul** point d'extension. Chaque capteur en hérite et :

- déclare son `id()` et son `kind()` (`"presence"`, `"temperature"`, …) ;
- pousse ses mesures de façon **asynchrone** via le signal `readingUpdated` ;
- met en cache son instantané, renvoyé par `lastReading()` (un `SensorReading`).

`start()` / `stop()` gèrent le cycle de vie. Un capteur peut démarrer « en
attente » (`available=false`) puis devenir disponible.

### `SensorReading` (struct)

Instantané homogène pour tous les capteurs : `id`, `kind`, `available`, `state`
(`ok`/`warning`/`error`/`starting`), `ts`, et un objet libre `values` où chaque
type met ses grandeurs. `indicatesPresence()` sert à l'agrégation `/presence`.

### `SensorFactory`

Fabrique un `ISensor` à partir d'un `SensorDef`. C'est le point d'extension
**compile-time** : une branche par type reconnu. `knownTypes()` liste les types.

### `SensorRegistry` (QObject + `morfbeacon::IMetricsProvider`)

Détient les capteurs, produit les vues JSON (`sensorsJson`, `presenceJson`) et
la **présence globale** (vrai dès qu'un capteur `presence` détecte — logique OU).
Comme il implémente `IMetricsProvider`, le heartbeat et `/status` exposent
automatiquement un résumé.

### `SensorHttpServer` (QObject)

Serveur HTTP/1.1 minimal sur `QTcpServer` (même patron que le `StatusServer` de
morfBeacon : une requête, une réponse, connexion fermée). Route vers `/presence`,
`/sensors`, `/sensors/{id}`, `/status`, `/healthz`.

### `SensorService` (façade)

L'unique objet manipulé par le démon. À partir d'une `SensorConfig`, il construit
les capteurs (via `SensorFactory`), les enregistre, démarre le serveur HTTP puis
le heartbeat morfBeacon (dont le `status_port` annoncé pointe sur le port HTTP
réellement ouvert).

## Fil d'exécution

Tout tourne sur **le thread principal Qt** (boucle d'événements). Les capteurs
poussent leurs mesures via signaux ; `lastReading()` renvoie un instantané déjà
calculé, si bien que le serveur HTTP répond sans jamais bloquer sur une lecture
matérielle. Un driver qui parle à un bus lent doit rester asynchrone (timers,
`QSerialPort::readyRead`) et ne publier qu'un instantané.

## Dépendance morfBeacon (facultative)

Quand morfBeacon est disponible, morfSensor réutilise `morfbeacon::Heartbeat`
pour l'annonce de présence sur le LAN (aucune duplication de code réseau), et le
`SensorRegistry` implémente `morfbeacon::IMetricsProvider` pour alimenter le
heartbeat et `/status`.

morfBeacon est **facultatif au build** : on part du principe qu'il est fourni par
le système. S'il est introuvable, le CMake n'émet qu'une **alerte** (jamais
d'échec) et morfSensor est compilé **sans** l'annonce LAN — l'API HTTP reste
pleinement fonctionnelle. Cela permet de compiler/livrer la bibliothèque sur une
plateforme où morfBeacon n'est pas présent. Tout le couplage à morfBeacon est
isolé derrière la macro `MORFSENSOR_HAVE_MORFBEACON` (posée par CMake).

Ordre de recherche : cible déjà définie → `find_package(morfBeacon)` (paquet
installé) → sources `MORFSENSOR_MORFBEACON_DIR` (voisin `../morfBeacon_travail`
par défaut). Même logique que Qt SerialPort pour le driver LD2410C.

## Portabilité

Le cœur n'a aucun code spécifique à une plateforme (`QTcpServer`, `QUdpSocket`,
JSON Qt). Seuls les drivers matériels en ont : le LD2410C via `QSerialPort`,
compilé uniquement si Qt SerialPort est disponible (sinon le service tourne avec
les capteurs qui n'en dépendent pas). Comportement identique Windows / Linux x64
/ Raspberry Pi (ARM64).
