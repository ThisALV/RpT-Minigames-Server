#define BOOST_TEST_MODULE Core
#include <boost/test/unit_test.hpp>

#include <RpT-Core/CommandLineOptionsParser.hpp>


using namespace RpT::Core;


BOOST_AUTO_TEST_SUITE(CommandLineOptionsParserTests)

//
// Constructor
//

BOOST_AUTO_TEST_SUITE(Constructor)

BOOST_AUTO_TEST_CASE(NoArguments) {
    const int argc { 1 };
    const char* argv[] { "--unused" };

    const CommandLineOptionsParser cmd_line_options { argc, argv, {} };

    // Check that first argument (program name) is ignored
    BOOST_CHECK(!cmd_line_options.has("unused"));
}

BOOST_AUTO_TEST_CASE(OnlyOptions) {
    const int argc { 4 };
    const char* argv[] { "--unused", "--a", "--b", "--c" };

    const CommandLineOptionsParser cmd_line_options { argc, argv, { "a", "b", "c", "d" } };

    // Check for a, b and c if they're parsed
    BOOST_CHECK(cmd_line_options.has("a"));
    BOOST_CHECK(cmd_line_options.has("b"));
    BOOST_CHECK(cmd_line_options.has("c"));
    // Check for allowed option d if it isn't parsed
    BOOST_CHECK(!cmd_line_options.has("d"));
}

BOOST_AUTO_TEST_CASE(OptionsAndValueAtEnd) {
    const int argc { 5 };
    const char* argv[] { "--unused", "--a", "--b", "--c", "Hello world!" };

    const CommandLineOptionsParser cmd_line_options { argc, argv, { "a", "b", "c", "d" } };

    // Check for a, b and c if they're parsed
    BOOST_CHECK(cmd_line_options.has("a"));
    BOOST_CHECK(cmd_line_options.has("b"));
    BOOST_CHECK(cmd_line_options.has("c"));
    // Check for allowed option d if it isn't parsed
    BOOST_CHECK(!cmd_line_options.has("d"));
    // Check for c option if it has expected value
    BOOST_CHECK_EQUAL(cmd_line_options.get("c"), "Hello world!");
}

BOOST_AUTO_TEST_CASE(OptionsAndValues) {
    const int argc { 6 };
    const char* argv[] { "--unused", "--a", "12345", "--b", "--c", "Hello world!" };

    const CommandLineOptionsParser cmd_line_options { argc, argv, { "a", "b", "c", "d" } };

    // Check for a, b and c if they're parsed
    BOOST_CHECK(cmd_line_options.has("a"));
    BOOST_CHECK(cmd_line_options.has("b"));
    BOOST_CHECK(cmd_line_options.has("c"));
    // Check for allowed option d if it isn't parsed
    BOOST_CHECK(!cmd_line_options.has("d"));
    // Check for a and c options if they have expected values
    BOOST_CHECK_EQUAL(cmd_line_options.get("a"), "12345");
    BOOST_CHECK_EQUAL(cmd_line_options.get("c"), "Hello world!");
}

BOOST_AUTO_TEST_CASE(OptionsWithTwoConsecutiveValues) {
    const int argc { 6 };
    const char* argv[] { "--unused", "--a", "--b", "Hello", "world!", "--c" };

    // Check for constructor if it throws InvalidCommandLineOptions as "world!" hasn't any option to be assigned
    BOOST_CHECK_THROW((CommandLineOptionsParser { argc, argv, { "a", "b", "c", "d" } }),
                      RpT::Core::InvalidCommandLineOptions);
}

BOOST_AUTO_TEST_CASE(ValueAtBegin) {
    const int argc { 5 };
    const char* argv[] { "--unused", "6789", "--a", "--b", "--c" };

    // Check for constructor if it throws InvalidCommandLineOptions as "6789" hasn't any option to be assigned
    BOOST_CHECK_THROW((CommandLineOptionsParser { argc, argv, { "a", "b", "c", "d" } }),
                      RpT::Core::InvalidCommandLineOptions);
}

BOOST_AUTO_TEST_CASE(NotAllowedOptionsWithoutValues) {
    const int argc { 4 };
    const char* argv[] { "--unused", "--a", "--z", "--c" };

    // Check for constructor if it throws UnknownOption as "z" isn't in allowed list
    BOOST_CHECK_THROW((CommandLineOptionsParser { argc, argv, { "a", "b", "c", "d" } }),
                      RpT::Core::InvalidCommandLineOptions);
}

BOOST_AUTO_TEST_CASE(NotAllowedOptionsWithValues) {
    const int argc { 6 };
    const char* argv[] { "--unused", "--a", "Hello", "--z", "world!", "--c" };

    // Check for constructor if it throws UnknownOption as "z" isn't in allowed list
    BOOST_CHECK_THROW((CommandLineOptionsParser { argc, argv, { "a", "b", "c", "d" } }),
                      RpT::Core::InvalidCommandLineOptions);
}

BOOST_AUTO_TEST_SUITE_END()

//
// has() and get() methods
//

/**
 * @brief Default setup used for all has() and get() unit tests
 *
 * Situation :
 *  - Option a enabled, no value
 *  - Option b enabled, value "Hello world!"
 *  - Option c enabled
 *  - Option d allowed, but not enabled
 */
class HasAndGetFixture {
public:
    CommandLineOptionsParser cmd_line_options;

    HasAndGetFixture() : cmd_line_options { 0, nullptr, {} } { // temporary value, doesn't matter
        const int argc { 5 };
        const char* argv[] { "--unused", "--a", "--b", "Hello world!", "--c" };

        cmd_line_options = { argc, argv, { "a", "b", "c", "d" } };
    }
};

BOOST_FIXTURE_TEST_SUITE(Has, HasAndGetFixture)

BOOST_AUTO_TEST_CASE(EnabledWithValue) {
    BOOST_CHECK(cmd_line_options.has("b"));
}

BOOST_AUTO_TEST_CASE(EnabledWithoutValue) {
    BOOST_CHECK(cmd_line_options.has("a"));
}

BOOST_AUTO_TEST_CASE(DisabledButAllowed) {
    BOOST_CHECK(!cmd_line_options.has("d"));
}

BOOST_AUTO_TEST_CASE(DisabledAndNotAllowed) {
    BOOST_CHECK(!cmd_line_options.has("e"));
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_FIXTURE_TEST_SUITE(Get, HasAndGetFixture)

BOOST_AUTO_TEST_CASE(EnabledWithValue) {
    BOOST_CHECK_EQUAL(cmd_line_options.get("b"), "Hello world:");
}

BOOST_AUTO_TEST_CASE(EnabledWithoutValue) {
    BOOST_CHECK_THROW(cmd_line_options.get("a"), RpT::Core::NoValueAssigned);
}

BOOST_AUTO_TEST_CASE(DisabledButAllowed) {
    BOOST_CHECK_THROW(cmd_line_options.get("d"), RpT::Core::UnknownOption);
}

BOOST_AUTO_TEST_CASE(DisabledAndNotAllowed) {
    BOOST_CHECK_THROW(cmd_line_options.get("e"), RpT::Core::UnknownOption);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
