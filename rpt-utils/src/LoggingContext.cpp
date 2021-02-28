#include <RpT-Utils/LoggingContext.hpp>
#include <RpT-Utils/LoggerView.hpp>


namespace RpT::Utils {


LoggingContext::LoggingContext(const LogLevel logging_level)
: logging_level_ { logging_level } {}

std::size_t LoggingContext::newLoggerFor(const std::string_view generic_name, LoggerView& created_logger) {
    // Register logger entry into loggers list, construct in place so only raw reference can be given
    registered_loggers_.emplace_back(created_logger);

    // If none logging backend with general purpose `generic_name` is found, then counter will begins to 0
    // And counter will be retrieved, then incremented
    const std::size_t created_backend_uid { logging_backend_records_[generic_name]++ };

    return created_backend_uid;
}

void LoggingContext::updateLoggingLevel(LogLevel default_logging_level) {
    logging_level_ = default_logging_level; // For later registered LoggerView

    // For currently registered LoggerView
    for (LoggerView& logger_view : registered_loggers_)
        logger_view.refreshLoggingLevel();
}

LogLevel LoggingContext::retrieveLoggingLevel() const {
    return logging_level_;
}


}
