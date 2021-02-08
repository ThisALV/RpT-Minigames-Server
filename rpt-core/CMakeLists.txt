set(RPT_CORE_HEADERS_DIR "include/RpT-Core")

set(RPT_CORE_HEADERS
        "${RPT_CORE_HEADERS_DIR}/NetworkProtocol.hpp")

set(RPT_CORE_SOURCES
        "src/NetworkProtocol.cpp")

find_package(Boost REQUIRED)
find_package(spdlog 1.8.1 REQUIRED CONFIG) # Required for forward-decl header, stdout fix and load_args/env_levels()

add_library(rpt-core STATIC ${RPT_CORE_HEADERS} ${RPT_CORE_SOURCES})
target_include_directories(rpt-core PUBLIC include PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(rpt-core PUBLIC rpt-gameplay PRIVATE spdlog::spdlog)

install(DIRECTORY "include/" TYPE INCLUDE)
install(TARGETS rpt-core LIBRARY)
