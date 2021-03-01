#ifndef RPTOGETHER_SERVER_LOGGINGCONTEXT_HPP
#define RPTOGETHER_SERVER_LOGGINGCONTEXT_HPP

#include <functional>
#include <string_view>
#include <unordered_map>
#include <vector>

/**
 * @file LoggingContext.hpp
 */


namespace RpT::Utils {


class LoggerView; // Need to be referenced by LoggingContext when a LoggerView is regietered


// wingdi.h header overrides this enum value, making it unusable inside switch cases
#if RPT_RUNTIME_PLATFORM == RPT_RUNTIME_WIN32
#undef ERROR
#endif

/**
 * @brief Available logging levels to use with `LoggingContext::updateLogLevel()`
 *
 * @see LoggingContext
 *
 * @author ThisALV, https://github.com/ThisALV
 */
enum struct LogLevel {
    TRACE, DEBUG, INFO, WARN, ERROR, FATAL
};

/**
 * @brief Provides context for multiple LoggerView management
 *
 * This context keep count for all LoggerView that are registered in it, so it can be used to determine logger UID
 * for unique name, and keep trace for last assigned default logging level.
 *
 * LoggerView should keep reference to it's assigned context, so it can be refreshed and check for newly assigned
 * default logging level.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class LoggingContext {
private:
    // Count for each logging backend created, classed by general purpose
    std::unordered_map<std::string_view, std::size_t> logging_backend_records_;
    // Logging level for registered loggers
    LogLevel logging_level_;
    // Loggers that will be notified for logging level changes
    std::vector<std::reference_wrapper<LoggerView>> registered_loggers_;

public:
    /**
     * @brief Constructs new logging context with empty logging backend records and given default logging level
     *
     * @param logging_level Logging level default used by LoggerView referencing this context
     */
    explicit LoggingContext(LogLevel logging_level = LogLevel::INFO);

    /*
     * Entity class semantic
     */

    LoggingContext(const LoggingContext&) = delete;
    LoggingContext& operator=(const LoggingContext&) = delete;

    bool operator==(const LoggingContext&) const = delete;

    /**
     * @brief Increments loggers count for given general purpose and retrieve next logger expected UID for this
     * general purpose
     *
     * @param generic_name General purpose for created logger
     * @param created_logger Logger to register, logging
     *
     * @returns Expected UID for created logger
     */
    std::size_t newLoggerFor(std::string_view generic_name, LoggerView& created_logger);

    /**
     * @brief Update default logging level for later created and currently running loggers backend
     *
     * @param default_logging_level New logging level applied
     */
    void updateLoggingLevel(LogLevel default_logging_level);

    /**
     * @brief Retrieve current default logging level
     *
     * @returns Currently set default logging level
     */
    LogLevel retrieveLoggingLevel() const;
};


}


#endif //RPTOGETHER_SERVER_LOGGINGCONTEXT_HPP
