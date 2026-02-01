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

    if(${TYPE} STREQUAL "library-shared")
        add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
    elseif(${TYPE} STREQUAL "library")
        add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
    elseif(${TYPE} STREQUAL "model")
        add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
        set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
        find_package(PluginCore REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE d3156::PluginCore)
    elseif(${TYPE} STREQUAL "plugin")
        add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
        set_target_properties(${PROJECT_NAME} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${PLUGINS_OUTPUT_DIR}"
            RUNTIME_OUTPUT_DIRECTORY "${PLUGINS_OUTPUT_DIR}"
            OUTPUT_NAME "${PROJECT_NAME}"
        )
        find_package(PluginCore REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE d3156::PluginCore)
    endif()

    target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:LOG_NAME="${PROJECT_NAME}">
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endfunction()

function(dependency PATH DEP_NAME)
    set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${PATH}")
    get_filename_component(_build_name "${CMAKE_BINARY_DIR}" NAME)
    set(BUILD_DIR "${LIB_DIR}/${_build_name}")
    set(LIB_PATH "${BUILD_DIR}/lib${DEP_NAME}.a")
    set(LIB_TYPE STATIC)

    if(NOT EXISTS "${LIB_PATH}")
        set(LIB_TYPE SHARED)
        set(LIB_PATH "${BUILD_DIR}/lib${DEP_NAME}.so")
    endif()

    if(EXISTS "${LIB_PATH}")
        message(STATUS "✓ Using prebuilt ${DEP_NAME} (${LIB_TYPE}): ${LIB_PATH}")
        add_library(${DEP_NAME} ${LIB_TYPE} IMPORTED)
        set_target_properties(${DEP_NAME} PROPERTIES
            IMPORTED_LOCATION "${LIB_PATH}"
            INTERFACE_INCLUDE_DIRECTORIES "${LIB_DIR}/include"
        )
    else()
        message(STATUS "⚙️  Building ${DEP_NAME} from source")

        if(NOT EXISTS "${BUILD_DIR}")
            file(MAKE_DIRECTORY "${BUILD_DIR}")
            execute_process(
                COMMAND ${CMAKE_COMMAND}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                "${LIB_DIR}"
                WORKING_DIRECTORY "${BUILD_DIR}"
                RESULT_VARIABLE CONFIG_RESULT
            )

            if(NOT CONFIG_RESULT EQUAL 0)
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
        endif()

        add_subdirectory("${LIB_DIR}" "${BUILD_DIR}")
    endif()

    target_link_libraries(${PROJECT_NAME} PUBLIC ${DEP_NAME})
endfunction()
