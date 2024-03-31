cmake_minimum_required (VERSION 3.13)

project(HKWireAnalyzer)

add_definitions( -DLOGIC2 )

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(CMAKE_CXX_STANDARD 23)

set(SOURCES
src/HKWire.cpp
src/HKWireAnalyzer.cpp
src/HKWireAnalyzer.h
src/HKWireAnalyzerResults.cpp
src/HKWireAnalyzerResults.h
src/HKWireAnalyzerSettings.cpp
src/HKWireAnalyzerSettings.h
)

add_analyzer_plugin(${PROJECT_NAME} SOURCES ${SOURCES})
