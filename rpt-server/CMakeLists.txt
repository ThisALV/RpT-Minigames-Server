add_executable(rpt-server Main.cpp)
target_link_libraries(rpt-server rpt-core rpt-serialization)

install(TARGETS rpt-server RUNTIME)
