set(RPT_SERVER_SOURCES
        "src/Main.cpp")

add_executable(rpt-server ${RPT_SERVER_HEADERS} ${RPT_SERVER_SOURCES})
target_include_directories(rpt-server PRIVATE include)
target_link_libraries(rpt-server PRIVATE rpt-core rpt-network rpt-utils rpt-serialization)

install(TARGETS rpt-server RUNTIME)
