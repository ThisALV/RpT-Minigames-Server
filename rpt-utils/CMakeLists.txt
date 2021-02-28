set(RPT_UTILS_HEADERS_DIR "include/RpT-Utils")

set(RPT_UTILS_HEADERS
        "${RPT_UTILS_HEADERS_DIR}/CommandLineOptionsParser.hpp"
        "${RPT_UTILS_HEADERS_DIR}/LoggerView.hpp")

set(RPT_UTILS_SOURCES
        "src/CommandLineOptionsParser.cpp"
        "src/LoggerView.cpp")

find_package(spdlog 1.8.1 REQUIRED CONFIG) # Required for forward-decl header and stdout fix

add_library(rpt-utils STATIC ${RPT_UTILS_HEADERS} ${RPT_UTILS_SOURCES})
target_include_directories(rpt-utils PUBLIC include ${RPT_CONFIG_DIR})
target_link_libraries(rpt-utils PUBLIC spdlog::spdlog)
register_doc_for(include)

install(DIRECTORY "include/" TYPE INCLUDE)
install(TARGETS rpt-utils LIBRARY)
