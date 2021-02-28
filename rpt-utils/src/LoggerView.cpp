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
        std::make_shared<spdlog::sinks::daily_file_sink_st>("logs/rpt-server", 0, 0)
    };

    // Signal backend logger to context and retrieve next available UID
    const std::size_t uid { context_.get().newLoggerFor(generic_name, *this) };
    // Required for concatenation when creating unique logger name
    const std::string generic_name_copy { generic_name };

    // Instances logger with unique name and two instanced sinks
    backend_ = std::make_shared<spdlog::logger>(
            generic_name_copy + '-' + std::to_string(uid),
            spdlog::sinks_init_list { stdout_sink, daily_file_sink });

    // Logger settings
    refreshLoggingLevel();
    backend_->set_pattern("[%T.%e/%^%L%$] %-20n : %v");
    backend_->set_error_handler(handleError);

    // As each logger of same purpose has an UID, each logger should has an unique name
    // So there should not be any logger with the same unqiue name, and get() should returns null_ptr
    assert(!spdlog::get(generic_name_copy + '-' + std::to_string(uid)));

    // Registration to logging backed
    spdlog::register_logger(backend_);
}

const std::string& LoggerView::name() const {
    return backend_->name();
}

void LoggerView::refreshLoggingLevel() {
    backend_->set_level(apiToBackendLevel(context_.get().retrieveLoggingLevel()));
}


}
