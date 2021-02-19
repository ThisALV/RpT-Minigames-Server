set(RPT_UTILS_HEADERS_DIR "include/RpT-Utils")

set(RPT_UTILS_HEADERS
        "${RPT_UTILS_HEADERS_DIR}/CommandLineOptionsParser.hpp")

set(RPT_UTILS_SOURCES
        "src/CommandLineOptionsParser.cpp")

add_library(rpt-utils STATIC ${RPT_UTILS_HEADERS} ${RPT_UTILS_SOURCES})
target_include_directories(rpt-utils PUBLIC include)
register_doc_for(include)

install(DIRECTORY "include/" TYPE INCLUDE)
install(TARGETS rpt-utils LIBRARY)
