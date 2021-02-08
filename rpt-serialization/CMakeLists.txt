set(RPT_SERIAL_HEADERS_DIR "include/RpT-Serialization")

set(RPT_SERIAL_HEADERS
        "${RPT_SERIAL_HEADERS_DIR}/JsonSceneLoader.hpp")

set(RPT_SERIAL_SOURCES
        "src/JsonSceneLoader.cpp")

find_package(nlohmann_json 3.7.3 REQUIRED CONFIG)
find_package(Lua 5.3 REQUIRED) # Specific version must be chosen, 5.3 is released since 2015
find_package(sol2 3.2.2 REQUIRED CONFIG) # Required for MSVC build fix

add_library(rpt-serialization STATIC ${RPT_SERIAL_HEADERS} ${RPT_SERIAL_HEADERS})
target_include_directories(rpt-serialization PRIVATE ${LUA_INCLUDE_DIR})
target_link_libraries(rpt-serialization PUBLIC rpt-gameplay PRIVATE nlohmann_json::nlohmann_json ${LUA_LIBRARIES} sol2::sol2)

install(DIRECTORY "include/" TYPE INCLUDE)
install(TARGETS rpt-serialization LIBRARY)
