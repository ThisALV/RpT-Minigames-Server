set(RPT_UTILS_HEADERS_DIR "include/RpT-Utils")

set(RPT_UTILS_HEADERS
        "${RPT_UTILS_HEADERS_DIR}/CommandLineOptionsParser.hpp"
        "${RPT_UTILS_HEADERS_DIR}/LoggingContext.hpp"
        "${RPT_UTILS_HEADERS_DIR}/LoggerView.hpp"
        "${RPT_UTILS_HEADERS_DIR}/HandlingResult.hpp"
        "${RPT_UTILS_HEADERS_DIR}/TextProtocolParser.hpp")

set(RPT_UTILS_SOURCES
        "src/CommandLineOptionsParser.cpp"
        "src/LoggingContext.cpp"
        "src/LoggerView.cpp"
        "src/HandlingResult.cpp"
        "src/TextProtocolParser.cpp")

find_package(spdlog CONFIG)

add_library(rpt-utils STATIC ${RPT_UTILS_HEADERS} ${RPT_UTILS_SOURCES})
target_include_directories(rpt-utils PUBLIC include ${RPT_CONFIG_DIR})
target_link_libraries(rpt-utils PUBLIC spdlog::spdlog)
register_doc_for(include)

install(DIRECTORY "include/" TYPE INCLUDE)
install(TARGETS rpt-utils LIBRARY)
