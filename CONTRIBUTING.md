# Contribuer à morfSensor

Merci de votre intérêt ! morfSensor est un **service de capteurs extensible** :
il doit rester **portable, sobre en dépendances**, et facile à étendre sans
toucher au cœur.

## 1. Philosophie

- **Cœur minimal, extensions isolées.** Le serveur HTTP, le registre et le
  service ne connaissent **aucun** capteur en particulier. Toute la connaissance
  matérielle vit dans une classe `ISensor`. Résister à faire fuiter des détails
  de capteur dans le cœur.
- **Qt Core + Network** obligatoires ; **Qt SerialPort optionnel** (capteurs
  UART). Un capteur qui a besoin d'une autre bibliothèque la garde derrière son
  propre `#ifdef`/option CMake, comme le LD2410C.
- **Portable par construction.** Pas de code spécifique à une plateforme dans le
  cœur ; comportement identique sous Windows, Linux x64 et Raspberry Pi (ARM64).
- **Asynchrone, jamais bloquant.** Un capteur pousse ses mesures (`readingUpdated`)
  et met en cache son instantané (`lastReading`). Le serveur HTTP ne bloque
  jamais sur une lecture matérielle.

## 2. Ajouter un type de capteur

Voir [`docs/fr/INTEGRATION.md`](docs/fr/INTEGRATION.md). En résumé :

1. Créer `include/morfsensor/MonCapteur.h` + `src/MonCapteur.cpp`, héritant de
   `ISensor` (renseigner `kind`, remplir `values`, émettre `readingUpdated`).
2. Ajouter une branche dans `SensorFactory::create` (lecture des paramètres) et
   son nom dans `knownTypes()`.
3. L'ajouter aux sources dans `CMakeLists.txt` (sous une option si une nouvelle
   dépendance est nécessaire).

Aucune modification du registre, du serveur HTTP ni des consommateurs.

## 3. Compiler et tester

```sh
cmake --preset mingw      # ou linux / linux-arm64
cmake --build --preset mingw
```

Exercer le chemin réel (capteur simulé, sans matériel) :

```sh
./build-mingw/service/morfsensor.exe
curl http://127.0.0.1:8788/presence
curl http://127.0.0.1:8788/sensors
```

## 4. Style

- C++17, conventions des projets frères (morfBeacon, morfUpdate) : en-tête de
  licence SPDX, namespace `morfsensor`, commentaires en français expliquant le
  **pourquoi**.
- Fins de ligne : voir `.gitattributes` (LF dans le dépôt).
