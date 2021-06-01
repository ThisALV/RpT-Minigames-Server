#include <RpT-Utils/LoggerView.hpp>

#include <iostream>
#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <RpT-Config/Config.hpp>


namespace RpT::Utils {


void LoggerView::handleError(const std::string& msg) {
    std::cerr << "Logging error: " << msg << std::endl;
}

LoggerView::LoggerView(const std::string_view generic_name, LoggingContext& context) : context_ { context } {
    // Creates both console and rotating file sinks
    auto stdout_sink { std::make_shared<spdlog::sinks::stdout_color_sink_st>() };
    auto daily_file_sink {
        std::make_shared<spdlog::sinks::daily_file_sink_st>(
                "logs/minigames-server", 0, 0)
    };

    // Configures colored output for console sink
#if RPT_RUNTIME_PLATFORM == RPT_RUNTIME_UNIX
    stdout_sink->set_color(spdlog::level::trace, stdout_sink->black);
#else
    stdout_sink->set_color(spdlog::level::trace, BACKGROUND_BLUE | BACKGROUND_INTENSITY);
#endif

    // Signal backend logger to context and retrieve next available UID
    const std::size_t uid { context_.get().newLoggerFor(generic_name) };
    // Required for concatenation when creating unique logger name
    const std::string generic_name_copy { generic_name };

    // Instances logger with unique name and two instanced sinks
    backend_ = std::make_shared<spdlog::logger>(
            generic_name_copy + '-' + std::to_string(uid),
            spdlog::sinks_init_list { stdout_sink, daily_file_sink });

    // Logger settings
    refreshLoggingLevel();
    backend_->set_pattern("%^[%T.%e/%L] %-15n : %v%$");
    backend_->set_error_handler(handleError);
}

const std::string& LoggerView::name() const {
    return backend_->name();
}

void LoggerView::refreshLoggingLevel() const {
    const spdlog::level::level_enum backend_logging_level {
        apiToBackendLevel(context_.get().retrieveLoggingLevel())
    };

    backend_->set_level(backend_logging_level);
    backend_->flush_on(backend_logging_level);
}

LogLevel LoggerView::loggingLevel() const {
    const LogLevel current_logging_level { backendToApiLevel(backend_->level()) };

    // Must corresponds to context logging level
    assert(current_logging_level == context_.get().retrieveLoggingLevel());

    return current_logging_level;
}


}
