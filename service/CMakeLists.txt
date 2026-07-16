# Demon de service morfSensor : execute la bibliotheque en tache de fond.
# Le binaire s'appelle « morfsensor » (comme l'unite systemd et /opt/morfsensor).
add_executable(morfsensor main.cpp)

target_link_libraries(morfsensor PRIVATE morfSensor::morfSensor Qt6::Core)

if(MSVC)
    target_compile_options(morfsensor PRIVATE /utf-8)
endif()

install(TARGETS morfsensor RUNTIME DESTINATION bin)
