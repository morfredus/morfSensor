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

# --- Dependance morfBeacon (annonce de presence sur le LAN) ---------------
# morfBeacon est EMBARQUE (copie vendoree) dans third_party/morf/beacon, lie
# statiquement, comme le font ComponentHub / SiteWatch. morfSensor se compile
# ainsi sans dependre d'aucun depot externe : parfait et fonctionnel du premier
# coup, a l'identique sous Windows, Linux x64 et Raspberry Pi (ARM64).
#
# Ne pas editer le code sous third_party/ : la source de verite est le depot
# morfBeacon. Resynchroniser avec scripts/sync-morf.(sh|ps1).
if(NOT TARGET morfBeacon::morfBeacon)
    add_subdirectory(third_party/morf/beacon)
endif()

# --- Bibliotheque ---------------------------------------------------------
add_library(morfSensor STATIC
    src/ModuleRegistry.cpp
    src/HttpServer.cpp
    src/Service.cpp
    src/ModuleFactory.cpp
    src/MockSensor.cpp
    include/morfsensor/SensorReading.h
    include/morfsensor/ISensor.h
    include/morfsensor/ModuleRegistry.h
    include/morfsensor/ServiceConfig.h
    include/morfsensor/ModuleFactory.h
    include/morfsensor/HttpServer.h
    include/morfsensor/Service.h
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

target_link_libraries(morfSensor
    PUBLIC  Qt6::Core Qt6::Network
    PRIVATE morfBeacon::morfBeacon)

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
