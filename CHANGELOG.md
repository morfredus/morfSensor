# Journal des versions - morfSensor

Le format s'inspire de [Keep a Changelog](https://keepachangelog.com/fr/1.1.0/)
et du [versionnage sémantique](https://semver.org/lang/fr/).

## [0.5.2] - 2026-08-21

### Ajouté

- Enregistrement des compilations au niveau CMake (record_compile) : la durée de compile est signalée à morfAnalytics quel que soit le déclencheur (cmake --build direct, morf upgrade, déploiement morfDeploy).

## [0.5.1] - 2026-08-21

### Modifié

- Resynchroniser la copie vendorée de morfdeploy vers 0.17.3.

## [0.5.0] - 2026-08-20

### Ajouté

- Mise à jour de la copie vendorée de morfDeploy 0.14.0 pour le packaging avec
  provenance vérifiée.

## [0.4.3] - 2026-08-18

### Ajouté

- **Déclaration de dépendance système** dans `service.json` : `qt-serialport`
  (paquet Debian `qt6-serialport-dev`), **optionnelle**, `required_for` le
  driver radar `ld2410c`. morfDeploy 0.7.0 la détecte et propose de l'installer
  lors de `deploy`/`install` sur une machine devant piloter le capteur ; son
  absence n'empêche rien (cœur + capteurs simulés). Formalise le besoin repéré au
  build Linux. Aucun changement de code du service.

## [0.4.2] - 2026-08-14

### Corrigé

- **Troncature des grandes réponses HTTP** dans `HttpServer::reply()`. Resynchronisation
  du correctif issu du patron `morfTemplateService` : la méthode fermait la connexion
  sans drainer le tampon d'écriture, ce qui coupait toute réponse dépassant la taille
  du tampon socket (~20 Ko). On attend désormais que `bytesToWrite()` retombe à zéro
  avant `disconnectFromHost()`.
- Resynchronisation de la copie vendorée de **morfBeacon** (`third_party/morf/beacon`)
  en 0.6.1 : même classe de bug corrigée dans son `StatusServer` (grande réponse
  `/status` coupée faute de drainage du tampon d'écriture).

## [0.4.1] - 2026-08-14

### Corrigé

- Description de l'unité systemd : remplacement du tiret cadratin par un tiret
  simple, conformément à la règle de ponctuation du parc.

## [0.4.0] - 2026-08-13

### Ajouté

- **État matériel dans `/status`** (bloc `hardware`, contrat morfBeacon) : morfSensor
  distingue désormais « aucun capteur attendu » (`state: none`, service `ok`) de
  « capteur attendu absent/défaillant » (`state: degraded`, service `warning`), avec
  un libellé lisible (« aucun capteur », « capteur présent », « capteur absent »).
  Le service reste seul juge de son matériel ; morfMonitor l'affiche sans inférer.
- **Drapeau `required` par capteur** (config, défaut `true`) : un capteur optionnel
  absent devient une configuration valide. Une machine sans capteur branché (ex. Pi
  de dev) se déclare `required: false` et n'émet plus de `warning`.

### Modifié

- `ModuleRegistry::state()` découle maintenant de l'état matériel : seul `degraded`
  produit un `warning` (un service sans capteur reste `ok`).
- Rafraîchissement des dépendances vendorées (morfBeacon 0.6.0, morfdeploy).

## [0.3.1] - 2026-07-28

### Documentation

- **Renvoi vers la vue consolidée écran + capteur** dans `docs/fr/CABLAGE.md` :
  sur un Pi qui porte aussi l'écran du dashboard (bus SPI), le capteur (UART) et
  l'écran cohabitent sans conflit de broches ; brochage d'ensemble dans
  `morfDashboard/docs/fr/CABLAGE.md`.

## [0.3.0] - 2026-07-28

### Modifié

- **Configuration regroupée sous `/etc/morfsystem/<service>`.** Tout le parc
  partage désormais un point d'entrée UNIQUE dans `/etc` (`/etc/morfsystem/`),
  qui contient le fichier partagé `morfsystem.json` et un sous-dossier par
  service, au lieu d'un `/etc/<service>` par service à la racine de `/etc`. Sous
  Windows : `%ProgramData%\morfsystem\<service>`. Les données restent sous
  `/opt/<service>`. L'ancien `/etc/<service>` est adopté à l'installation
  (`migrate_from`).


## [0.2.2] - 2026-07-26
### Ajouté

- **Déclaration de l'API dans `/status`.** Le service annonce désormais ses
  routes métier (`GET /presence`, `GET /sensors`, `GET /sensors/{id}`) via le
  point unique `fillAnnouncedDetail` + `morfbeacon::describeService` - la même
  source que morfAnalytics et morfMonitor. Étant sans interface web, il n'émet
  que le bloc `api`, jamais de `web_ui`. Un superviseur du parc peut ainsi
  cartographier son API sans la connaître à l'avance. Le heartbeat reste
  inchangé (il annonce des capacités, pas l'API).

## [0.2.1] - 2026-07-22
### Modifié

- **Installation, mise à jour et désinstallation par `./service.py`** - point
  d'entrée unique multiplateforme (morfdeploy), en remplacement des scripts
  `install-service.sh`/`.ps1`. Le binaire de ce service est inchangé ; seul son
  mode de déploiement évolue.
- **La configuration vit désormais dans `/etc/morfsensor`** (convention Linux),
  séparée du binaire dans `/opt/morfsensor`. Le déplacement est déclaré : la config
  existante est adoptée, jamais écrasée.
- **Enrichissement à la mise à jour** : une clé introduite par une nouvelle
  version est ajoutée avec sa valeur par défaut, sans jamais toucher vos réglages.

## [0.2.0] - 2026-07-21

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
  `IModule` - elle expose `kind()` et un `SensorReading` typé là où `IModule`
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

## [0.1.1] - 2026-07-19

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

## [0.1.0] - 2026-07-16

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
