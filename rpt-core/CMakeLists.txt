set(RPT_CORE_HEADERS_DIR "include/RpT-Core")

set(RPT_CORE_HEADERS
        "${RPT_CORE_HEADERS_DIR}/ServiceEventRequestProtocol.hpp"
        "${RPT_CORE_HEADERS_DIR}/Executor.hpp"
        "${RPT_CORE_HEADERS_DIR}/InputOutputInterface.hpp"
        "${RPT_CORE_HEADERS_DIR}/InputEvent.hpp"
        "${RPT_CORE_HEADERS_DIR}/Service.hpp")

set(RPT_CORE_SOURCES
        "src/ServiceEventRequestProtocol.cpp"
        "src/Executor.cpp"
        "src/InputEvent.cpp"
        "src/InputOutputInterface.cpp"
        "src/Service.cpp")

find_package(Boost REQUIRED COMPONENTS filesystem) # Variant and Filesystem requirements, Filesystem must be static-link

add_library(rpt-core STATIC ${RPT_CORE_HEADERS} ${RPT_CORE_SOURCES})
target_include_directories(rpt-core PUBLIC include ${Boost_INCLUDE_DIR})
target_link_libraries(rpt-core PUBLIC rpt-utils rpt-gameplay Boost::filesystem)
register_doc_for(include)

install(DIRECTORY "include/" TYPE INCLUDE)
install(TARGETS rpt-core LIBRARY)
