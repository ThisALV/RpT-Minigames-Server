set(RPT_SERVER_SOURCES
        "src/Main.cpp")

add_executable(rpt-server ${RPT_SERVER_HEADERS} ${RPT_SERVER_SOURCES})
target_include_directories(rpt-server PRIVATE include ${RPT_CONFIG_DIR})
target_link_libraries(rpt-server PRIVATE stdc++fs rpt-core rpt-serialization) # stdc++fs necessary if GCC < 9

install(TARGETS rpt-server RUNTIME)
