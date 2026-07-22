# Journal des versions — morfSensor

Le format s'inspire de [Keep a Changelog](https://keepachangelog.com/fr/1.1.0/)
et du [versionnage sémantique](https://semver.org/lang/fr/).

## [Non publié]

## [0.2.1] – 2026-07-22
### Modifié

- **Installation, mise à jour et désinstallation par `./service.py`** — point
  d'entrée unique multiplateforme (morfdeploy), en remplacement des scripts
  `install-service.sh`/`.ps1`. Le binaire de ce service est inchangé ; seul son
  mode de déploiement évolue.
- **La configuration vit désormais dans `/etc/morfsensor`** (convention Linux),
  séparée du binaire dans `/opt/morfsensor`. Le déplacement est déclaré : la config
  existante est adoptée, jamais écrasée.
- **Enrichissement à la mise à jour** : une clé introduite par une nouvelle
  version est ajoutée avec sa valeur par défaut, sans jamais toucher vos réglages.

## [0.2.0] – 2026-07-21

### Modifié

- **Briques d'infrastructure alignées sur les noms du gabarit.** morfSensor
  précède `morfTemplateService` : il a donné naissance au gabarit et en avait
  gardé ses propres noms. Cette divergence empêchait toute comparaison
  mécanique avec l'amont et compliquait la maintenance du squelette.

  | Avant | Après |
  | --- | --- |
  | `SensorHttpServer` | `HttpServer` |
  | `SensorService` | `Service` |
  | `SensorConfig` | `ServiceConfig` |
  | `SensorRegistry` | `ModuleRegistry` |
  | `SensorFactory` | `ModuleFactory` |
  | `SensorDef` | `ModuleDef` |

  Les fichiers correspondants sont renommés à l'identique.

- **Le métier n'est pas touché.** `ISensor`, `SensorReading`, `Ld2410Sensor` et
  `MockSensor` gardent leurs noms : ce sont les points d'extension propres à
  morfSensor, pas des briques d'infrastructure. `ISensor` n'est d'ailleurs pas
  `IModule` — elle expose `kind()` et un `SensorReading` typé là où `IModule`
  expose `type()` et un `statusJson()` générique. Les méthodes métier de
  `ModuleRegistry` (`sensorsJson`, `presenceJson`, `anyPresence`) sont
  conservées telles quelles.

- **Aucun changement fonctionnel ni de contrat.** Le format du fichier de
  configuration est inchangé (clés `sensors`, `http_port`, `beacon`…), l'API
  HTTP est inchangée, et le binaire se comporte à l'identique. Le renommage
  porte sur des identifiants C++ et les noms de fichiers, rien d'autre.

  Cette harmonisation ne remet pas en cause le principe de l'écosystème :
  `morfTemplateService` reste un **gabarit de création**, pas un framework
  d'exécution. Chaque service demeure propriétaire de son implémentation et
  libre de la faire évoluer. Seuls les noms convergent, pas le code.

## [0.1.1] – 2026-07-19

### Modifié

- **Copie vendorée de morfBeacon resynchronisée en 0.2.0** (champ `capabilities`
  du heartbeat). Ajout purement additif et facultatif ; ce projet n'annonce
  aucune capacité et son comportement est strictement inchangé. La
  resynchronisation évite que la copie embarquée ne dérive de l'amont.


### Corrigé

- **La mise à jour ne livrait jamais les nouveaux paramètres de configuration.**
  `update-service.sh` ne recopiait que le binaire et laissait `morfsensor.json`
  intact, par souci de préserver les réglages locaux. Conséquence : un paramètre
  introduit après l'installation restait absent indéfiniment, et la fonction
  correspondante ne s'activait jamais **sans que rien ne le signale**. La mise à
  jour **complète** désormais la configuration (`scripts/linux/merge-config.py`) :
  les valeurs déjà en place ne sont jamais modifiées, les clés manquantes sont
  ajoutées puis listées, et une sauvegarde précède toute écriture. Option
  `--no-config` pour laisser la configuration strictement intacte.
- **La configuration absente n'était pas recréée.** Après une installation
  partielle ou une suppression du dossier, la mise à jour laissait le service
  démarrer sans configuration. Elle est désormais recopiée depuis l'exemple.
- **L'unité systemd n'était pas rafraîchie.** Une modification du fichier
  `.service` dans le dépôt ne parvenait jamais à `/etc/systemd/system` : le
  service continuait de tourner avec l'ancienne définition.

## [0.1.0] — 2026-07-16

### Ajouté

- **Service autonome de capteurs** avec API HTTP locale : routes `/presence`,
  `/sensors`, `/sensors/{id}`, `/status` (compatible morfBeacon) et `/healthz`.
- **Point d'extension `ISensor`** (QObject asynchrone) et fabrique
  `ModuleFactory` : ajouter un type de capteur = une classe + une ligne.
- **Driver LD2410C** (`Ld2410Sensor`) sur `QSerialPort` : décodage des trames
  de rapport, lissage de présence, resynchronisation et reconnexion auto.
- **Capteur simulé** (`MockSensor`) pour tester l'API sans matériel.
- **Annonce LAN via morfBeacon** (heartbeat UDP) : découverte automatique par le
  parc, `status_port` aligné sur le port HTTP réel. **morfBeacon est embarqué**
  (vendoré dans `third_party/morf/beacon`, lié statiquement, comme ComponentHub /
  SiteWatch) : build autonome, sans dépôt externe. Scripts `scripts/sync-morf.*`
  pour resynchroniser depuis la source. `/status` expose l'état de l'annonce
  (`beacon.active`), lisible à l'identique sous Linux et Windows.
- **Démon `morfsensor`** (config JSON, `--config`, `--list-types`, repli sur un
  capteur simulé si aucune config).
- **Service systemd** : `install-service.sh`, `update-service.sh`,
  `morfsensor.service` (installation dans `/opt/morfsensor`, accès `dialout`).
- **Qt SerialPort optionnel** : le service compile sans lui (cœur + `mock`).
- Documentation FR (architecture, protocole, intégration, câblage).

### Corrigé

- **LD2410C : crash (SIGSEGV) au démarrage** quand le port série ne peut pas
  s'ouvrir (UART non activé, droits manquants, capteur absent). Le gestionnaire
  d'erreur appelait `QSerialPort::close()`, qui ré-émet `errorOccurred` →
  récursion infinie → débordement de pile. Corrigé par une garde anti-récursion
  et une fermeture « silencieuse » (signaux bloqués pendant `close()`). Le
  service reste debout et retente l'ouverture ; le capteur est simplement
  rapporté `available:false, state:error` tant que le port est injoignable.

### Intégration

- **RaspberryDashboard** interroge `/presence` (`presence_sensor.py`) : la
  détection LD2410C réveille l'écran **en plus** de l'activité SSH.
