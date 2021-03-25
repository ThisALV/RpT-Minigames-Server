set(RPT_NETWORK_HEADERS_DIR "include/RpT-Network")

set(RPT_NETWORK_HEADERS
        "${RPT_NETWORK_HEADERS_DIR}/BeastWebsocketBackendBase.inl"
        "${RPT_NETWORK_HEADERS_DIR}/NetworkBackend.hpp")

set(RPT_NETWORK_SOURCES
        "src/BeastWebsocketBackendBase.cpp"
        "src/NetworkBackend.cpp")

find_package(Boost REQUIRED)

add_library(rpt-network STATIC ${RPT_NETWORK_HEADERS} ${RPT_NETWORK_SOURCES})
target_include_directories(rpt-network PUBLIC include PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(rpt-network PUBLIC rpt-core rpt-utils)
register_doc_for(include)

install(DIRECTORY "include/" TYPE INCLUDE)
install(TARGETS rpt-network LIBRARY)
