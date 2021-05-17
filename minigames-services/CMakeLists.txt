set(MINIGAMES_SERVICES_HEADERS_DIR
        "include/Minigames-Services")

set(MINIGAMES_SERVICES_HEADERS
        "${MINIGAMES_SERVICES_HEADERS_DIR}/ChatService.hpp"
        "${MINIGAMES_SERVICES_HEADERS_DIR}/Grid.hpp")

set(MINIGAMES_SERVICES_SOURCES
        "src/ChatService.cpp"
        "src/Grid.cpp")

add_library(minigames-services STATIC ${MINIGAMES_SERVICES_SOURCES} ${MINIGAMES_SERVICES_HEADERS})
target_include_directories(minigames-services PUBLIC include)
target_link_libraries(minigames-services PUBLIC rpt-core)
register_doc_for(include)

install(DIRECTORY "include/" TYPE INCLUDE)
install(TARGETS minigames-services LIBRARY)
