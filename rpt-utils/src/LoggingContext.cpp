#include <RpT-Utils/LoggingContext.hpp>


namespace RpT::Utils {


LoggingContext::LoggingContext(const LogLevel logging_level) : logging_level_ { logging_level }, enabled_ { true } {}

std::size_t LoggingContext::newLoggerFor(const std::string_view generic_name) {
    // If none logging backend with general purpose `generic_name` is found, then counter will begins to 0
    // And counter will be retrieved, then incremented
    const std::size_t created_backend_uid { logging_backend_records_[generic_name]++ };

    return created_backend_uid;
}

void LoggingContext::updateLoggingLevel(LogLevel default_logging_level) {
    logging_level_ = default_logging_level; // For later registered LoggerView
}

LogLevel LoggingContext::retrieveLoggingLevel() const {
    return logging_level_;
}

void LoggingContext::enable() {
    enabled_ = true;
}

void LoggingContext::disable() {
    enabled_ = false;
}

bool LoggingContext::isEnabled() const {
    return enabled_;
}


}
