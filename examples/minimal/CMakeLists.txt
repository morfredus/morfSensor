find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(morfsensor_demo main.cpp)
target_link_libraries(morfsensor_demo PRIVATE morfSensor::morfSensor Qt6::Core)

if(MSVC)
    target_compile_options(morfsensor_demo PRIVATE /utf-8)
endif()
