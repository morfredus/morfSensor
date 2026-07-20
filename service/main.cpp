/*
 * morfSensor — demon de service
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Charge une configuration JSON, demarre les capteurs declares, expose l'API
 * HTTP et annonce sa presence sur le LAN (morfBeacon). Concu pour tourner en
 * service (systemd sous Linux/Raspberry, tache sous Windows).
 */

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QTextStream>

#include <morfsensor/Service.h>
#include <morfsensor/ModuleFactory.h>
#include <morfsensor/Version.h>

using morfsensor::ServiceConfig;

namespace {

QTextStream& out() { static QTextStream s(stdout); return s; }
QTextStream& err() { static QTextStream s(stderr); return s; }

// Cherche un fichier de config aux emplacements usuels si aucun n'est fourni.
QString findDefaultConfig() {
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir::current().filePath("morfsensor.json"),
        QDir(exeDir).filePath("morfsensor.json"),
        QDir(exeDir).filePath("config/morfsensor.json"),
#ifdef Q_OS_UNIX
        QStringLiteral("/etc/morfsensor/morfsensor.json"),
#endif
    };
    for (const QString& c : candidates)
        if (QFileInfo::exists(c))
            return c;
    return {};
}

// Config de repli : un capteur simule, prete a tester l'API sans materiel.
ServiceConfig fallbackConfig() {
    ServiceConfig c;
    morfsensor::ModuleDef mock;
    mock.type = QStringLiteral("mock");
    mock.id   = QStringLiteral("presence-sim");
    c.sensors.push_back(mock);
    return c;
}

bool loadConfig(const QString& path, ServiceConfig* outCfg, QString* error) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        *error = QStringLiteral("impossible d'ouvrir %1 : %2").arg(path, f.errorString());
        return false;
    }
    QJsonParseError pe{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        *error = QStringLiteral("JSON invalide dans %1 : %2").arg(path, pe.errorString());
        return false;
    }
    *outCfg = ServiceConfig::fromJson(doc.object());
    return true;
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("morfSensor"));
    QCoreApplication::setApplicationVersion(morfsensor::version());

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("morfSensor — service de capteurs (LD2410C et autres) "
                       "avec API HTTP et annonce LAN morfBeacon."));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption configOpt({"c", "config"},
        QStringLiteral("Fichier de configuration JSON."), QStringLiteral("chemin"));
    QCommandLineOption listOpt("list-types",
        QStringLiteral("Liste les types de capteurs disponibles puis quitte."));
    parser.addOption(configOpt);
    parser.addOption(listOpt);
    parser.process(app);

    if (parser.isSet(listOpt)) {
        out() << "Types de capteurs disponibles : "
              << morfsensor::ModuleFactory::knownTypes().join(", ") << '\n';
        return 0;
    }

    // Determine la configuration : option explicite > recherche > repli mock.
    ServiceConfig config;
    QString configPath = parser.value(configOpt);
    if (configPath.isEmpty())
        configPath = findDefaultConfig();

    if (configPath.isEmpty()) {
        err() << "Aucune configuration trouvee : demarrage avec un capteur simule "
                 "(type 'mock'). Fournir --config pour un vrai capteur.\n";
        config = fallbackConfig();
    } else {
        QString error;
        if (!loadConfig(configPath, &config, &error)) {
            err() << "Erreur de configuration : " << error << '\n';
            return 2;
        }
        out() << "Configuration chargee : " << configPath << '\n';
    }

    morfsensor::Service service(config);
    for (const QString& w : service.warnings())
        err() << "Avertissement : " << w << '\n';

    const bool ok = service.start();
    if (!ok) {
        err() << "Le serveur HTTP n'a pas pu ecouter sur le port "
              << config.httpPort << " (deja utilise ?).\n";
        return 3;
    }

    out() << "morfSensor v" << morfsensor::version() << " demarre : "
          << service.sensorCount() << " capteur(s), API http://"
          << config.bindAddress << ':' << service.httpPort()
          << "/  (routes : /presence /sensors /status /healthz)\n";
    out().flush();

    return app.exec();
}
