set(MINIGAMES_SERVICES_HEADERS_DIR
        "include/Minigames-Services")

set(MINIGAMES_SERVICES_HEADERS
        "${MINIGAMES_SERVICES_HEADERS_DIR}/ChatService.hpp"
        "${MINIGAMES_SERVICES_HEADERS_DIR}/Grid.hpp"
        "${MINIGAMES_SERVICES_HEADERS_DIR}/AxisIterator.hpp"
        "${MINIGAMES_SERVICES_HEADERS_DIR}/BoardGame.hpp"
        "${MINIGAMES_SERVICES_HEADERS_DIR}/MinigameService.hpp"
        "${MINIGAMES_SERVICES_HEADERS_DIR}/Acores.hpp"
        "${MINIGAMES_SERVICES_HEADERS_DIR}/Bermudes.hpp" src/Canaries.cpp include/Minigames-Services/Canaries.hpp)

set(MINIGAMES_SERVICES_SOURCES
        "src/ChatService.cpp"
        "src/Grid.cpp"
        "src/AxisIterator.cpp"
        "src/BoardGame.cpp"
        "src/MinigameService.cpp"
        "src/Acores.cpp"
        "src/Bermudes.cpp" src/Canaries.cpp include/Minigames-Services/Canaries.hpp)

add_library(minigames-services STATIC ${MINIGAMES_SERVICES_SOURCES} ${MINIGAMES_SERVICES_HEADERS})
target_include_directories(minigames-services PUBLIC include)
target_link_libraries(minigames-services PUBLIC rpt-core rpt-utils)
register_doc_for(include)

install(DIRECTORY "include/" TYPE INCLUDE)
install(TARGETS minigames-services LIBRARY)
