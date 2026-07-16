# Journal des versions — morfSensor

Le format s'inspire de [Keep a Changelog](https://keepachangelog.com/fr/1.1.0/)
et du [versionnage sémantique](https://semver.org/lang/fr/).

## [0.1.0] — 2026-07-16

### Ajouté

- **Service autonome de capteurs** avec API HTTP locale : routes `/presence`,
  `/sensors`, `/sensors/{id}`, `/status` (compatible morfBeacon) et `/healthz`.
- **Point d'extension `ISensor`** (QObject asynchrone) et fabrique
  `SensorFactory` : ajouter un type de capteur = une classe + une ligne.
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
