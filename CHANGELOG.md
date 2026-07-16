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
  parc, `status_port` aligné sur le port HTTP réel. Dépendance **facultative** au
  build : absente, une simple alerte est émise et morfSensor compile sans
  l'annonce (API HTTP seule), permettant la compilation sur toute plateforme.
- **Démon `morfsensor`** (config JSON, `--config`, `--list-types`, repli sur un
  capteur simulé si aucune config).
- **Service systemd** : `install-service.sh`, `update-service.sh`,
  `morfsensor.service` (installation dans `/opt/morfsensor`, accès `dialout`).
- **Qt SerialPort optionnel** : le service compile sans lui (cœur + `mock`).
- Documentation FR (architecture, protocole, intégration, câblage).

### Intégration

- **RaspberryDashboard** interroge `/presence` (`presence_sensor.py`) : la
  détection LD2410C réveille l'écran **en plus** de l'activité SSH.
