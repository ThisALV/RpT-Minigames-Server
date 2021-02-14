set(RPT_SERVER_HEADERS_DIR "include/RpT-Server")

set(RPT_SERVER_HEADERS
        "${RPT_SERVER_HEADERS_DIR}/CommandLineOptionsParser.hpp")

set(RPT_SERVER_SOURCES
        "src/Main.cpp"
        "src/CommandLineOptionsParser.cpp")

add_executable(rpt-server ${RPT_SERVER_HEADERS} ${RPT_SERVER_SOURCES})
target_include_directories(rpt-server PRIVATE include ${RPT_CONFIG_DIR})
target_link_libraries(rpt-server PRIVATE rpt-core rpt-serialization)
register_doc_for(include)

install(TARGETS rpt-server RUNTIME)
