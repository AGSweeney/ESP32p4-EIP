# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(PLANE_COMPONENT_DIRS
    "./components/vl53l0x"
    "./components/mcp23008"
    "./components/mcp23017"
    "./components/tca9534"
    "./components/tca9555"
    "./components/pcf8574"
    "./components/pcf8575"
    "./components/mcp3008"
    "./components/mcp3208"
    "./components/mcp4725"
    "./components/nau7802"
    "./components/bno086"
    "./components/bmi270"
    "./components/lsm6dsv16x"
    "./components/hx711"
    "./components/hc595"
    "./components/hc165")

list(APPEND EXTRA_COMPONENT_DIRS ${PLANE_COMPONENT_DIRS})

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
idf_build_set_property(MINIMAL_BUILD ON)
project(ESP32-P4-OpENerEIP)
