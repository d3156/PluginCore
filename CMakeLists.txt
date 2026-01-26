cmake_minimum_required(VERSION 3.16)

project(PluginCore
  VERSION 1.0.2
  LANGUAGES CXX
)


add_compile_definitions(LOG_NAME="Core" )
add_compile_definitions( PLUGIN_CORE_VERSION="${PROJECT_VERSION}" )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

file(GLOB_RECURSE SRC_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

add_library(PluginCore STATIC ${SRC_FILES})
add_library(d3156::PluginCore ALIAS PluginCore)

target_include_directories(PluginCore
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/PluginCore>
)

# -------------------------
# Install (components)
# -------------------------
install(TARGETS PluginCore
  EXPORT PluginCoreTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT runtime
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/PluginCore
)

# dev: заголовки
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/PluginCore
  COMPONENT dev
  FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# dev: экспорт таргетов
install(EXPORT PluginCoreTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PluginCore
  NAMESPACE d3156::
  COMPONENT dev
)

# -------------------------
# Package config (dev)
# -------------------------
include(CMakePackageConfigHelpers)

set(PLUGINCORE_CONFIG_INSTALL_DIR
  ${CMAKE_INSTALL_LIBDIR}/cmake/PluginCore
)

# Нужен файл-шаблон cmake/PluginCoreConfig.cmake.in
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PluginCoreConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/PluginCoreConfig.cmake
  INSTALL_DESTINATION ${PLUGINCORE_CONFIG_INSTALL_DIR}
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/PluginCoreConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/PluginCoreConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/PluginCoreConfigVersion.cmake
  DESTINATION ${PLUGINCORE_CONFIG_INSTALL_DIR}
  COMPONENT dev
)

# -------------------------
# CPack (DEB, 2 packages)
# -------------------------
set(CPACK_GENERATOR "DEB")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")

set(CPACK_PACKAGE_VENDOR "d3156")
set(CPACK_PACKAGE_CONTACT "d3156 <you@example.com>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")

set(CPACK_COMPONENTS_ALL runtime dev)
set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME "d3156-plugincore")
set(CPACK_DEBIAN_DEV_PACKAGE_NAME     "d3156-plugincore-dev")
set(CPACK_DEBIAN_DEV_PACKAGE_DEPENDS "d3156-plugincore (= ${PROJECT_VERSION})")
set(CPACK_COMPONENT_DEV_DEPENDS runtime)

include(CPack)
set_target_properties(PluginCore PROPERTIES POSITION_INDEPENDENT_CODE ON)