set(MINIGAMES_SERVER_SOURCES
        "src/Main.cpp")

find_package(Boost REQUIRED COMPONENTS filesystem) # Filesystem requirement, must be statically linked

add_executable(minigames-server ${RPT_SERVER_HEADERS} ${MINIGAMES_SERVER_SOURCES})
target_link_libraries(minigames-server PRIVATE rpt-core rpt-network rpt-utils Boost::filesystem)

install(TARGETS minigames-server RUNTIME)
