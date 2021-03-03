#include <RpT-Core/Executor.hpp>

#include <type_traits>
#include <RpT-Core/ServiceEventRequestProtocol.hpp>


namespace RpT::Core {

namespace { // Input handler only visible for Executor::run() implementation


/**
 * @brief Provides call operators set, one call operator per InputEvent type
 *
 * Has an access to Executor IO interface, running SER Protocol and Executor's logger
 */
class InputHandler {
private:
    InputOutputInterface& io_interface_;
    ServiceEventRequestProtocol& ser_protocol_;
    Utils::LoggerView& logger_;

public:
    /**
     * @brief Initializes handler to use given IO interface and SER Protocol
     *
     * @param io_interface Input/Output events interface
     * @param ser_protocol Running SER Protocol
     * @param caller_logger Logger used by caller (Executor)
     */
    InputHandler(InputOutputInterface& io_interface, ServiceEventRequestProtocol& ser_protocol,
                 Utils::LoggerView& caller_logger) :

                 io_interface_ { io_interface },
                 ser_protocol_ { ser_protocol },
                 logger_ { caller_logger } {}

    void operator()(const NoneEvent&) {
        logger_.debug("Null event, skipping...");
    }

    void operator()(const StopEvent& event) {
        logger_.debug("Stopping server: caught signal {}", event.caughtSignal());

        io_interface_.close();
    }

    void operator()(const ServiceRequestEvent& event) {
        logger_.debug("Service Request command received from player \"{}\".", event.actor());

        try { // Tries to parse SR command
            // Give SR command to parse and execute by SER Protocol
            const Utils::HandlingResult sr_command_result {
                    ser_protocol_.handleServiceRequest(event.actor(), event.serviceRequest())
            };

            // Replies to actor with command handling result
            io_interface_.replyTo(event, sr_command_result);

            // If command was handled successfully, it must be transmitted across actors
            if (sr_command_result)
                io_interface_.outputRequest(event);
        } catch (const BadServiceRequest& err) { // If command cannot be parsed, replies with error
            io_interface_.replyTo(event, Utils::HandlingResult { err.what() });
        }
    }

    void operator()(const TimerEvent&) {
        logger_.debug("Timer end, continuing...");
    }

    void operator()(const JoinedEvent&) {
        throw std::runtime_error { "JoinedEvent handling not implemented yet." };
    }

    void operator()(const LeftEvent&) {
        throw std::runtime_error { "LeftEvent handling not implemented yet." };
    }
};


}

Executor::Executor(std::vector<boost::filesystem::path> game_resources_path, std::string game_name,
                   InputOutputInterface& io_interface, Utils::LoggingContext& logger_context) :
    logger_context_ { logger_context },
    logger_ { "Executor", logger_context_ },
    io_interface_ { io_interface } {

    logger_.debug("Game name: {}", game_name);

    for (const boost::filesystem::path& resource_path : game_resources_path)
        logger_.debug("Game resources path: {}", resource_path.string());
}

/**
 * @brief TEMPORARY : Chat service which can be toggled on/off with "/toggle" command
 *
 * Used to test efficiency of `InputOutputInterface::replyTo()`.
 */
class ChatService : public Service {
private:
    bool enabled_;

    static constexpr bool isAdmin(const std::uint64_t actor) {
        return actor == 0;
    }

    static constexpr bool isToggleCommand(const std::string_view command) {
        return command == "/toggle";
    }

public:
    explicit ChatService(ServiceContext& run_context) : Service { run_context }, enabled_ { true } {}

    std::string_view name() const override {
        return "Chat";
    }

    Utils::HandlingResult handleRequestCommand(uint64_t actor,
                                               const std::vector<std::string_view>& sr_command_arguments) override {

        if (sr_command_arguments.empty())
            return Utils::HandlingResult { "Message cannot be empty" };

        const std::string_view first_word { sr_command_arguments.at(0) }; // Might be command

        if (isToggleCommand(first_word)) { // If message begins with "/toggle"
            if (sr_command_arguments.size() > 1) // This command hasn't any arguments
                return Utils::HandlingResult { "Invalid arguments for /toggle: command hasn't any args" };

            if (!isAdmin(actor)) // Player using this command should be admin
                return Utils::HandlingResult { "Permission denied: you must be admin to use that command" };

            enabled_ = !enabled_;

            emitEvent("TOGGLED " + std::string { enabled_ ? "true" : "false" });

            return {}; // State was successfully changed
        } else if (enabled_) {
            return {}; // Message should be sent to all players if chat is enabled
        } else {
            return Utils::HandlingResult { "Chat disabled by admin." }; // If it isn't, message can't be sent
        }
    }
};

bool Executor::run() {
    logger_.info("Initializing online services...");

    /*
     * Initializes services and protocol
     */

    ServiceContext ser_protocol_context; // Context in which all online services will be running on
    ChatService chat_svc { ser_protocol_context }; // A test service fot chat feature

    // Protocol initialization with created services
    ServiceEventRequestProtocol ser_protocol {{ chat_svc }, logger_context_ };
    // Functions set for input events handling
    InputHandler input_handler { io_interface_, ser_protocol, logger_ };

    logger_.info("Starts main loop.");

    try { // Any errors occurring during main loop execution will
        while (!io_interface_.closed()) { // Main loop must run as long as inputs and outputs with players can occur
            // Blocking until receiving external event to handle (timer, data packet, etc.)
            const AnyInputEvent input_event { io_interface_.waitForInput() };

            boost::apply_visitor(input_handler, input_event);

            // After input event has been handled, events emitted by services should also be handled in the order they
            // appeared

            logger_.debug("Polling service events...");

            std::optional<std::string> next_svc_event { ser_protocol.pollServiceEvent() }; // Read first event
            while (next_svc_event) { // Then while next event actually exists, handles it
                logger_.debug("Output event: {}", *next_svc_event);
                io_interface_.outputEvent(*next_svc_event); // Sent across actors

                next_svc_event = ser_protocol.pollServiceEvent(); // Read next event
            }

            logger_.debug("Events polled.");
        }

        logger_.info("Stopped.");

        return true;
    } catch (const std::exception& err) {
        logger_.error("Runtime error: {}", err.what());

        return false;
    }
}

}
