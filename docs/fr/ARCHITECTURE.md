# Architecture — morfSensor

Retour à l'[index de la documentation](README.md).

---

morfSensor est un **service Qt (Core + Network, SerialPort optionnel)**, sans
interface graphique. Le cœur ne connaît **aucun** capteur en particulier : toute
la connaissance matérielle est isolée derrière l'interface `ISensor`.

## Les pièces

```
Service  (façade : câble tout à partir d'une ServiceConfig)
├── ModuleRegistry     -> collectionne les ISensor, agrège les lectures
│     └── ISensor (interface, QObject)   ◀── POINT D'EXTENSION
│            ├── Ld2410Sensor   (kind="presence", QSerialPort)
│            └── MockSensor     (kind="presence", simulé)
├── HttpServer   -> API HTTP (/presence, /sensors, /status, /healthz)
└── morfbeacon::Heartbeat -> annonce UDP "morfSensor" (découverte LAN)
        ▲ IMetricsProvider
        └── ModuleRegistry expose un résumé (nombre de capteurs, présence)
```

### `ServiceConfig` (struct)

Chargée depuis un fichier JSON. Définit l'identité annoncée (`appName`), le
serveur HTTP (`httpPort`, `bindAddress`), l'annonce morfBeacon (`beaconEnabled`,
`beaconUdpPort`, `beaconIntervalMs`) et la **liste des capteurs** (`ModuleDef` :
`type`, `id`, `params`). `ServiceConfig::fromJson` fait le parsing.

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

### `ModuleFactory`

Fabrique un `ISensor` à partir d'un `ModuleDef`. C'est le point d'extension
**compile-time** : une branche par type reconnu. `knownTypes()` liste les types.

### `ModuleRegistry` (QObject + `morfbeacon::IMetricsProvider`)

Détient les capteurs, produit les vues JSON (`sensorsJson`, `presenceJson`) et
la **présence globale** (vrai dès qu'un capteur `presence` détecte — logique OU).
Comme il implémente `IMetricsProvider`, le heartbeat et `/status` exposent
automatiquement un résumé.

### `HttpServer` (QObject)

Serveur HTTP/1.1 minimal sur `QTcpServer` (même patron que le `StatusServer` de
morfBeacon : une requête, une réponse, connexion fermée). Route vers `/presence`,
`/sensors`, `/sensors/{id}`, `/status`, `/healthz`.

### `Service` (façade)

L'unique objet manipulé par le démon. À partir d'une `ServiceConfig`, il construit
les capteurs (via `ModuleFactory`), les enregistre, démarre le serveur HTTP puis
le heartbeat morfBeacon (dont le `status_port` annoncé pointe sur le port HTTP
réellement ouvert).

## Fil d'exécution

Tout tourne sur **le thread principal Qt** (boucle d'événements). Les capteurs
poussent leurs mesures via signaux ; `lastReading()` renvoie un instantané déjà
calculé, si bien que le serveur HTTP répond sans jamais bloquer sur une lecture
matérielle. Un driver qui parle à un bus lent doit rester asynchrone (timers,
`QSerialPort::readyRead`) et ne publier qu'un instantané.

## Dépendance morfBeacon (embarquée / vendorée)

morfSensor réutilise `morfbeacon::Heartbeat` pour l'annonce de présence sur le
LAN (aucune duplication de code réseau), et le `ModuleRegistry` implémente
`morfbeacon::IMetricsProvider` pour alimenter le heartbeat et `/status`.

morfBeacon est **embarqué** dans `third_party/morf/beacon` (copie vendorée, liée
statiquement), comme le font ComponentHub et SiteWatch. Le build ne dépend donc
d'**aucun dépôt externe** : `add_subdirectory(third_party/morf/beacon)` suffit,
et morfSensor se compile du premier coup, à l'identique sur toutes les
plateformes. Rien n'est à installer ni à cloner à côté.

Ne pas éditer le code sous `third_party/` : la **source de vérité** est le dépôt
morfBeacon. Pour resynchroniser la copie : `scripts/sync-morf.(sh|ps1)` (recopie
`include/`, `src/`, `VERSION` ; ne touche pas au `CMakeLists.txt` vendoré,
volontairement allégé).

## Portabilité

Le cœur n'a aucun code spécifique à une plateforme (`QTcpServer`, `QUdpSocket`,
JSON Qt). Seuls les drivers matériels en ont : le LD2410C via `QSerialPort`,
compilé uniquement si Qt SerialPort est disponible (sinon le service tourne avec
les capteurs qui n'en dépendent pas). Comportement identique Windows / Linux x64
/ Raspberry Pi (ARM64).
