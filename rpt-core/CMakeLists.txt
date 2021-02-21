set(RPT_CORE_HEADERS_DIR "include/RpT-Core")

set(RPT_CORE_HEADERS
        "${RPT_CORE_HEADERS_DIR}/ServiceEventRequestProtocol.hpp"
        "${RPT_CORE_HEADERS_DIR}/Executor.hpp"
        "${RPT_CORE_HEADERS_DIR}/LoggerView.hpp"
        "${RPT_CORE_HEADERS_DIR}/InputOutputInterface.hpp"
        "${RPT_CORE_HEADERS_DIR}/InputEvent.hpp")

set(RPT_CORE_SOURCES
        "src/ServiceEventRequestProtocol.cpp"
        "src/Executor.cpp"
        "src/LoggerView.cpp"
        "src/InputEvent.cpp"
        "src/InputOutputInterface.cpp")

find_package(spdlog 1.8.1 REQUIRED CONFIG) # Required for forward-decl header, stdout fix and load_args/env_levels()

add_library(rpt-core STATIC ${RPT_CORE_HEADERS} ${RPT_CORE_SOURCES})
target_include_directories(rpt-core PUBLIC include ${RPT_CONFIG_DIR})
target_link_libraries(rpt-core PUBLIC stdc++fs rpt-gameplay spdlog::spdlog) # stdc++fs necessary if GCC < 9
register_doc_for(include)

install(DIRECTORY "include/" TYPE INCLUDE)
install(TARGETS rpt-core LIBRARY)
