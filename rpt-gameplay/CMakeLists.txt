set(RPT_GP_HEADERS_DIR "include/RpT-Gameplay")

set(RPT_GP_HEADERS
        "${RPT_GP_HEADERS_DIR}/Session.hpp")

set(RPT_GP_SOURCES
        "src/Session.cpp")

add_library(rpt-gameplay STATIC ${RPT_GP_HEADERS} ${RPT_GP_SOURCES})
target_include_directories(rpt-gameplay PUBLIC include)
register_doc_for(include)

install(DIRECTORY "include/" TYPE INCLUDE)
install(TARGETS rpt-gameplay LIBRARY)
