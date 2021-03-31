#include <cassert>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <RpT-Config/Config.hpp>
#include <RpT-Core/Executor.hpp>
#include <RpT-Network/UnsafeBeastWebsocketBackend.hpp>
#include <RpT-Utils/CommandLineOptionsParser.hpp>
#include <RpT-Network/SafeBeastWebsocketBackend.hpp>


constexpr int SUCCESS { 0 };
constexpr int INVALID_ARGS { 1 };
constexpr int RUNTIME_ERROR { 2 };

constexpr std::uint16_t DEFAULT_PORT { 35555 };


/**
 * @brief Parses log level string and converts to enum value.
 *
 * @param level String or first char of log level
 *
 * @throws std::invalid_argument If level string cannot be parsed into LogLevel value
 *
 * @return Corresponding `RpT::Core::LogLevel` enum value
 */
constexpr RpT::Utils::LogLevel parseLogLevel(const std::string_view level) {
    if (level == "t" || level == "trace")
        return RpT::Utils::LogLevel::TRACE;
    else if (level == "d" || level == "debug")
        return RpT::Utils::LogLevel::DEBUG;
    else if (level == "i" || level == "info")
        return RpT::Utils::LogLevel::INFO;
    else if (level == "w" || level == "warn")
        return RpT::Utils::LogLevel::WARN;
    else if (level == "e" || level == "error")
        return RpT::Utils::LogLevel::ERR;
    else if (level == "f" || level == "fatal")
        return RpT::Utils::LogLevel::FATAL;
    else
        throw std::invalid_argument { "Unable to parse level \"" + std::string { level }+ "\"" };
}

