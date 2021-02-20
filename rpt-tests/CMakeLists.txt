find_package(Boost REQUIRED COMPONENTS unit_test_framework)

# Register test target under ${NAME}-tests with ${SOURCES} cpp files
# Creates variable named ${NAME}_EXEC for adding include directories or libs to the new target
function(register_test NAME SOURCES)
    set(EXEC_NAME "${NAME}-tests")

    add_executable(${EXEC_NAME} ${SOURCES})
    target_link_libraries(${EXEC_NAME} PRIVATE Boost::unit_test_framework)
    set(${NAME}_EXEC ${EXEC_NAME} PARENT_SCOPE)

    add_test(NAME ${NAME} COMMAND ${EXEC_NAME})

    install(TARGETS ${EXEC_NAME} RUNTIME)
endfunction()


register_test(utils "src/CommandLineOptionsParserTests.cpp")
target_link_libraries(${utils_EXEC} PRIVATE rpt-utils)
