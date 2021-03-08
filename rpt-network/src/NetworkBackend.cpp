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
        throw TooManyArguments { parsed_rptl_command.invokedCommandName() };

    try {
        const std::string actor_uid_copy { getParsedWord(0) }; // Required for conversion to unsigned integer

        /*
         * Critical check :
         *
         * Explicitly asking stoul to converts into uint64_t is not possible, function returns an `unsigned long`,
         * BUT size of unsigned long might be different from 64 bits, so it's necessary to check for return type to
         * be 64 bits unsigned long, else received UID which is 64 bits unsigned long may not be parsed and assigned
         * properly to actor UID.
         */
        static_assert(sizeof(decltype(std::stoul(actor_uid_copy))) == sizeof(std::uint64_t));

        parsed_actor_uid_ = std::stoul(actor_uid_copy);
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
            throw BadClientMessage { "Actor UID \"" + std::to_string(new_actor_uid) + "\" is not available" };

        const auto insertion_result { // Insert new actor's name into registry, copy string from parsed argument
            logged_in_actors_.insert({ new_actor_uid, std::string { handshake_parser.actorName() } })
        };
        // Must be sure that actor has been successfully registered, this is why insertion result is saved
        assert(insertion_result.second);

        // Returns event triggered by actor registration, takes reference to actor's name, no copy done on string
        return Core::JoinedEvent { new_actor_uid, logged_in_actors_.at(new_actor_uid) };
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
            // Logging out actor who sent LOGOUT command
            const std::size_t removed_actors_count { logged_in_actors_.erase(client_actor) };

            assert(removed_actors_count == 1); // Checks for actors being unregistered

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

bool NetworkBackend::isRegistered(const std::uint64_t actor_uid) const {
    return logged_in_actors_.count(actor_uid) == 1;
}


}
