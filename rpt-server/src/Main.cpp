#include <cassert>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <RpT-Config/Config.hpp>
#include <RpT-Core/Executor.hpp>
#include <RpT-Network/NetworkBackend.hpp>
#include <RpT-Utils/CommandLineOptionsParser.hpp>


/**
 * @brief Tests purposes networking backend, using given input stream as source for RPTL command and given output to
 * prompt commands
 *
 * Dictionary associating actors UID with theirs respective name is used as actors registry.
 */
class ConsoleIO : public RpT::Network::NetworkBackend {
private:
    // Console actor (UID 0 for chat admin)
    static constexpr std::uint64_t CONSOLE_ACTOR_UID { 0 };

    /// Thrown if command line syntax is invalid for ConsoleIO
    class BadConsoleInput : public std::logic_error {
    public:
        explicit BadConsoleInput(const std::string& reason) : std::logic_error { reason } {};
    };

    /// Parser for input stream (console) commands, gets command actor UID with 1st argument (must be UID or *)
    class ConsoleInputParser : public RpT::Utils::TextProtocolParser {
    private:
        std::optional<std::uint64_t> client_uid_; // Uninitialized of not registered

    public:
        /// Throws `BadConsoleInput` if doesn't begin with UID or *
        ConsoleInputParser(const std::string_view input_line)
                : RpT::Utils::TextProtocolParser { input_line, 1 } {
            const std::string_view client_uid_arg { getParsedWord(0) };

            if (client_uid_arg != "*") {
                try {
                    client_uid_ = std::stoull(std::string { client_uid_arg });
                } catch (const std::logic_error&) {
                    throw BadConsoleInput { "Client UID isn't an unsigned integer of 64bits" };
                }
            }
        }

        /// Checks if client sending RPTL command is registered or not
        bool isRegisteredClient() const {
            return client_uid_.has_value();
        }

        /// Retrieves UID for registered client sending RPTL command
        std::uint64_t clientUID() const {
            return *client_uid_;
        }

        /// Retrieves sent client message (as copied string, handle*() NetworkBackend takes const string refs)
        std::string clientMessage() const {
            return std::string { unparsedWords() };
        }
    };

    RpT::Utils::LoggerView logger_;
    std::istream& input_;
    std::ostream& output_;
    std::unordered_map<std::uint64_t, std::string> actors_registry_; // UID for each actor name

protected:
    /// Pushes UID / name pair into dictionary
    void registerActor(const std::uint64_t uid, std::string name) override {
        const auto insertion_result { actors_registry_.insert({ uid, std::move(name) }) };
        assert(insertion_result.second); // Checks for insert operation result
    }

    /// Removes UID from dictionary, disabling Console actor registration
    void unregisterActor(const std::uint64_t uid, const RpT::Utils::HandlingResult&) override {
        const std::size_t removed_count { actors_registry_.erase(uid) };
        assert(removed_count == 1); // Checks for erase operation result

        // Automatically register Console actor if unregistered during client RPTL message handling
        if (!isRegistered(CONSOLE_ACTOR_UID)) {
            initConsole();
            pushInputEvent(RpT::Core::JoinedEvent { CONSOLE_ACTOR_UID, "Console" });
        }
    }

    /// Checks for UID key inside dictionary
    bool isRegistered(const std::uint64_t actor_uid) const override {
        return actors_registry_.count(actor_uid) == 1;
    }

public:
    /// Initializes logger, input/output stream references
    explicit ConsoleIO(RpT::Utils::LoggingContext& logger_context, std::istream& input, std::ostream& output)
    : logger_ { "IO-Events", logger_context }, input_ { input }, output_ { output } {}

    /// Register default Console actor with UID 0
    void initConsole() {
        registerActor(CONSOLE_ACTOR_UID, "Console");
    }

    /// Reads next command from input stream (console). Depending on current actor UID, command will be treated as
    /// client message or client handshake for registration.
    RpT::Core::AnyInputEvent waitForEvent() override {
        std::string rptl_command;
        output_ << "> ";
        std::getline(input_, rptl_command); // Read RPTL command from input stream

        if (!input_) // If input stream is closed, no more events will be emitted, server can stop
            return RpT::Core::StopEvent { CONSOLE_ACTOR_UID, 0 };

        std::optional<RpT::Core::AnyInputEvent> event_from_client; // Variant not default constructible in that case
        try { // Tries to parse console line input into RPTL command for specified actor, if any
            const ConsoleInputParser console_parser { rptl_command };

            // RPTL command handling depends on actor which is sending it
            if (console_parser.isRegisteredClient()) { // Specified actor UID
                const std::uint64_t actor_client_uid { console_parser.clientUID() };

                if (!isRegistered(actor_client_uid)) // Checks for current actor client registration
                    throw BadConsoleInput { "Actor client " + std::to_string(actor_client_uid) + " unknown" };

                try { // Tries to handle message, disconnect actor if it fails
                    event_from_client = handleMessage(actor_client_uid, console_parser.clientMessage());
                } catch (const RpT::Network::BadClientMessage& err) { // Message handling failed, client left event
                    closePipelineWith(actor_client_uid,
                                      RpT::Utils::HandlingResult { err.what() });

                    // Unable to get event from RPTL command, console input is not valid
                    throw;
                }
            } else { // "*" actor UID, for unregistered actor
                event_from_client = handleHandshake(console_parser.clientMessage());
            }
        } catch (const std::logic_error& err) { // Console line input parsing failed
            logger_.error("Unable to get event from console input: {}", err.what());
            event_from_client = RpT::Core::NoneEvent { CONSOLE_ACTOR_UID }; // Console input invalid, skip event waiting
        }

        assert(event_from_client.has_value()); // Must have input event to return
        return *event_from_client;
    }

    /// Logs SR Responses into backend logger
    void replyTo(const std::uint64_t sr_actor, const std::string& sr_response) override {
        logger_.info("Reply to {} command: {}", sr_actor, sr_response);
    }

    /// Logs SE into backend logger
    void outputEvent(const std::string& event) override {
        logger_.info("Event triggered: \"{}\"", event);
    }
};


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
        return RpT::Utils::LogLevel::ERROR;
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
            argc, argv, { "game", "log-level", "testing" }
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

        /*
         * Create executor with listed resources paths, game name argument and run main loop with simple IO interface
         * required to build
         */

        ConsoleIO io { server_logging, std::cin, std::cout };
        io.initConsole(); // Required for Console actor to exist

        // For testing if executable is properly launched inside continuous integration, main loop must not continue
        if (cmd_line_options.has("testing")) {
            logger.info("Testing mode for CI, server will be immediately closed.");

            io.close();
        }

        RpT::Core::Executor rpt_executor { std::move(game_resources_path), std::string { game_name }, io, server_logging };
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
    }
}
