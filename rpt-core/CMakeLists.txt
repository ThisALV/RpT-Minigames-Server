set(RPT_CORE_HEADERS_DIR "include/RpT-Core")

set(RPT_CORE_HEADERS
        "${RPT_CORE_HEADERS_DIR}/ServicesEventRequestProtocol.hpp"
        "${RPT_CORE_HEADERS_DIR}/Executor.hpp"
        "${RPT_CORE_HEADERS_DIR}/CommandLineOptionsParser.hpp"
        "${RPT_CORE_HEADERS_DIR}/LoggerView.hpp")

set(RPT_CORE_SOURCES
        "src/ServicesEventRequestProtocol.cpp"
        "src/Executor.cpp"
        "src/CommandLineOptionsParser.cpp"
        "src/LoggerView.cpp")

find_package(Boost REQUIRED)
find_package(spdlog 1.8.1 REQUIRED CONFIG) # Required for forward-decl header, stdout fix and load_args/env_levels()

add_library(rpt-core STATIC ${RPT_CORE_HEADERS} ${RPT_CORE_SOURCES})
target_include_directories(rpt-core PUBLIC include ${RPT_CONFIG_DIR} PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(rpt-core PUBLIC stdc++fs rpt-gameplay spdlog::spdlog) # stdc++fs necessary if GCC < 9
register_doc_for(include)

install(DIRECTORY "include/" TYPE INCLUDE)
install(TARGETS rpt-core LIBRARY)
