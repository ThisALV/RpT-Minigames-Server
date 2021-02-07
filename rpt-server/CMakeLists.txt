add_executable(rpt-server Main.cpp)
target_include_directories(rpt-server PRIVATE ${RPT_CONFIG_DIR})
target_link_libraries(rpt-server PRIVATE rpt-core rpt-serialization)

install(TARGETS rpt-server RUNTIME)
