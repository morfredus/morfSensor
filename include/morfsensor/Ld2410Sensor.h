/*
 * morfSensor
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QByteArray>
#include "morfsensor/ISensor.h"

class QSerialPort;
class QTimer;

namespace morfsensor {

// -----------------------------------------------------------------------------
// Ld2410Sensor : capteur de presence radar mmWave HLK-LD2410C sur liaison UART.
//
// Le module emet en continu, en "mode rapport", des trames contenant l'etat de
// cible (aucune / mouvement / statique / les deux), les distances et energies.
// Ce driver lit le port serie de facon asynchrone (QSerialPort), decode les
// trames et met a jour l'instantane (`present`, distances, energie).
//
// Parametres (ModuleDef::params) :
//   "port"             : nom du port serie (REQUIS). Ex. "/dev/ttyAMA0",
//                        "/dev/serial0" (Pi) ou "COM5" (Windows).
//   "baud"             : debit (defaut 256000, valeur usine du LD2410C).
//   "presence_hold_ms" : maintien de present=true apres la derniere detection,
//                        pour lisser les micro-coupures (defaut 2000 ; 0 = brut).
//   "stale_ms"         : sans trame valide depuis ce delai, le capteur est
//                        declare indisponible (defaut 3000).
//
// Robustesse : resynchronisation sur l'entete de trame, reouverture automatique
// du port s'il disparait (capteur debranche), chien de garde de fraicheur.
// -----------------------------------------------------------------------------
class Ld2410Sensor : public ISensor {
    Q_OBJECT
public:
    Ld2410Sensor(const QString& id,
                 const QString& portName,
                 int baud = 256000,
                 int presenceHoldMs = 2000,
                 int staleMs = 3000,
                 QObject* parent = nullptr);
    ~Ld2410Sensor() override;

    bool start() override;
    void stop() override;
    SensorReading lastReading() const override;

private:
    bool openPort();                 // tente d'ouvrir le port serie
    void closePortQuietly();         // ferme le port SANS re-emettre errorOccurred
    void onReadyRead();              // octets recus -> tampon
    void parseBuffer();              // extrait les trames completes du tampon
    void applyFrame(const QByteArray& payload); // decode une trame valide
    void onReconnectTick();          // reessaie d'ouvrir le port
    void onStaleTick();              // marque indisponible si plus de trames
    void setUnavailable(const QString& state, const QString& reason);

    QString      m_portName;
    int          m_baud;
    int          m_presenceHoldMs;
    int          m_staleMs;

    QSerialPort* m_port;
    QTimer*      m_reconnectTimer;   // reouverture du port
    QTimer*      m_staleTimer;       // fraicheur des trames
    QByteArray   m_buffer;           // octets en attente de decodage

    qint64        m_lastPresenceMs = 0; // horodatage (monotone) de la derniere detection
    bool          m_handlingError = false; // garde anti-recursion errorOccurred
    SensorReading m_last;
};

} // namespace morfsensor
