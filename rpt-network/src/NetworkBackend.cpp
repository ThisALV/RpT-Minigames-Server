#include <RpT-Network/NetworkBackend.hpp>

#include <algorithm>
#include <cassert>


namespace RpT::Network {


NetworkBackend::RptlCommandParser::RptlCommandParser(const std::string_view rptl_command)
: Utils::TextProtocolParser { rptl_command, 1 } {}

std::string_view NetworkBackend::RptlCommandParser::invokedCommandName() const {
    return getParsedWord(0);
}

std::string_view NetworkBackend::RptlCommandParser::invokedCommandArgs() const {
    return unparsedWords();
}

bool NetworkBackend::RptlCommandParser::isHandshake() const {
    return invokedCommandName() == HANDSHAKE_COMMAND;
}


NetworkBackend::HandshakeParser::HandshakeParser(const NetworkBackend::RptlCommandParser& parsed_rptl_command)
: Utils::TextProtocolParser { parsed_rptl_command.invokedCommandArgs(), 2 } {

    assert(parsed_rptl_command.isHandshake()); // Parsed handshake must be an handshake command

    if (!unparsedWords().empty()) // Checks for syntax, there must NOT be any remaining argument
        throw TooManyArguments { HANDSHAKE_COMMAND };

    try {
        const std::string actor_uid_copy { getParsedWord(0) }; // Required for conversion to unsigned integer

        // `unsigned long long` return type, which is ALWAYS 64 bits large
        // See: https://en.cppreference.com/w/cpp/language/types
        parsed_actor_uid_ = std::stoull(actor_uid_copy);
    } catch (const std::invalid_argument&) { // If parsed actor UID argument isn't a valid unsigned integer
        throw BadClientMessage { "Actor UID must be an unsigned integer of 64 bits" };
    }
}

std::uint64_t NetworkBackend::HandshakeParser::actorUID() const {
    return parsed_actor_uid_;
}

std::string_view NetworkBackend::HandshakeParser::actorName() const {
    return getParsedWord(1);
}


NetworkBackend::ServiceCommandParser::ServiceCommandParser(
        const NetworkBackend::RptlCommandParser& parsed_rptl_command)
        : Utils::TextProtocolParser { parsed_rptl_command.invokedCommandArgs(), 0 } {

    assert(parsed_rptl_command.invokedCommandName() == SERVICE_COMMAND); // Parsed command must be `SERVICE`
}

std::string_view NetworkBackend::ServiceCommandParser::serviceRequest() const {
    return unparsedWords(); // SR command parsing left to SER Protocol
}


Core::JoinedEvent NetworkBackend::handleHandshake(const std::string& client_handshake) {
    try { // Tries to parse received RPTL command, will fail if command is empty
        const RptlCommandParser command_parser { client_handshake };

        if (!command_parser.isHandshake()) // Checks for invoked command, must be handshake command
            throw BadClientMessage { "Invoked command for connection handshaking must be \"HANDSHAKE\"" };

        const HandshakeParser handshake_parser { command_parser };
        const std::uint64_t new_actor_uid { handshake_parser.actorUID() };

        if (isRegistered(new_actor_uid)) // Checks if new actor UID is available
            throw InternalError { "Player UID \"" + std::to_string(new_actor_uid) + "\" is not available" };

        const std::string_view new_actor_name { handshake_parser.actorName() };

        try { // Tries to register actor, implementation registration may fail
            registerActor(new_actor_uid, new_actor_name);

            // If registration hasn't been done at this point, this is an implementation error
            assert(isRegistered(new_actor_uid));
        } catch (const std::exception& err) { // It it fails, then registration must NOT have been done
            // If registration is still active at this point, this is an implementation error and server must stop
            assert(!isRegistered(new_actor_uid));

            // Handshaking is valid, but server is currently unable to register actor
            throw InternalError { err.what() };
        }

        // Returns event triggered by actor registration, takes reference to actor's name, no copy done on string
        return Core::JoinedEvent { new_actor_uid, std::string { new_actor_name }};
    } catch (const Utils::NotEnoughWords&) { // If command is empty, unable to parse invoked command name
        throw EmptyRptlCommand {};
    }
}

Core::AnyInputEvent RpT::Network::NetworkBackend::handleMessage(const std::uint64_t client_actor,
                                                                const std::string& client_message) {

    try { // Tries to parse received RPTL command, will fail if command is empty
        const RptlCommandParser command_parser { client_message };

        const std::string_view invoked_command_name { command_parser.invokedCommandName() };

        // Checks for each available command name if it is invoked by received RPTL message
        if (invoked_command_name == SERVICE_COMMAND) {
            const ServiceCommandParser service_command_parser { command_parser }; // Parse specific SERVICE command

            // Copy required to be moved as string inside emitted event
            std::string sr_command_copy { service_command_parser.serviceRequest() };

            // Returns input event triggered by received Service Request command from given actor with new SR command
            return Core::ServiceRequestEvent { client_actor, std::move(sr_command_copy) };
        } else if (invoked_command_name == LOGOUT_COMMAND) {
            if (!command_parser.invokedCommandArgs().empty()) // If any extra arg detected, command call is ill-formed
                throw TooManyArguments { LOGOUT_COMMAND };

            // If tried to handle unregistered client message as non-handshake message, it is an implementation error
            assert(isRegistered(client_actor));

            unregisterActor(client_actor, {});

            // If actor is still registered, it is an implementation error
            assert(!isRegistered(client_actor));

            // Returns input event triggered by player disconnection (or unregistration)
            // RPTL command way disconnection, clean
            return Core::LeftEvent { client_actor, Core::LeftEvent::Reason::Clean };
        } else { // If none of available commands is being invoked, then invoked command is unknown
            throw BadClientMessage { "Unknown RPTL command: " + std::string { invoked_command_name } };
        }
    } catch (const Utils::NotEnoughWords&) { // If there isn't any word to parse (if command is empty)
        throw EmptyRptlCommand {};
    }
}

void NetworkBackend::pushInputEvent(Core::AnyInputEvent input_event) {
    input_events_queue_.push(std::move(input_event)); // Move triggered input event into queue
}

std::optional<Core::AnyInputEvent> NetworkBackend::pollInputEvent() {
    if (input_events_queue_.empty()) // If events queue is empty, returns uninitialized value
        return {};

    /*
     * Else, moves polled event, removes it from queue and retrieves it
     */

    Core::AnyInputEvent polled_event { std::move(input_events_queue_.front()) };
    input_events_queue_.pop();

    return polled_event;
}

Core::AnyInputEvent NetworkBackend::waitForInput() {
    // Checks for events inside queue before waiting for new input events
    std::optional<Core::AnyInputEvent> last_input_event { pollInputEvent() };
    if (last_input_event.has_value())
        return *last_input_event;

    // If queue is empty, new input event must be waited for by NetworkBackend implementation
    return waitForEvent();
}

void NetworkBackend::closePipelineWith(uint64_t actor, const Utils::HandlingResult& clean_shutdown) {


    // Server must be notified by disconnection, clean if no error occurred, crash otherwise,
    pushInputEvent(Core::LeftEvent {
        actor,
        clean_shutdown ? Core::LeftEvent::Reason::Clean : Core::LeftEvent::Reason::Crash
    });

    // Actor is no longer connected, removes it from register
    unregisterActor(actor, Utils::HandlingResult());
}


}
