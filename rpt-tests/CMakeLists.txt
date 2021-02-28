find_package(Boost REQUIRED COMPONENTS unit_test_framework)

set(RPT_TESTING_HEADERS_DIR "include/RpT-Testing") # Utils header

# Register test target under ${NAME}-tests with given additional arguments cpp files
# Creates variable named ${NAME}_EXEC for adding include directories or libs to the new target
function(register_test NAME)
    set(SOURCES_LIST "") # Used to store conversion from array arguments into single-string argument for source files

    math(EXPR LAST_ARGUMENT "${ARGC} - 1") # Index for last argument to handle
    foreach(ADDITIONAL_ARGUMENT RANGE 1 ${LAST_ARGUMENT}) # For each additional argument, last included
        # Argument with index ADDITIONAL_ARGUMENT is holding next cpp file to add
        set(SOURCES_LIST "${SOURCES_LIST};${ARGV${ADDITIONAL_ARGUMENT}}") # Append next given cpp file to sources list
    endforeach()

    set(EXEC_NAME "${NAME}-tests")

    add_executable(${EXEC_NAME} ${SOURCES_LIST} ${RPT_TESTING_HEADERS_DIR})
    target_include_directories(${EXEC_NAME} PRIVATE include)
    target_link_libraries(${EXEC_NAME} PRIVATE Boost::unit_test_framework)
    set(${NAME}_EXEC ${EXEC_NAME} PARENT_SCOPE)

    add_test(NAME ${NAME} COMMAND ${EXEC_NAME})

    install(TARGETS ${EXEC_NAME} RUNTIME)
endfunction()


register_test(utils "src/UtilsTests.cpp" "src/CommandLineOptionsParserTests.cpp")
target_link_libraries(${utils_EXEC} PRIVATE rpt-utils)

register_test(core "src/CoreTests.cpp" "src/InputEventTests.cpp" "src/ServiceTests.cpp" "src/SerProtocolTests.cpp")
target_link_libraries(${core_EXEC} PRIVATE rpt-core)
