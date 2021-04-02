#include <RpT-Testing/TestingUtils.hpp>

#include <RpT-Utils/LoggingContext.hpp>
#include <RpT-Utils/LoggerView.hpp>


using namespace RpT::Utils;


// << overloads must be defined inside this namespace, so ADL can be performed to find them when boost use <<
// operator on RpT::Utils types
namespace RpT::Utils {


// Required for Boost.Test logging
std::ostream& operator<<(std::ostream& out, const LogLevel api_logging_level) {
    switch (api_logging_level) {
    case LogLevel::TRACE:
        return out << "TRACE";
    case LogLevel::DEBUG:
        return out << "DEBUG";
    case LogLevel::INFO:
        return out << "INFO";
    case LogLevel::WARN:
        return out << "WARN";
    case LogLevel::ERR:
        return out << "ERR";
    case LogLevel::FATAL:
        return out << "FATAL";
    }
}


}


BOOST_AUTO_TEST_SUITE(LoggingContextTests)

/*
 * Constructor tests
 */

BOOST_AUTO_TEST_SUITE(Constructor)

BOOST_AUTO_TEST_CASE(DefaultLoggingLevel) {
    const LoggingContext logging_context;

    // Default logging lvl should be INFO
    BOOST_CHECK_EQUAL(logging_context.retrieveLoggingLevel(), LogLevel::INFO);
    // Logging should be enabled by default
    BOOST_CHECK(logging_context.isEnabled());
}

