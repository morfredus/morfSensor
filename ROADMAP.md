# Roadmap — morfSensor

Pistes envisagées, sans engagement de date. morfSensor doit rester **petit,
portable et sans dépendance superflue** : chaque ajout se pèse à cette aune.

## Nouveaux types de capteurs

Le point d'extension `ISensor` + `SensorFactory` est prêt pour :

- **Température / humidité** — sondes I²C (AHT20, BME280, DHT22…). `kind` =
  `temperature` / `humidity`, `values` = `{ "celsius": …, "percent": … }`.
- **Distance** — capteurs ToF/ultrason (VL53L0X, HC-SR04). `kind` = `distance`.
- **Lumière** — luxmètres (BH1750, TSL2591). `kind` = `light`, `values` =
  `{ "lux": … }`.
- **Qualité d'air / CO₂** — SGP30, MH-Z19.

Chaque ajout : une classe `ISensor`, une branche dans `SensorFactory::create`,
une ligne dans `knownTypes()`. Aucune autre modification.

## Service

- **Journalisation optionnelle** des lectures (fichier tournant / CSV) pour
  l'historique météo.
- **Endpoint `/metrics`** au format Prometheus (en plus du JSON) pour
  supervision.
- **Hot-reload** de la configuration (SIGHUP) sans redémarrer le service.
- **Publication MQTT** en option, pour les intégrations domotiques.
- **Configuration active du LD2410** (mode ingénierie, seuils de portée, durée
  d'absence) via l'API de commande du module, en plus de la simple lecture.

## Portabilité / packaging

- Service Windows natif (en plus du lancement manuel).
- Paquet Debian / image prête pour le Raspberry.
