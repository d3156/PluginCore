cmake_minimum_required(VERSION 3.16)
project(PluginLoader LANGUAGES CXX)

add_executable(PluginLoader
  src/main.cpp
)

target_compile_features(PluginLoader PRIVATE cxx_std_20)

find_package(PluginCore CONFIG REQUIRED)

target_link_libraries(PluginLoader PRIVATE d3156::PluginCore)
