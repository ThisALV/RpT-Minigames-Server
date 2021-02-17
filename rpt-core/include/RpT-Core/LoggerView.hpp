#ifndef RPTOGETHER_SERVER_LOGGERVIEW_HPP
#define RPTOGETHER_SERVER_LOGGERVIEW_HPP

#include <cassert>
#include <string_view>
#include <unordered_map>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

/**
 * @file LoggerView.hpp
 */


/**
 * @brief RpT-Server engine and main loop related components
 */
namespace RpT::Core {

/**
 * @brief Available logging levels to use with `LoggerView::updateLogLevel()`
 *
 * @see LoggerView
 *
 * @author ThisALV, https://github.com/ThisALV
 */
enum struct LogLevel {
    TRACE, DEBUG, INFO, WARN, ERROR, FATAL
};

/**
 * @brief Logging API class which offers access to loggers identified by their generic name and their unique identifier
 *
 * It's important to note that %LoggerView doesn't possesses any logger but rather provide access to one of the loggers
 * registered by the logging backed. Thus, copying a %LoggerView does NOT register a new logger into backend.
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
 * Default log level is INFO, but it can be modified later. Note that log level modification doesn't affect already
 * registered loggers.
 *
 * @note Two loggers of different purposes may have the same unique identifier.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class LoggerView {
private:
    static std::unordered_map<std::string_view, std::size_t> registered_loggers_record_;
    static spdlog::level::level_enum log_level_;

    // Logger pointed by the view
    std::shared_ptr<spdlog::logger> this_logger_;

    /**
     * @brief Logging errors handler
     *
     * Prints error message to `std::cerr`.
     *
     * @param msg Error reason
     */
    static void handleError(const std::string& msg);

public:
    /**
     * @brief Changes default log level and updates log level for all registered loggers
     *
     * @param level New logging level required for a message to be sunk
     */
    static void updateLogLevel(LogLevel level);

    /**
     * @brief Register new logger into backend with given generic name
     *
     * @param generic_name New logger general purpose
     */
    explicit LoggerView(std::string_view generic_name);

    /// Log trace level message
    template<typename... Args>
    void trace(const std::string_view fmt, Args&& ...args) {
        this_logger_->trace(fmt, std::forward<Args>(args)...);
    }

    /// Log debug level message
    template<typename... Args>
    void debug(const std::string_view fmt, Args&& ...args) {
        this_logger_->debug(fmt, std::forward<Args>(args)...);
    }

    /// Log info level message
    template<typename... Args>
    void info(const std::string_view fmt, Args&& ...args) {
        this_logger_->info(fmt, std::forward<Args>(args)...);
    }

    /// Log warn level message
    template<typename... Args>
    void warn(const std::string_view fmt, Args&& ...args) {
        this_logger_->warn(fmt, std::forward<Args>(args)...);
    }

    /// Log error level message
    template<typename... Args>
    void error(const std::string_view fmt, Args&& ...args) {
        this_logger_->error(fmt, std::forward<Args>(args)...);
    }

    /// Log fatal level message
    template<typename... Args>
    void fatal(const std::string_view fmt, Args&& ...args) {
        this_logger_->critical(fmt, std::forward<Args>(args)...);
    }
};

}

#endif //RPTOGETHER_SERVER_LOGGERVIEW_HPP
