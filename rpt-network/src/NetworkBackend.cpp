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
        throw BadClientMessage { "Too many arguments given to handshaking RPTL command" };

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


Core::JoinedEvent NetworkBackend::handleHandshake(const std::string& client_handshake) {
    try { // Tries to parse received RPTL command, will fail if command is empty
        const RptlCommandParser command_parser { client_handshake };

        if (!command_parser.isHandshake()) // Checks for invoked command, must be handshake command
            throw BadClientMessage { "Invoked command for connection handshaking must be \"HANDSHAKE\"" };

        const HandshakeParser handshake_parser { command_parser };
        const std::uint64_t new_actor_uid { handshake_parser.actorUID() };

        if (isRegistered(new_actor_uid)) // Checks if new actor UID is available
            throw BadClientMessage { "Actor UID \"" + std::to_string(new_actor_uid) + "\" is not available" };

        // Must be copied to be registered into logged in actors
        std::string new_actor_name { handshake_parser.actorName() };

        const auto insertion_result { logged_in_actors_.insert({ new_actor_uid, std::move(new_actor_name) }) };
        // Must be sure that actor has been successfully registered, this is why insertion result is saved
        assert(insertion_result.second);

        return Core::JoinedEvent { new_actor_uid }; // Returns event triggered by actor registration
    } catch (const Utils::NotEnoughWords&) { // If command is empty, unable to parse invoked command name
        throw BadClientMessage { "RPTL command must NOT be empty" };
    }
}

Core::AnyInputEvent RpT::Network::NetworkBackend::handleMessage(const std::uint64_t client_actor,
                                                                const std::string& client_message) {

    // Iterators to begin and end of client message string
    const auto client_message_begin { client_message.cbegin() };
    const auto client_message_end { client_message.cend() };

    // First space char found (or if not, end of string) determines end of RPTL protocol command invoked by client
    const auto rptl_command_end { std::find(client_message_begin, client_message_end, ' ') };
    // Necessary to access client_message first word substring without copying data
    const std::string_view client_message_view { client_message };
    // View to first word inside string, is invoked command
    const std::string_view rptl_invoked_command {
        client_message_view.substr(0, rptl_command_end - client_message_begin)
    };

    // Testing invoked command for each available RPTL command
    if (rptl_invoked_command == SERVICE_COMMAND) {
        // Part of command string which have NOT been parsed yet
        const std::string_view not_evaluated_chars {
                client_message_view.substr(rptl_command_end - client_message_begin)
        };

        // If there isn't any argument, then Service Request command contained inside RPTL command is ill-formed
        if (not_evaluated_chars.empty())
            throw BadClientMessage { "Expected SR command as argument for RPTL command \"SERVICE\"" };

        const std::string_view sr_command { // SR command argument, begins after separator following RPTL command
                not_evaluated_chars.substr(1)
        };

        std::string sr_command_copy { sr_command }; // Required to pass received SR command into input event

        // Ready to trigger ServiceRequest input event with given Service Request command
        return Core::ServiceRequestEvent { client_actor, std::move(sr_command_copy) };
    } else if (rptl_invoked_command == LOGOUT_COMMAND) {
        // Logged out, must be removed from known client actors UID
        logged_in_actors_.erase(client_actor);

        // Clean disconnection requested by client
        return Core::LeftEvent { client_actor, Core::LeftEvent::Reason::Clean };
    } else { // If invoked command doesn't exist, then client RPTL message is ill-formed
        const std::string invoked_command_copy { rptl_invoked_command }; // Required for error message concatenation

        throw BadClientMessage { "This RPTL command doesn't exist: " + invoked_command_copy };
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
