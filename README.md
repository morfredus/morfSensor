# morfSensor

*Read in another language: **English** (this document) · [Français](README.fr.md).*

[![Version](https://img.shields.io/badge/version-0.1.0-blue)](CHANGELOG.md)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?logo=qt)
![Build](https://img.shields.io/badge/CMake-3.21+-064F8C?logo=cmake)
![License](https://img.shields.io/badge/License-GPL--3.0--only-blue)

**Autonomous sensor service — presence (LD2410C) and more — exposed over a local HTTP API and announced on the LAN (morfBeacon).**

morfSensor keeps the hardware (presence radar, weather probes, distance, light…)
in a **separate service**, so an application (such as RaspberryDashboard) only
has to **query an HTTP API** instead of driving the sensor itself. Adding a new
sensor type touches neither the HTTP server nor the consumers: you write a class
and register it.

## Principle

```
    Hardware sensor                  morfSensor (service)              Consumer
  ┌──────────────────┐            ┌───────────────────────┐         ┌──────────────┐
  │ LD2410C (UART)   │──frames──▶ │ ISensor → SensorRegistry│─HTTP─▶ │ RaspberryDash│
  │ (and others…)    │            │ SensorHttpServer        │        │ (wake screen)│
  └──────────────────┘            │ + morfBeacon heartbeat  │─UDP──▶ │ (discovery)  │
                                  └───────────────────────┘         └──────────────┘
```

- **The consumer drives no hardware**: it does a `GET /presence` (or `/sensors`)
  and reads JSON.
- **Extensible**: each sensor is an `ISensor` class. The LD2410C and a simulated
  (`mock`) sensor ship in the box; temperature, humidity, distance, light… follow
  the same pattern.
- **Cross-platform**: Linux x86-64, Windows, ARM64 (Raspberry Pi), like
  morfBeacon / morfUpdate.

## HTTP API (port 8788 by default)

| Route | Response |
|---|---|
| `GET /presence` | `{ "present": bool, "sources": [...], "ts": ... }` — **polled by the dashboard** |
| `GET /sensors` | `{ "sensors": [...], "count": N, "ts": ... }` |
| `GET /sensors/{id}` | one sensor's reading (404 if unknown) |
| `GET /status` | morfBeacon-compatible (app, host, version, state, uptime_s, metrics, ts) |
| `GET /healthz` | `{ "status": "ok" }` |

Reading example (generic, whatever the type):

```json
{ "id": "presence-salon", "kind": "presence", "available": true,
  "state": "ok", "ts": 1784206069,
  "values": { "present": true, "target_label": "moving",
              "moving_cm": 142, "static_cm": 0, "moving_energy": 78 } }
```

## Build

Needs Qt 6 (Core, Network; **SerialPort** for UART sensors) and the **morfBeacon**
library cloned alongside (`../morfBeacon`).

```sh
cmake --preset mingw        # or linux / linux-arm64
cmake --build --preset mingw
```

> If Qt SerialPort is missing, morfSensor still builds (core + `mock` sensor);
> only the LD2410C driver is disabled. On the Raspberry Pi:
> `sudo apt install libqt6serialport6-dev`.

## Run

```sh
# Instant demo, no hardware nor config (simulated sensor):
./build-mingw/service/morfsensor.exe
curl http://127.0.0.1:8788/presence

# With a real configuration:
./build-mingw/service/morfsensor.exe --config config/morfsensor.json
```

See [`config/morfsensor.example.json`](config/morfsensor.example.json) for the
configuration (HTTP port, morfBeacon announce, sensor list).

## Install as a service (Raspberry Pi)

```sh
sudo ./scripts/linux/install-service.sh      # build, copy to /opt/morfsensor, enable systemd
journalctl -u morfsensor -f
```

## Documentation

French documentation in [`docs/fr/`](docs/fr/README.md): architecture, HTTP
protocol, **adding a sensor type**, LD2410C wiring.

## License

GPL-3.0-only — © 2026 morfredus (Frédéric Biron).
