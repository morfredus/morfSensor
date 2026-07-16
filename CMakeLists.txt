cmake_minimum_required(VERSION 3.21)

# Version pilotee par le fichier VERSION (meme convention que morfBeacon / morfUpdate).
set(MORFSENSOR_VERSION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${MORFSENSOR_VERSION_FILE}")
file(STRINGS "${MORFSENSOR_VERSION_FILE}" MORFSENSOR_VERSION_FROM_FILE LIMIT_COUNT 1)
string(STRIP "${MORFSENSOR_VERSION_FROM_FILE}" MORFSENSOR_VERSION_FROM_FILE)

project(morfSensor VERSION "${MORFSENSOR_VERSION_FROM_FILE}" LANGUAGES CXX)

# --- Standard C++ ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Qt : generation automatique du moc (classes QObject) -----------------
set(CMAKE_AUTOMOC ON)

# Qt Core + Network (serveur HTTP, JSON) obligatoires.
find_package(Qt6 REQUIRED COMPONENTS Core Network)
# SerialPort OPTIONNEL : requis seulement pour les capteurs UART (LD2410C). Sans
# lui, morfSensor compile quand meme (cœur + capteurs simules) ; les drivers
# serie sont simplement absents. Sur le Raspberry : paquet libqt6serialport6.
find_package(Qt6 QUIET COMPONENTS SerialPort)

# --- Dependance morfBeacon (annonce de presence sur le LAN) — FACULTATIVE ---
# En fonctionnement, morfSensor s'annonce via morfBeacon (heartbeat UDP). Mais
# on part du principe que morfBeacon est fourni par le systeme (deja installe) :
# morfSensor n'a PAS a l'embarquer ni a le compiler obligatoirement. On se
# contente de VERIFIER sa presence.
#
# Si morfBeacon est introuvable : simple ALERTE, la compilation continue. Cela
# permet de compiler la bibliotheque morfSensor sur n'importe quelle plateforme
# (pour la livrer) sans avoir morfBeacon sous la main. L'annonce LAN est alors
# absente ; l'API HTTP (le cœur) fonctionne quand meme.
#
# Ordre de recherche :
#   1. cible morfBeacon::morfBeacon deja definie (build parent) ;
#   2. paquet installe   -> find_package(morfBeacon) ;
#   3. sources fournies  -> MORFSENSOR_MORFBEACON_DIR (dossier voisin par defaut).
set(MORFSENSOR_MORFBEACON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../morfBeacon_travail"
    CACHE PATH "Chemin des sources de morfBeacon (si non installe comme paquet)")

set(MORFSENSOR_HAVE_MORFBEACON OFF)
if(TARGET morfBeacon::morfBeacon)
    set(MORFSENSOR_HAVE_MORFBEACON ON)
else()
    find_package(morfBeacon QUIET)
    if(morfBeacon_FOUND)
        set(MORFSENSOR_HAVE_MORFBEACON ON)
    elseif(EXISTS "${MORFSENSOR_MORFBEACON_DIR}/CMakeLists.txt")
        add_subdirectory("${MORFSENSOR_MORFBEACON_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/morfBeacon")
        set(MORFSENSOR_HAVE_MORFBEACON ON)
    endif()
endif()

if(NOT MORFSENSOR_HAVE_MORFBEACON)
    message(WARNING
        "morfSensor : morfBeacon introuvable -> annonce LAN DESACTIVEE. "
        "morfSensor compile quand meme (API HTTP seule). Pour activer l'annonce : "
        "installer morfBeacon, ou definir -DMORFSENSOR_MORFBEACON_DIR=/chemin.")
else()
    message(STATUS "morfSensor : morfBeacon trouve -> annonce LAN activee.")
endif()

# --- Bibliotheque ---------------------------------------------------------
add_library(morfSensor STATIC
    src/SensorRegistry.cpp
    src/SensorHttpServer.cpp
    src/SensorService.cpp
    src/SensorFactory.cpp
    src/MockSensor.cpp
    include/morfsensor/SensorReading.h
    include/morfsensor/ISensor.h
    include/morfsensor/SensorRegistry.h
    include/morfsensor/SensorConfig.h
    include/morfsensor/SensorFactory.h
    include/morfsensor/SensorHttpServer.h
    include/morfsensor/SensorService.h
    include/morfsensor/MockSensor.h
    include/morfsensor/Version.h
)
add_library(morfSensor::morfSensor ALIAS morfSensor)

# Driver LD2410C : compile uniquement si Qt SerialPort est disponible.
if(TARGET Qt6::SerialPort)
    target_sources(morfSensor PRIVATE
        src/Ld2410Sensor.cpp
        include/morfsensor/Ld2410Sensor.h)
    target_link_libraries(morfSensor PRIVATE Qt6::SerialPort)
    target_compile_definitions(morfSensor PRIVATE MORFSENSOR_HAVE_SERIALPORT=1)
    message(STATUS "morfSensor : Qt SerialPort trouve -> driver LD2410C active.")
else()
    message(WARNING "morfSensor : Qt SerialPort absent -> driver LD2410C DESACTIVE "
                    "(cœur + capteurs simules seulement). Sur le Pi : installer "
                    "libqt6serialport6-dev.")
endif()

target_include_directories(morfSensor PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(morfSensor PUBLIC Qt6::Core Qt6::Network)

# Annonce LAN morfBeacon : liee et activee seulement si morfBeacon est present.
# La definition est PUBLIC car elle conditionne un en-tete public (SensorRegistry
# implemente morfbeacon::IMetricsProvider uniquement quand morfBeacon existe) :
# tous les consommateurs doivent voir la meme valeur.
if(MORFSENSOR_HAVE_MORFBEACON)
    target_link_libraries(morfSensor PRIVATE morfBeacon::morfBeacon)
    target_compile_definitions(morfSensor PUBLIC MORFSENSOR_HAVE_MORFBEACON=1)
endif()

# Numero de version accessible depuis le code.
target_compile_definitions(morfSensor PUBLIC MORFSENSOR_VERSION="${PROJECT_VERSION}")

if(MSVC)
    target_compile_options(morfSensor PRIVATE /utf-8)
endif()

# --- Cible service (le demon autonome) ------------------------------------
add_subdirectory(service)

# --- Exemple (compile seulement en build autonome, pas via add_subdirectory) -
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(_morfsensor_standalone ON)
else()
    set(_morfsensor_standalone OFF)
endif()
option(MORFSENSOR_BUILD_EXAMPLES "Compiler l'exemple de demonstration" ${_morfsensor_standalone})

if(MORFSENSOR_BUILD_EXAMPLES)
    add_subdirectory(examples/minimal)
endif()
