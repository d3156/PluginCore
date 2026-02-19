get_filename_component(WORKSPACE_ROOT "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
get_filename_component(WORKSPACE_ROOT "${WORKSPACE_ROOT}" DIRECTORY)
set(CMAKE_PROJECT_TOP_DIR "${WORKSPACE_ROOT}" CACHE PATH "")

function(create_target TYPE)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(PLUGINS_OUTPUT_DIR "${CMAKE_PROJECT_TOP_DIR}/Plugins")
    add_compile_definitions(${PROJECT_NAME}_VERSION="${PROJECT_VERSION}")
    file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    )
    list(SORT SRC_FILES)
    set(FINAL_HASH "")

    foreach(source_file ${SRC_FILES})
        file(MD5 ${source_file} FILE_MD5)
        string(MD5 FINAL_HASH "${FINAL_HASH}${FILE_MD5}")
    endforeach()

    message(STATUS "${PROJECT_NAME} FINAL_HASH: ${FINAL_HASH}")

    if(${TYPE} STREQUAL "library-shared")
        add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
    elseif(${TYPE} STREQUAL "library")
        add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
    elseif(${TYPE} STREQUAL "model")
        add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
        set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    elseif(${TYPE} STREQUAL "plugin")
        add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
        set_target_properties(${PROJECT_NAME} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${PLUGINS_OUTPUT_DIR}"
            RUNTIME_OUTPUT_DIRECTORY "${PLUGINS_OUTPUT_DIR}"
            OUTPUT_NAME "${PROJECT_NAME}"
        )
    else()
        message(FATAL_ERROR "Unknown target type: ${TYPE}")
    endif()

    find_package(PluginCore CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE d3156::PluginCore)
    target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:LOG_NAME="${PROJECT_NAME}">
        FULL_NAME="${PROJECT_NAME}_${PROJECT_VERSION}:${FINAL_HASH}"
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endfunction()

function(find_project_cmake PROJECT_NAME START_DIR OUT_PATH)
    message(STATUS "Find ${PROJECT_NAME} in ${START_DIR}")
    file(GLOB_RECURSE CMAKE_FILES_ ABSOLUTE "${START_DIR}/**/CMakeLists.txt")

    foreach(CMAKE_FILE ${CMAKE_FILES_})
        file(READ "${CMAKE_FILE}" CONTENT)
        string(TOUPPER "${CONTENT}" CONTENT_UPPER)
        string(TOUPPER "${PROJECT_NAME}" PROJECT_UPPER)
        string(FIND "${CONTENT_UPPER}" "PROJECT(${PROJECT_UPPER}" INDEX)

        if(NOT INDEX EQUAL -1)
            get_filename_component(FOLDER_PATH "${CMAKE_FILE}" DIRECTORY ABSOLUTE)
            set(${OUT_PATH} "${FOLDER_PATH}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()

function(get_repo_name GIT_URL OUT_NAME)
    string(REGEX REPLACE "/$" "" _url "${GIT_URL}")
    string(REGEX REPLACE ".*/|.*:" "" _name "${_url}")
    string(REGEX REPLACE "\\.git$" "" _name "${_name}")
    set(${OUT_NAME} "${_name}" PARENT_SCOPE)
endfunction()

function(dependency PATH DEP_NAME GIT_URL)
    set(PATH "${CMAKE_CURRENT_SOURCE_DIR}/${PATH}")
    set(LIB_DIR "")
    find_project_cmake(${DEP_NAME} ${PATH} LIB_DIR)

    if(NOT EXISTS "${LIB_DIR}")
        message(STATUS "⬇ Cloning EasyHttpLib from ${GIT_URL}")
        get_repo_name(${GIT_URL} REPO_NAME)
        execute_process(
            COMMAND git clone ${GIT_URL} "${PATH}/${REPO_NAME}"
            RESULT_VARIABLE GIT_RESULT
        )

        if(NOT GIT_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to clone ${GIT_URL}")
        endif()

        find_project_cmake(${DEP_NAME} ${PATH} LIB_DIR)
    endif()

    get_filename_component(_build_name "${CMAKE_BINARY_DIR}" NAME)
    set(BUILD_DIR "${LIB_DIR}/${_build_name}")
    set(LIB_PATH "${BUILD_DIR}/lib${DEP_NAME}.a")
    set(LIB_TYPE STATIC)

    if(NOT EXISTS "${BUILD_DIR}")
        file(MAKE_DIRECTORY "${BUILD_DIR}")
    endif()

    get_cmake_property(_cache_vars CACHE_VARIABLES)
    set(_forward_args)

    foreach(var ${_cache_vars})
        get_property(_type CACHE ${var} PROPERTY TYPE)

        if(NOT _type STREQUAL "INTERNAL" AND NOT _type STREQUAL "STATIC")
            if(DEFINED ${var})
                list(APPEND _forward_args "-D${var}=${${var}}")
            endif()
        endif()
    endforeach()

    message(STATUS "cmake -S ${LIB_DIR} ${BUILD_DIR} ${_forward_args}")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
        -S "${LIB_DIR}"
        -B "${BUILD_DIR}"
        ${_forward_args}
        RESULT_VARIABLE CONFIG_RESULT
    )

    if(NOT CONFIG_RESULT EQUAL 0)
        message(STATUS "---- ${REPO_NAME} configure stdout ----")
        message(STATUS "${CONFIG_OUT}")
        message(STATUS "---- ${REPO_NAME} configure stderr ----")
        message(STATUS "${CONFIG_ERR}")
        message(FATAL_ERROR "Failed to configure ${DEP_NAME}")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build . --parallel
        WORKING_DIRECTORY "${BUILD_DIR}"
        RESULT_VARIABLE BUILD_RESULT
    )

    if(NOT BUILD_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to build ${DEP_NAME}")
    endif()

    if(NOT EXISTS "${LIB_PATH}")
        set(LIB_TYPE SHARED)
        set(LIB_PATH "${BUILD_DIR}/lib${DEP_NAME}.so")
    endif()

    message(STATUS "✓ Using prebuilt ${DEP_NAME} (${LIB_TYPE}): ${LIB_PATH}")
    add_library(${DEP_NAME} ${LIB_TYPE} IMPORTED)
    set_target_properties(${DEP_NAME} PROPERTIES
        IMPORTED_LOCATION "${LIB_PATH}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIB_DIR}/include"
    )
    target_link_libraries(${PROJECT_NAME} PUBLIC ${DEP_NAME})
endfunction()
