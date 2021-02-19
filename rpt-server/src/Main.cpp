#include <cstdlib>
#include <RpT-Config/Config.hpp>
#include <RpT-Core/Executor.hpp>
#include <RpT-Utils/CommandLineOptionsParser.hpp>


constexpr int SUCCESS { 0 };
constexpr int INVALID_ARGS { 1 };
constexpr int RUNTIME_ERROR { 2 };


/**
 * @brief Parses log level string and converts to enum value.
 *
 * @param level String or first char of log level
 *
 * @throws std::invalid_argument If level string cannot be parsed into LogLevel value
 *
 * @return Corresponding `RpT::Core::LogLevel` enum value
 */
constexpr RpT::Core::LogLevel parseLogLevel(const std::string_view level) {
    // Initialization is necessary for constexpr function until C++20
    RpT::Core::LogLevel parsed_level { RpT::Core::LogLevel::INFO };

    if (level == "t" || level == "trace")
        parsed_level = RpT::Core::LogLevel::TRACE;
    else if (level == "d" || level == "debug")
        parsed_level = RpT::Core::LogLevel::DEBUG;
    else if (level == "i" || level == "info")
        parsed_level = RpT::Core::LogLevel::INFO;
    else if (level == "w" || level == "warn")
        parsed_level = RpT::Core::LogLevel::WARN;
    else if (level == "e" || level == "error")
        parsed_level = RpT::Core::LogLevel::ERROR;
    else if (level == "f" || level == "fatal")
        parsed_level = RpT::Core::LogLevel::FATAL;
    else
        throw std::invalid_argument { "Unable to parse level \"" + std::string { level }+ "\"" };

    return parsed_level;
}

int main(const int argc, const char** argv) {
    RpT::Core::LoggerView logger { "Main" };

    try {
        // Read and parse command line options
        const RpT::Utils::CommandLineOptionsParser cmd_line_options { argc, argv, { "game", "log-level" } };

        // Get game name from command line options
        const std::string_view game_name { cmd_line_options.get("game") };

        // Try to get and parse logging level from command line options
        if (cmd_line_options.has("log-level")) { // Only if option is enabled
            try {
                // Get option value from command line
                const std::string_view log_level_argument { cmd_line_options.get("log-level") };
                // Parse option value
                const RpT::Core::LogLevel parsed_log_level { parseLogLevel(log_level_argument) };

                RpT::Core::LoggerView::updateLogLevel(parsed_log_level);
                logger.debug("Logging level set to \"{}\".", log_level_argument);
            } catch (const std::logic_error& err) { // Option value may be missing, or parse may fail
                logger.error("Log-level parsing: {}", err.what());
                logger.warn("log-level option has been ignored, \"info\" will be used.");
            }
        }

        logger.info("Running RpT server {} on {}.", RpT::Config::VERSION, RpT::Config::runtimePlatformName());

        std::vector<supported_fs::path> game_resources_path;
        // At max 3 paths : next to server executable, into user directory and into /usr/share for Unix platform
        game_resources_path.reserve(3);

        supported_fs::path local_path { ".rpt-server" };
        supported_fs::path user_path; // Platform dependent

        // Get user home directory depending of the current runtime platform
        if constexpr (RpT::Config::isUnixBuild()) {
            user_path = std::getenv("HOME");
        } else {
            user_path = std::getenv("UserProfile");
        }

        user_path /= ".rpt-server"; // Search for .rpt-server hidden subdirectory inside user directory

        if constexpr (RpT::Config::isUnixBuild()) { // Unix systems may have /usr/share directory for programs data
            supported_fs::path system_path { "/usr/share/rpt-server" };

            game_resources_path.push_back(std::move(system_path)); // Add Unix /usr/share directory
        }

        // Add path for subdirectory inside working directory and inside user directory
        game_resources_path.push_back(std::move(user_path));
        game_resources_path.push_back(std::move(local_path));

        // Create executor with listed resources paths, game name argument and run main loop
        RpT::Core::Executor rpt_executor { std::move(game_resources_path), std::string { game_name } };
        const bool done_successfully { rpt_executor.run() };

        return done_successfully ? SUCCESS : RUNTIME_ERROR; // Process exit code depends on main loop result
    } catch (const RpT::Utils::OptionsError& err) {
        logger.fatal("Command line error: {}", err.what());

        return INVALID_ARGS;
    }
}
