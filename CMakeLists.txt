cmake_minimum_required(VERSION 3.12)

message(STATUS "C Compiler: ${CMAKE_C_COMPILER}, C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Archiver: ${CMAKE_AR}, Ranlib: ${CMAKE_RANLIB}, Linker flags: $ENV{LDFLAGS}")

project(RpTogether-Server LANGUAGES CXX VERSION 0.0.1)

## Init project options

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(Boost_USE_STATIC_LIBS ON)

# Debug features are usually enabled only in Debug mode
# User may configure project to enable debug features even in Release mode
option(RPT_FORCE_DEBUG_FEATURES "Force testing and doc generation for Release build type" OFF)

if(("${CMAKE_BUILD_TYPE}" STREQUAL Debug) OR ${RPT_FORCE_DEBUG_FEATURES})
    set(ENABLE_DEBUG_FEATURES ON)
else()
    set(ENABLE_DEBUG_FEATURES OFF)
endif()

## Detect target/runtime platform

if(WIN32)
    set(RPT_TARGET_PLATFORM RPT_RUNTIME_WIN32)
elseif(UNIX)
    set(RPT_TARGET_PLATFORM RPT_RUNTIME_UNIX)
else()
    message(FATAL_ERROR "Unable to get target's platform type")
endif()

## Init doc options only if debug features are ON

if(ENABLE_DEBUG_FEATURES)
    find_package(Doxygen)

    if(Doxygen_FOUND)
        message(STATUS "Doxygen ${DOXYGEN_VERSION} found.")
        set(RPT_GENERATE_DOC ON) # Generate doc only if Doxygen was found
        set(RPT_DOC_STYLESHEETS "${CMAKE_SOURCE_DIR}/doc-style") # Path to custom dark theme
        set(RPT_DOC_OUTPUT "${CMAKE_BINARY_DIR}/doc")

        message(STATUS "Doc will be generated to ${RPT_DOC_OUTPUT}")
    else()
        message(STATUS "Doxygen not found, doc will not be generated.")
        set(RPT_GENERATE_DOC OFF) # If Doxygen was not found, doc will not be generated
    endif()
else() # If debug features are OFF, disable doc generation
    message(STATUS "Release build type without debug features, doc will not be generated.")
    set(RPT_GENERATE_DOC OFF)
endif()

# Add headers directory to Doxygen API doc
function(register_doc_for INCLUDE_DIR)
    if(RPT_GENERATE_DOC)
        # Necessary as function will be called by subdirectories using relative paths
        set(ABSOLUTE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}")

        # Append path to header directories
        set(RPT_INCLUDE_DIRS ${RPT_INCLUDE_DIRS} ${ABSOLUTE_INCLUDE_DIR} CACHE INTERNAL
                "Header directories to use for API documenation")
        message(DEBUG "Add include dir ${ABSOLUTE_INCLUDE_DIR} for doc.")
    endif()
endfunction()

## Init config

# General configuration
set(RPT_CONFIG "Config.hpp")

set(RPT_CONFIG_DIR "${CMAKE_BINARY_DIR}/rpt-config") # Path used for including config header into source code
set(RPT_CONFIG_OUTPUT "${RPT_CONFIG_DIR}/RpT-Config/${RPT_CONFIG}") # Full path of generated misc config header

# Configure general header, making it ready to be included into sources
configure_file(${RPT_CONFIG} ${RPT_CONFIG_OUTPUT})

## Init source directories

add_subdirectory(rpt-gameplay)
add_subdirectory(rpt-core)
add_subdirectory(rpt-serialization)
add_subdirectory(rpt-utils)
add_subdirectory(rpt-network)
add_subdirectory(minigames-server)

# Enable tests sources directory if debug features are ON
if(ENABLE_DEBUG_FEATURES)
    message(STATUS "Enable RpT testing")
    enable_testing()

    add_subdirectory(rpt-tests)
endif()

## Enable doc target if debug features are ON and Doxygen was found

if(ENABLE_DEBUG_FEATURES AND RPT_GENERATE_DOC)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_MAN YES)
    set(DOXYGEN_OUTPUT_DIRECTORY ${RPT_DOC_OUTPUT})

    set(DOXYGEN_HTML_COLORSTYLE_HUE 60) # Dark yellow foreground
    set(DOXYGEN_HTML_EXTRA_STYLESHEET # Enable custom dark theme
            "${RPT_DOC_STYLESHEETS}/custom.css"
            "${RPT_DOC_STYLESHEETS}/custom_dark_theme.css"
            "${RPT_DOC_STYLESHEETS}/override_menu_colors.css")

    message(DEBUG "Additional include dirs for doc : ${RPT_INCLUDE_DIRS}")
    # Doc for all headers (config and include files)
    doxygen_add_docs(doc ${RPT_CONFIG_OUTPUT} ${RPT_INCLUDE_DIRS} ALL)

    install(DIRECTORY "${RPT_DOC_OUTPUT}/html/" DESTINATION share/doc/${CMAKE_PROJECT_NAME}/html)
    install(DIRECTORY "${RPT_DOC_OUTPUT}/man/" TYPE MAN)
endif()
