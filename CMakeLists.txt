cmake_minimum_required(VERSION 3.12)

project(RpTogether-Server LANGUAGES CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(Boost_USE_STATIC_LIBS ON)

set(RPT_CONFIG_DIR "${CMAKE_BINARY_DIR}/rpt-config")
set(RPT_CONFIG "Config.hpp")
configure_file(${RPT_CONFIG} "${RPT_CONFIG_DIR}/Rpt-Config/${RPT_CONFIG}")

add_subdirectory(rpt-gameplay)
add_subdirectory(rpt-core)
add_subdirectory(rpt-serialization)
add_subdirectory(rpt-server)

if("${CMAKE_BUILD_TYPE}" STREQUAL Debug)
    enable_testing()
    add_subdirectory(rpt-tests)
endif()
