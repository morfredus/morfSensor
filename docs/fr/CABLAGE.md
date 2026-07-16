# Câblage du LD2410C — Raspberry Pi

Retour à l'[index de la documentation](README.md).

---

Le HLK-LD2410C est un radar de présence 24 GHz qui communique en **UART**
(série TTL 3,3 V) à **256000 bauds** par défaut. morfSensor lit ce flux ; il ne
configure pas le module (réglages d'usine suffisants pour la détection).

## Raccordement

Le LD2410C expose 5 broches : `VCC`, `GND`, `TX`, `RX`, `OUT` (sortie tout-ou-rien,
non utilisée ici — morfSensor lit l'UART, plus riche).

| LD2410C | Broche Pi (BCM) | Broche physique | Remarque |
|---|---|---|---|
| `VCC` | 5V | 2 ou 4 | le module accepte 5 V ; ses E/S restent en 3,3 V |
| `GND` | GND | 6 | masse commune |
| `TX`  | GPIO15 / RXD | 10 | **TX du capteur → RX du Pi** |
| `RX`  | GPIO14 / TXD | 8  | RX du capteur ← TX du Pi (non requis en lecture seule) |

> ⚠️ Les E/S du LD2410C sont en 3,3 V : relier `TX`/`RX` directement aux GPIO du
> Pi (3,3 V), **pas** via un niveau 5 V. Alimenter en 5 V, signaux en 3,3 V.

## Activer l'UART matériel du Pi

Par défaut, l'UART performant (PL011) est mobilisé par le Bluetooth sur les
Pi 3/4/5, et le port GPIO n'a que le « mini-UART » (`ttyS0`), peu fiable à
256000 bauds. Pour un UART stable :

1. `sudo raspi-config` → *Interface Options* → *Serial Port* :
   - *login shell over serial* → **Non** (libère le port de la console) ;
   - *serial port hardware* → **Oui**.
2. Dans `/boot/firmware/config.txt` (ou `/boot/config.txt`), basculer le PL011
   sur les GPIO en désactivant le Bluetooth :
   ```
   enable_uart=1
   dtoverlay=disable-bt
   ```
3. Redémarrer. Le capteur est alors sur **`/dev/ttyAMA0`** (alias souvent
   `/dev/serial0`).

Vérifier la présence de trames :

```sh
stty -F /dev/ttyAMA0 256000 raw -echo
timeout 2 cat /dev/ttyAMA0 | xxd | head   # on doit voir des motifs f4 f3 f2 f1 ...
```

## Configurer morfSensor

Dans `morfsensor.json` :

```json
{
  "type": "ld2410",
  "id": "presence-salon",
  "port": "/dev/ttyAMA0",
  "baud": 256000,
  "presence_hold_ms": 2000,
  "stale_ms": 3000
}
```

- `presence_hold_ms` — maintient `present=true` un court instant après la
  dernière détection (lisse le clignotement en limite de portée). `0` = brut.
- `stale_ms` — sans trame valide pendant ce délai, le capteur passe
  `available=false` (câble coupé, capteur muet).

## Droits d'accès

L'utilisateur du service doit appartenir au groupe **`dialout`** pour lire le
port série. `install-service.sh` l'ajoute automatiquement et le service systemd
inclut `SupplementaryGroups=dialout`.

## Dépannage

| Symptôme | Piste |
|---|---|
| `available:false`, `state:"error"` | port introuvable / occupé : vérifier `port`, la console série désactivée, les droits `dialout`. |
| `available:false`, `state:"warning"` | port ouvert mais **aucune trame** : câblage TX/RX (les inverser ?), baud, alimentation. |
| présence erratique | ajuster `presence_hold_ms` ; vérifier la masse commune et la distance de détection. |
