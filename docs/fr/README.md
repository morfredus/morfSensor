# Documentation de morfSensor (français)

Service autonome de capteurs (présence LD2410C et autres) exposant une API HTTP
locale et s'annonçant sur le LAN via morfBeacon.

> 🇬🇧 English documentation: [`docs/en/`](../en/README.md) *(index, in progress)*.
> Retour au [README (français)](../../README.fr.md).

## Comprendre et intégrer

| Document | Contenu |
|---|---|
| [Architecture](ARCHITECTURE.md) | Les classes (`ISensor`, `ModuleRegistry`, `HttpServer`, `Service`) et le fil d'exécution. |
| [Protocole HTTP](PROTOCOL.md) | Les routes (`/presence`, `/sensors`, `/status`, `/healthz`) et le schéma JSON des lectures. |
| [Intégration](INTEGRATION.md) | **Ajouter un type de capteur** ; brancher un consommateur (dashboard). |
| [Câblage LD2410C](CABLAGE.md) | Raccordement du radar à l'UART du Raspberry Pi. |

## À la racine du projet

| Document | Contenu |
|---|---|
| [README](../../README.md) | Présentation générale (anglais). |
| [README (français)](../../README.fr.md) | Présentation générale (français). |
| [Journal des versions](../../CHANGELOG.md) | Historique des versions. |
| [Roadmap](../../ROADMAP.md) | Évolutions envisagées. |
| [Contribuer](../../CONTRIBUTING.md) | Guide de contribution. |