BOOST_AUTO_TEST_CASE(FatalLoggingLevel) {
    const LoggingContext logging_context { LogLevel::FATAL };

    BOOST_CHECK_EQUAL(logging_context.retrieveLoggingLevel(), LogLevel::FATAL);
    // Logging should be enabled by default
    BOOST_CHECK(logging_context.isEnabled());
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * newLoggerFor() tests
 */

BOOST_AUTO_TEST_SUITE(NewLoggerFor)

BOOST_AUTO_TEST_CASE(OneRegisteredLogger) {
    LoggingContext logging_context;

    // Registering new loggers
    const LoggerView logger_a { "LoggerA", logging_context };

    // Should be equals, as generic name (or general purpose) is "LoggerA" and UID should be 0
    BOOST_CHECK_EQUAL(logger_a.name(), "LoggerA-0");
}

BOOST_AUTO_TEST_CASE(ManyRegisteredLoggersWithSamePurpose) {
    LoggingContext logging_context;

    // Registering new loggers
    const LoggerView logger_a0 { "LoggerA", logging_context };
    const LoggerView logger_a1 { "LoggerA", logging_context };
    const LoggerView logger_a2 { "LoggerA", logging_context };

    // Should be equals, as generic name is "LoggerA" and UID increments by 1 for each logger registered with same name
    BOOST_CHECK_EQUAL(logger_a0.name(), "LoggerA-0");
    BOOST_CHECK_EQUAL(logger_a1.name(), "LoggerA-1");
    BOOST_CHECK_EQUAL(logger_a2.name(), "LoggerA-2");
}

BOOST_AUTO_TEST_CASE(ManyRegisteredLoggersWithDifferentPurposes) {
    LoggingContext logging_context;

    // Registering new loggers
    const LoggerView logger_a { "LoggerA", logging_context };
    const LoggerView logger_b { "LoggerB", logging_context };
    const LoggerView logger_c { "LoggerC", logging_context };

    // Should be equals, as generic names are "LoggerA...B...C" and are always different (so UID isn't incremented)
    BOOST_CHECK_EQUAL(logger_a.name(), "LoggerA-0");
    BOOST_CHECK_EQUAL(logger_b.name(), "LoggerB-0");
    BOOST_CHECK_EQUAL(logger_c.name(), "LoggerC-0");
}

BOOST_AUTO_TEST_CASE(ManyRegiteredLoggersWithDifferentAndSamePurposes) {
    LoggingContext logging_context;

    /*
     * Registering new loggers
     */

    const LoggerView logger_a0 { "LoggerA", logging_context };
    const LoggerView logger_a1 { "LoggerA", logging_context };
    const LoggerView logger_a2 { "LoggerA", logging_context };

    const LoggerView logger_b0 { "LoggerB", logging_context };
    const LoggerView logger_b1 { "LoggerB", logging_context };

    const LoggerView logger_c0 { "LoggerC", logging_context };
    const LoggerView logger_c1 { "LoggerC", logging_context };
    const LoggerView logger_c2 { "LoggerC", logging_context };
    const LoggerView logger_c3 { "LoggerC", logging_context };

    // Should be equals, as generic name is "LoggerA" and UID increments by 1 for each logger registered with same name
    BOOST_CHECK_EQUAL(logger_a0.name(), "LoggerA-0");
    BOOST_CHECK_EQUAL(logger_a1.name(), "LoggerA-1");
    BOOST_CHECK_EQUAL(logger_a2.name(), "LoggerA-2");

    // Should be equals, as generic name is "LoggerB" and UID increments by 1 for each logger registered with same name
    BOOST_CHECK_EQUAL(logger_b0.name(), "LoggerB-0");
    BOOST_CHECK_EQUAL(logger_b1.name(), "LoggerB-1");

    // Should be equals, as generic name is "LoggerC" and UID increments by 1 for each logger registered with same name
    BOOST_CHECK_EQUAL(logger_c0.name(), "LoggerC-0");
    BOOST_CHECK_EQUAL(logger_c1.name(), "LoggerC-1");
    BOOST_CHECK_EQUAL(logger_c2.name(), "LoggerC-2");
    BOOST_CHECK_EQUAL(logger_c3.name(), "LoggerC-3");
}

BOOST_AUTO_TEST_SUITE_END()

/**
 * updateLoggingLevel() tests
 */

BOOST_AUTO_TEST_SUITE(UpdateLoggingLevel)

BOOST_AUTO_TEST_CASE(LoggersRegisteredAfter) {
    LoggingContext logging_context;

    logging_context.updateLoggingLevel(LogLevel::WARN); // Updating current logging level
    // Registering new loggers into context after update
    const LoggerView logger_a { "LoggerA", logging_context };
    const LoggerView logger_b { "LoggerB", logging_context };

    // Logging level should be set to WARN for all running loggers inside context
    BOOST_CHECK_EQUAL(logger_a.loggingLevel(), LogLevel::WARN);
    BOOST_CHECK_EQUAL(logger_b.loggingLevel(), LogLevel::WARN);
}

BOOST_AUTO_TEST_CASE(LoggersRegisteredBefore) {
    LoggingContext logging_context;

    // Registering new loggers into context before update
    LoggerView logger_a { "LoggerA", logging_context };
    LoggerView logger_b { "LoggerB", logging_context };
    logging_context.updateLoggingLevel(LogLevel::WARN); // Updating current logging level

    // Refresh logging level from context for each API logger
    logger_a.refreshLoggingLevel();
    logger_b.refreshLoggingLevel();

    // Logging level should be set to WARN for all running loggers inside context
    BOOST_CHECK_EQUAL(logger_a.loggingLevel(), LogLevel::WARN);
    BOOST_CHECK_EQUAL(logger_b.loggingLevel(), LogLevel::WARN);
}

BOOST_AUTO_TEST_CASE(LoggersRegisteredBeforeAndAfter) {
    LoggingContext logging_context;

    // Registering new loggers into context before update
    LoggerView logger_a { "LoggerA", logging_context };
    LoggerView logger_b { "LoggerB", logging_context };
    // Updating current logging level
    logging_context.updateLoggingLevel(LogLevel::WARN);
    // Registering new loggers into context after update
    const LoggerView logger_c { "LoggerC", logging_context };
    const LoggerView logger_d { "LoggerD", logging_context };

    // Refresh logging level from context for each API logger created before context level updating
    logger_a.refreshLoggingLevel();
    logger_b.refreshLoggingLevel();

    // Logging level should be set to WARN for all running loggers inside context
    BOOST_CHECK_EQUAL(logger_a.loggingLevel(), LogLevel::WARN);
    BOOST_CHECK_EQUAL(logger_b.loggingLevel(), LogLevel::WARN);
    BOOST_CHECK_EQUAL(logger_c.loggingLevel(), LogLevel::WARN);
    BOOST_CHECK_EQUAL(logger_d.loggingLevel(), LogLevel::WARN);
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * enable/disable() unit tests
 */

BOOST_AUTO_TEST_SUITE(IsEnabled)

BOOST_AUTO_TEST_CASE(Enabled) {
    LoggingContext logging_context;

    // Disable multiple times and re-enable logging
    logging_context.disable();
    logging_context.disable();
    logging_context.disable();
    logging_context.enable();

    // Last method called is enable(), so logging should be enabled
    BOOST_CHECK(logging_context.isEnabled());
}

BOOST_AUTO_TEST_CASE(Disabled) {
    LoggingContext logging_context;

    // Re-nable multiple times and disable logging
    logging_context.enable();
    logging_context.enable();
    logging_context.enable();
    logging_context.disable();

    // Last method called is disable(), so logging should NOT be enabled
    BOOST_CHECK(!logging_context.isEnabled());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()