int main(const int argc, const char** argv) {
    RpT::Utils::LoggingContext server_logging;
    RpT::Utils::LoggerView logger { "Main", server_logging };

    try {
        // Read and parse command line options
        const RpT::Utils::CommandLineOptionsParser cmd_line_options {
            argc, argv, { "game", "log-level", "testing", "ip", "net-backend", "crt", "privkey" }
        };

        // Get game name from command line options
        const std::string_view game_name { cmd_line_options.get("game") };

        // Try to get and parse logging level from command line options
        if (cmd_line_options.has("log-level")) { // Only if option is enabled
            try {
                // Get option value from command line
                const std::string_view log_level_argument { cmd_line_options.get("log-level") };
                // Parse option value
                const RpT::Utils::LogLevel parsed_log_level { parseLogLevel(log_level_argument) };

                server_logging.updateLoggingLevel(parsed_log_level);
                logger.debug("Logging level set to \"{}\".", log_level_argument);
            } catch (const std::logic_error& err) { // Option value may be missing, or parse may fail
                logger.error("Log-level parsing: {}", err.what());
                logger.warn("log-level option has been ignored, \"info\" will be used.");
            }
        }

        std::uint16_t server_local_port;
        // Try to get and parse server local port from command line options
        if (cmd_line_options.has("port")) {
            // String copy must be created anyway to use stoull function
            const std::string port_argument { cmd_line_options.get("port") };
            const std::uint64_t parsed_port { std::stoull(port_argument) };

            if (parsed_port > std::numeric_limits<std::uint16_t>::max())
                throw RpT::Utils::OptionsError { "port argument must be included inside 0..65535" };

            logger.debug("Switch listening port to {}", parsed_port);

            server_local_port = parsed_port;
        } else { // If option isn't specified, use default value
            logger.debug("Keeps default listening port {}", DEFAULT_PORT);

            server_local_port = DEFAULT_PORT;
        }

        // Default local protocol is IPv4
        boost::asio::ip::tcp server_local_protocol { boost::asio::ip::tcp::v4() };
        // Try to get and parse provided custom protocol from command line options
        if (cmd_line_options.has("ip")) {
            const std::string_view mode { cmd_line_options.get("ip") };

            // Must be IPv4 or IPv6 mode
            if (mode == "v4") { // Nothing to, as default is already IPv4
                logger.debug("Keeps mode IPv4, as specified by command line options");
            } else if (mode == "v6") {
                logger.debug("Switch to IPv6 mode, as specified by command line options");

                server_local_protocol = boost::asio::ip::tcp::v6();
            } else {
                throw RpT::Utils::OptionsError { "Unknown IP protocol: " + std::string { mode } };
            }
        } else {
            logger.debug("Keeps mode IPv4, no command line options \"ip\"");
        }

        logger.info("Running RpT server {} on {}.", RpT::Config::VERSION, RpT::Config::runtimePlatformName());

        std::vector<boost::filesystem::path> game_resources_path;
        // At max 3 paths : next to server executable, into user directory and into /usr/share for Unix platform
        game_resources_path.reserve(3);

        boost::filesystem::path local_path { ".rpt-server" };
        boost::filesystem::path user_path; // Platform dependent

        // Get user home directory depending of the current runtime platform
        if constexpr (RpT::Config::isUnixBuild()) {
            user_path = std::getenv("HOME");
        } else {
            user_path = std::getenv("UserProfile");
        }

        user_path /= ".rpt-server"; // Search for .rpt-server hidden subdirectory inside user directory

        if constexpr (RpT::Config::isUnixBuild()) { // Unix systems may have /usr/share directory for programs data
            boost::filesystem::path system_path { "/usr/share/rpt-server" };

            game_resources_path.push_back(std::move(system_path)); // Add Unix /usr/share directory
        }

        // Add path for subdirectory inside working directory and inside user directory
        game_resources_path.push_back(std::move(user_path));
        game_resources_path.push_back(std::move(local_path));

        // Selected backend for IO interface, defaults to WSS (Safe Websocket)
        std::string_view selected_network_bakcend { "wss" };
        // If backend is supplied by command line options, then override default behavior
        if (cmd_line_options.has("net-backend"))
            selected_network_bakcend = cmd_line_options.get("net-backend");

        // Dynamic selection from command line options, requires dynamic allocation
        std::unique_ptr<RpT::Network::NetworkBackend> network_backend;
        // Local server endpoint evaluated from configurable port and IP protocol version
        const boost::asio::ip::tcp::endpoint server_local_endpoint { server_local_protocol, server_local_port };

        if (selected_network_bakcend == "wss") { // Websockets switched from HTTPS
            logger.debug("Using Secure Websocket backend for IO interface.");

            // Retrieves and copies options from command line
            const std::string certificate_option { cmd_line_options.get("crt") };
            const std::string private_key_option { cmd_line_options.get("privkey") };

            // Get and parse valid paths from appropriate command line options
            const boost::filesystem::path certificate_path { certificate_option };
            const boost::filesystem::path private_path { private_key_option };

            // Checks for both path validity and file type

            if (!boost::filesystem::is_regular_file(certificate_path))
                throw RpT::Utils::OptionsError { "Given certificate path isn't a valid path to regular file" };

            if (!boost::filesystem::is_regular_file(private_path))
                throw RpT::Utils::OptionsError { "Given private key path isn't a valid path to reguler file" };

            // If both paths are valid, uses them to build backend with appropriate TLS features configuration
            network_backend = std::make_unique<RpT::Network::SafeBeastWebsocketBackend>(
                    certificate_option, private_key_option, server_local_endpoint, server_logging);
        } else if (selected_network_bakcend == "unsafe-ws") { // Websockets switched from HTTP
            logger.debug("Using NON-Secure Websocket backend for IO interface.");

            network_backend = std::make_unique<RpT::Network::UnsafeBeastWebsocketBackend>(
                    server_local_endpoint, server_logging);
        } else { // Unknown network backend
            const std::string backend_copy { selected_network_bakcend }; // Copy required for string concat

            throw RpT::Utils::OptionsError { "Unknown networking backend " + backend_copy };
        }

        /*
         * Create executor with listed resources paths, game name argument and run main loop with dynamically
         * initialized NetworkBackend implementation
         */

        // For testing if executable is properly launched inside continuous integration, main loop must not continue
        if (cmd_line_options.has("testing")) {
            logger.info("Testing mode for CI, server will be immediately closed.");

            network_backend->close();
        }

        RpT::Core::Executor rpt_executor {
            std::move(game_resources_path), std::string { game_name }, *network_backend, server_logging
        };

        const bool done_successfully { rpt_executor.run() };

        // Process exit code depends on main loop result
        if (done_successfully) {
            logger.info("Successfully shut down.");

            return SUCCESS;
        } else {
            logger.fatal("Shut down for unhandled error.");

            return RUNTIME_ERROR;
        }
    } catch (const RpT::Utils::OptionsError& err) {
        logger.fatal("Command line error: {}", err.what());

        return INVALID_ARGS;
    } catch (const std::exception& err) {
        logger.fatal("Unhandled runtime error: {}", err.what());

        return RUNTIME_ERROR;
    }
}
