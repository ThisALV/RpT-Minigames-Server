#include <RpT-Core/Executor.hpp>

#include <type_traits>
#include <RpT-Core/ServiceEventRequestProtocol.hpp>


namespace RpT::Core {

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
 * @brief Temporary service, used to send back request command with given prefix
 */
class EchoService : public Service {
private:
    const std::string prefix_;

public:
    EchoService(ServiceContext& context, std::string prefix) : Service { context }, prefix_ { std::move(prefix) } {}

    std::string_view name() const override {
        return "Echo";
    }

    Utils::HandlingResult handleRequestCommand(std::string_view,
                                               const std::vector<std::string_view>& sr_command_arguments) override {

        std::string response { prefix_ + " $$" };
        for (const std::string_view command_arg : sr_command_arguments)
            response += ' ' + std::string { command_arg };

        emitEvent(response);

        return {};
    }
};

bool Executor::run() {
    logger_.info("Initializing online services...");

    /*
     * Initializes services and protocol
     */

    ServiceContext ser_protocol_context; // Context in which all online services will be running on
    EchoService echo_svc { ser_protocol_context, "MAIN_LOOP" }; // A test service that basically echo requests

    // Protocol initialization with created services
    ServiceEventRequestProtocol ser_protocol { { echo_svc }, logger_context_ };

    logger_.info("Starts main loop.");

    try { // Any errors occurring during main loop execution will
        while (!io_interface_.closed()) { // Main loop must run as long as inputs and outputs with players can occur
            // Blocking until receiving external event to handle (timer, data packet, etc.)
            const AnyInputEvent input_event { io_interface_.waitForInput() };

            boost::apply_visitor([&](auto&& event) {
                using EventType = std::decay_t<decltype(event)>;

                if constexpr (std::is_same_v<EventType, StopEvent>) {
                    logger_.info("Stopping server: caught signal {}", event.caughtSignal());

                    io_interface_.close();
                } else if constexpr (std::is_same_v<EventType, ServiceRequestEvent>) {
                    logger_.debug("Service Request command received from player \"{}\".", event.actor());

                    try { // Tries to parse SR command
                        // Give SR command to parse and execute by SER Protocol
                        const Utils::HandlingResult sr_command_result {
                                ser_protocol.handleServiceRequest(event.actor(), event.serviceRequest())
                        };

                        // Replies to actor with command handling result
                        io_interface_.replyTo(event, sr_command_result);

                        // If command was handled successfully, it must be transmitted across actors
                        if (sr_command_result)
                            io_interface_.outputRequest(event);
                    } catch (const BadServiceRequest& err) { // If command cannot be parsed, replies with error
                        io_interface_.replyTo(event, Utils::HandlingResult { err.what() });
                    }
                } else {
                    throw std::runtime_error { "Unexpected input event" };
                }
            }, input_event);

            // After input event has been handled, events emitted by services should also be handled in the order they
            // appeared

            logger_.info("Polling service events...");

            std::optional<std::string> next_svc_event { ser_protocol.pollServiceEvent() }; // Read first event
            while (next_svc_event) { // Then while next event actually exists, handles it
                logger_.debug("Output event: {}", *next_svc_event);
                io_interface_.outputEvent(*next_svc_event); // Sent across actors

                next_svc_event = ser_protocol.pollServiceEvent(); // Read next event
            }

            logger_.info("Events polled.");
        }

        logger_.info("Stopped.");

        return true;
    } catch (const std::exception& err) {
        logger_.error("Runtime error: {}", err.what());

        return false;
    }
}

}
