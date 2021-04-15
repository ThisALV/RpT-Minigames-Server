#ifndef RPTOGETHER_SERVER_LOGGERVIEW_HPP
#define RPTOGETHER_SERVER_LOGGERVIEW_HPP

#include <functional>
#include <string_view>
#include <spdlog/logger.h>
#include <RpT-Utils/LoggingContext.hpp>

/**
 * @file LoggerView.hpp
 */


namespace RpT::Utils {


/**
 * @brief Logging API class which offers access to loggers identified by their generic name and their unique identifier
 *
 * It's important to note that %LoggerView doesn't possesses any logger but rather provide access to a logging backend.
 * Thus, copying a %LoggerView does NOT creates any new logging backend. Backend currently used is spdlog.
 *
 * %LoggerView generic name corresponds to logger general purpose, for example "Main" or "GameLoader".
 * Unique identifier corresponds to logger identity among loggers of same purpose.
 *
 * The two combined give an unique name with the following format : `${generic_name}-${unique_identifier}`.
 * Each logger will be registered with this unique name into loggers record.
 * This record will keep count how many loggers of given generic name have been registered to generate unique
 * identifier.
 *
 * Log messages follow fmt format specifications and have priority level which is either :
 * trace, debug, info, warn or error. Logging is done by calling appropriate method for each level.
 *
 * Log messages are sunk in both console and daily rotated logging files.
 *
 * Default log level is INFO, but it can be modified later using `LoggingContext::updateLogLevel()`.
 *
 * @note Two loggers of different purposes may have the same unique identifier.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class LoggerView {
private:
    std::reference_wrapper<LoggingContext> context_;

    // Logger pointed by this view
    std::shared_ptr<spdlog::logger> backend_;

    // Converts API logging level enum value to backend logging level enum value
    static constexpr spdlog::level::level_enum apiToBackendLevel(const LogLevel api_logging_level) {
        switch (api_logging_level) {
        case LogLevel::TRACE:
            return spdlog::level::trace;
        case LogLevel::DEBUG:
            return spdlog::level::debug;
        case LogLevel::INFO:
            return spdlog::level::info;
        case LogLevel::WARN:
            return spdlog::level::warn;
        case LogLevel::ERR:
            return spdlog::level::err;
        case LogLevel::FATAL:
            return spdlog::level::critical;
        }
    }

    // Converts backend logging level enum value to API logging level enum value
    static constexpr LogLevel backendToApiLevel(const spdlog::level::level_enum backend_logging_level) {
        switch (backend_logging_level) {
        case spdlog::level::trace:
            return LogLevel::TRACE;
        case spdlog::level::debug:
            return LogLevel::DEBUG;
        case spdlog::level::info:
            return LogLevel::INFO;
        case spdlog::level::warn:
            return LogLevel::WARN;
        case spdlog::level::err:
            return LogLevel::ERR;
        case spdlog::level::critical:
            return LogLevel::FATAL;
        default:
            throw std::invalid_argument { "Unhandled backend logging level \"off\"" };
        }
    }
    
    /**
     * @brief Logging errors handler
     *
     * Prints error message to `std::cerr`.
     *
     * @param msg Error reason
     */
    static void handleError(const std::string& msg);

    /**
     * @brief Logs message if and only if logging is enabled inside current context
     *
     * Backend logging level is updated from current `LoggingContext` each time method is called,
     *
     * @tparam message_level Message priority level
     * @tparam Args Formatter arguments type
     *
     * @param fmt Message format
     * @param args Formatter arguments
     */
    template<LogLevel message_level, typename... Args>
    void log(const std::string_view fmt, Args&& ...args) const {
        refreshLoggingLevel(); // Automatically refresh logging level before

        if (context_.get().isEnabled()) { // Should be logged only if logging is enabled inside this context
            constexpr spdlog::level::level_enum backend_message_level { apiToBackendLevel(message_level) };

            backend_->log(backend_message_level, fmt, std::forward<Args>(args)...);
        }
    }

public:
    /**
     * @brief Register new logger into given context with given generic name
     *
     * @param generic_name New logger general purpose
     * @param context Context in which logger is running, used to determine logging level and backend logger UID
     */
    explicit LoggerView(std::string_view generic_name, LoggingContext& context);

    /**
     * @brief Gets backend logger name
     *
     * @returns Name of backend logger used : `${generic_name}-${uid}`
     */
    const std::string& name() const;

    /**
     * @brief Gets backend logging level
     *
     * @returns Logging level used by backend : will corresponds to `LoggingContext` logging level
     */
    LogLevel loggingLevel() const;

    /**
     * @brief Update backend logger to follow current context logging level
     *
     * @note Automatically called before each message logging.
     */
    void refreshLoggingLevel() const;

    /// Log trace level message
    template<typename... Args>
    void trace(const std::string_view fmt, Args&& ...args) const {
        log<LogLevel::TRACE>(fmt, std::forward<Args>(args)...);
    }

    /// Log debug level message
    template<typename... Args>
    void debug(const std::string_view fmt, Args&& ...args) const {
        log<LogLevel::DEBUG>(fmt, std::forward<Args>(args)...);
    }

    /// Log info level message
    template<typename... Args>
    void info(const std::string_view fmt, Args&& ...args) const {
        log<LogLevel::INFO>(fmt, std::forward<Args>(args)...);
    }

    /// Log warn level message
    template<typename... Args>
    void warn(const std::string_view fmt, Args&& ...args) const {
        log<LogLevel::WARN>(fmt, std::forward<Args>(args)...);
    }

    /// Log error level message
    template<typename... Args>
    void error(const std::string_view fmt, Args&& ...args) const {
        log<LogLevel::ERR>(fmt, std::forward<Args>(args)...);
    }

    /// Log fatal level message
    template<typename... Args>
    void fatal(const std::string_view fmt, Args&& ...args) const {
        log<LogLevel::FATAL>(fmt, std::forward<Args>(args)...);
    }
};


}

#endif //RPTOGETHER_SERVER_LOGGERVIEW_HPP
