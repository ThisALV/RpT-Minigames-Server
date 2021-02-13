cmake_minimum_required(VERSION 3.12)

message(STATUS "C Compiler: ${CMAKE_C_COMPILER}, C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Archiver: ${CMAKE_AR}, Ranlib: ${CMAKE_RANLIB}, Linker flags: $ENV{LDFLAGS}")

project(RpTogether-Server LANGUAGES CXX VERSION 0.0.1)

## Init project options

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(Boost_USE_STATIC_LIBS ON)

if(WIN32)
    set(PLATFORM Win32)
elseif(UNIX)
    set(PLATFORM Unix)
else()
    message(FATAL_ERROR "Unable to get target's platform type")
endif()

set(RPT_TARGET_PLATFORM ${PLATFORM} CACHE INTERNAL "Target's platform type")

## Init doc options

find_package(Doxygen)

if(Doxygen_FOUND)
    message(STATUS "Doxygen ${DOXYGEN_VERSION} found.")
    set(RPT_GENERATE_DOC ON)
    set(RPT_DOC_OUTPUT "${CMAKE_BINARY_DIR}/doc")

    message(STATUS "Doc will be generated to ${RPT_DOC_OUTPUT}")
else()
    message(STATUS "Doxygen not found, doc will not be generated.")
    set(RPT_GENERATE_DOC OFF)
endif()

function(register_doc_for INCLUDE_DIR)
    if(RPT_GENERATE_DOC)
        set(ABSOLUTE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}")

        set(RPT_INCLUDE_DIRS ${RPT_INCLUDE_DIRS} ${ABSOLUTE_INCLUDE_DIR} CACHE INTERNAL
                "Header directories to use for API documenation")
        message(DEBUG "Add include dir ${ABSOLUTE_INCLUDE_DIR} for doc.")
    endif()
endfunction()

## Init config

set(RPT_CONFIG "Config.hpp")
set(RPT_CONFIG_DIR "${CMAKE_BINARY_DIR}/rpt-config")
set(RPT_CONFIG_OUTPUT "${RPT_CONFIG_DIR}/RpT-Config/${RPT_CONFIG}")
configure_file(${RPT_CONFIG} ${RPT_CONFIG_OUTPUT})

## Init source directories

add_subdirectory(rpt-gameplay)
add_subdirectory(rpt-core)
add_subdirectory(rpt-serialization)
add_subdirectory(rpt-server)

# Enable tests sources directory if Debug mode is enabled
if("${CMAKE_BUILD_TYPE}" STREQUAL Debug)
    enable_testing()
    add_subdirectory(rpt-tests)
endif()

## Enable doc target

if(RPT_GENERATE_DOC)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_MAN YES)
    set(DOXYGEN_OUTPUT_DIRECTORY ${RPT_DOC_OUTPUT})

    message(DEBUG "Additional include dirs for doc : ${RPT_INCLUDE_DIRS}")
    doxygen_add_docs(doc ${RPT_CONFIG_OUTPUT} ${RPT_INCLUDE_DIRS} ALL)

    install(DIRECTORY "${RPT_DOC_OUTPUT}/html/" DESTINATION share/doc/${CMAKE_PROJECT_NAME}/html)
    install(DIRECTORY "${RPT_DOC_OUTPUT}/man/" TYPE MAN)
endif()
