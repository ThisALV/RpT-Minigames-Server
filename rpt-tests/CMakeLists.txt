find_package(Boost 1.71 REQUIRED COMPONENTS unit_test_framework)

function(register_test NAME SOURCES)
    set(EXEC_NAME "${NAME}-tests")

    add_executable(${EXEC_NAME} ${SOURCES})
    target_link_libraries(${EXEC_NAME} PRIVATE Boost::unit_test_framework)

    add_test(NAME ${NAME} COMMAND ${EXEC_NAME})

    install(TARGETS ${EXEC_NAME} RUNTIME)
endfunction()
