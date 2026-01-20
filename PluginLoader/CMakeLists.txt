cmake_minimum_required(VERSION 3.16)
project(PluginLoader LANGUAGES CXX)

find_package(PluginCore CONFIG REQUIRED)

message(STATUS "PluginCore version: ${PluginCore_VERSION}")

set(TARGET_NAME "PluginLoader_${PluginCore_VERSION}")

add_executable(${TARGET_NAME}
  src/main.cpp
)

target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)

target_link_libraries(${TARGET_NAME} PRIVATE d3156::PluginCore)
