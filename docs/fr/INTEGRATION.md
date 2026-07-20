# Intégration — morfSensor

Retour à l'[index de la documentation](README.md).

---

Deux intégrations : **ajouter un type de capteur** (côté morfSensor) et
**brancher un consommateur** (une application qui interroge l'API).

## A. Ajouter un type de capteur

Le point d'extension est l'interface `ISensor`. Exemple : une sonde de
température fictive `demoTemp`.

### 1. La classe (`include/morfsensor/DemoTempSensor.h`)

```cpp
#pragma once
#include "morfsensor/ISensor.h"
class QTimer;

namespace morfsensor {
class DemoTempSensor : public ISensor {
    Q_OBJECT
public:
    DemoTempSensor(const QString& id, QObject* parent = nullptr);
    bool start() override;
    void stop() override;
    SensorReading lastReading() const override;
private:
    void tick();
    QTimer*       m_timer;
    SensorReading m_last;
};
} // namespace morfsensor
```

### 2. L'implémentation (`src/DemoTempSensor.cpp`)

```cpp
#include "morfsensor/DemoTempSensor.h"
#include <QTimer>
#include <QJsonObject>

namespace morfsensor {

DemoTempSensor::DemoTempSensor(const QString& id, QObject* parent)
    : ISensor(id, QStringLiteral("temperature"), parent),   // <-- le kind
      m_timer(new QTimer(this)) {
    m_timer->setInterval(5000);
    connect(m_timer, &QTimer::timeout, this, &DemoTempSensor::tick);
    m_last = makeReading();                 // id/kind/ts pré-remplis
}

bool DemoTempSensor::start() { m_timer->start(); return true; }
void DemoTempSensor::stop()  { m_timer->stop(); }
SensorReading DemoTempSensor::lastReading() const { return m_last; }

void DemoTempSensor::tick() {
    m_last = makeReading();
    m_last.available = true;
    m_last.state     = QStringLiteral("ok");
    QJsonObject v;
    v["celsius"] = 21.4;                     // grandeurs propres au type
    m_last.values = v;
    emit readingUpdated(m_last);             // pousse la mesure
}

} // namespace morfsensor
```

### 3. L'enregistrer dans la fabrique (`src/ModuleFactory.cpp`)

```cpp
#include "morfsensor/DemoTempSensor.h"
// ...
if (type == QLatin1String("demotemp"))
    return new DemoTempSensor(def.id, parent);
```

Et ajouter `"demotemp"` à `knownTypes()`.

### 4. Le déclarer dans CMake (`CMakeLists.txt`)

Ajouter `src/DemoTempSensor.cpp` et son en-tête aux sources de la cible
`morfSensor`. Si le capteur a besoin d'une dépendance (I²C, une bibliothèque
tierce), la mettre derrière une option et un `#ifdef`, comme le LD2410C avec Qt
SerialPort.

### 5. L'activer dans la config

```json
{ "type": "demotemp", "id": "temp-bureau" }
```

Rien d'autre à toucher : le registre, le serveur HTTP et les consommateurs le
prennent en charge automatiquement (il apparaît dans `/sensors`).

## B. Brancher un consommateur

Un consommateur ne pilote aucun capteur : il fait une requête HTTP et lit du
JSON. Exemple minimal en Python (le cas réel du dashboard) :

```python
import json, urllib.request

def presence_detected(url="http://127.0.0.1:8788/presence", timeout=0.5):
    try:
        with urllib.request.urlopen(url, timeout=timeout) as r:
            return bool(json.loads(r.read().decode()).get("present"))
    except Exception:
        return False        # service absent -> on ignore, jamais d'exception
```

### Cas réel : RaspberryDashboard

Le dashboard interroge `/presence` à chaque tour de boucle : une présence
détectée compte comme une activité et **réveille l'écran, en plus de l'activité
SSH**. Voir, dans le dépôt RaspberryDashboard :

- `config.py` — `PRESENCE_SENSOR_ENABLED`, `PRESENCE_SENSOR_URL`,
  `PRESENCE_SENSOR_TIMEOUT` ;
- `presence_sensor.py` — la fonction `presence_detected()` ci-dessus ;
- `dashboard.py` — `if PRESENCE_SENSOR_ENABLED and presence_detected():
  last_active = time.time()`.

Comme morfSensor s'annonce aussi via morfBeacon, l'outil `beacon_status.py` du
dashboard le découvre et affiche ses métriques (`/status`) sans configuration.

## Démarrer sans matériel

Pour développer un consommateur sans capteur réel, lancer morfSensor sans config :
il démarre un capteur simulé (`mock`) dont `present` bascule périodiquement.

```sh
./build-mingw/service/morfsensor.exe
watch -n1 'curl -s http://127.0.0.1:8788/presence'
```
