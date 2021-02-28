#include <RpT-Utils/LoggerView.hpp>

#include <iostream>
#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <RpT-Config/Config.hpp>


// wingdi.h header overrides this enum value, making it unusable into switch cases
#if RPT_RUNTIME_PLATFORM == RPT_RUNTIME_WIN32
#undef ERROR
#endif

namespace RpT::Utils {

// Spdlog loggers record initialization
std::unordered_map<std::string_view, std::size_t> LoggerView::registered_loggers_record_ {};
// Spdlog default log level initialization (not using spdlog::level() static method to provide more flexibility)
spdlog::level::level_enum LoggerView::log_level_ { spdlog::level::info };

void LoggerView::handleError(const std::string& msg) {
    std::cerr << "Logging error: " << msg << std::endl;
}

void LoggerView::updateLogLevel(const LogLevel level) {
    spdlog::level::level_enum spdlog_converted_level;

    // Converts API logging levels to backend logging levels
    switch (level) {
    case LogLevel::TRACE:
        spdlog_converted_level = spdlog::level::trace;
        break;
    case LogLevel::DEBUG:
        spdlog_converted_level = spdlog::level::debug;
        break;
    case LogLevel::INFO:
        spdlog_converted_level = spdlog::level::info;
        break;
    case LogLevel::WARN:
        spdlog_converted_level = spdlog::level::warn;
        break;
    case LogLevel::ERROR:
        spdlog_converted_level = spdlog::level::err;
        break;
    case LogLevel::FATAL:
        spdlog_converted_level = spdlog::level::critical;
        break;
    }

    // Sets new default log level
    log_level_ = spdlog_converted_level;

    // Updates log level for already existing registered loggers

    // For each general purpose
    for (const auto [general_purpose, loggers_count] : registered_loggers_record_) {
        // Necessary for unique name formatting which is using concatenation
        const std::string general_purpose_copy { general_purpose };

        // For each UID inside this general purpose group
        for (std::size_t uid { 0 }; uid < loggers_count; uid++) {
            const std::string unique_name { general_purpose_copy + '-' + std::to_string(uid) };
            const auto current_logger { spdlog::get(unique_name) };

            assert(current_logger); // If loggers count is superior than UID, then this logger must exists
            current_logger->set_level(log_level_); // Update logger log level
        }
    }
}

LoggerView::LoggerView(const std::string_view generic_name) {
    // Creates both console and rotating file sinks
    auto stdout_sink { std::make_shared<spdlog::sinks::stdout_color_sink_st>() };
    auto daily_file_sink {
        std::make_shared<spdlog::sinks::daily_file_sink_st>("logs/rpt-server", 0, 0)
    };

    // If entry doesn't exist, then loggers count is initialized to 0
    // And loggers count is updated into loggers record
    const std::size_t uid { registered_loggers_record_[generic_name]++ };
    // Required for concatenation when creating unique logger name
    const std::string generic_name_copy { generic_name };

    // Instances logger with unique name and two instanced sinks
    this_logger_ = std::make_shared<spdlog::logger>(
            generic_name_copy + '-' + std::to_string(uid),
            spdlog::sinks_init_list { stdout_sink, daily_file_sink });

    // Logger settings
    this_logger_->set_level(log_level_);
    this_logger_->set_pattern("[%T.%e/%^%L%$] %-20n : %v");
    this_logger_->set_error_handler(handleError);

    // As each logger of same purpose has an UID, each logger should has an unique name
    // So there should not be any logger with the same unqiue name, and get() should returns null_ptr
    assert(!spdlog::get(generic_name_copy + '-' + std::to_string(uid)));

    // Registration to logging backed
    spdlog::register_logger(this_logger_);
}

}
