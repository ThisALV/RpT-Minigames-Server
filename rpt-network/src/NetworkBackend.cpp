#include <RpT-Network/NetworkBackend.hpp>

#include <algorithm>


namespace RpT::Network {


Core::JoinedEvent NetworkBackend::handleHandshake(const std::string& client_handshake) {
    // Necessary to access client_handshake first word substring without copying data
    const std::string_view client_handshake_view { client_handshake };

    // Iterators to begin and end of client message string
    const auto client_handshake_begin { client_handshake_view.cbegin() };
    const auto client_handshake_end { client_handshake_view.cend() };

    // First space char found (or if not, end of string) determines end of RPTL protocol command invoked by client
    const auto rptl_command_end { std::find(client_handshake_begin, client_handshake_end, ' ') };
    // View to first word inside string, is invoked command
    const std::string_view rptl_invoked_command {
            client_handshake_view.substr(0, rptl_command_end - client_handshake_begin)
    };

    if (rptl_invoked_command != HANDSHAKE_COMMAND) // Handshake stage, only LOGIN command is accepted
        throw BadClientMessage { "Message is handshake as actor isn't registered yet, command \"LOGIN\" expected" };

    // Part of command string which have NOT been parsed yet
    const std::string_view not_evaluated_chars {
            client_handshake_view.substr(rptl_command_end - client_handshake_end)
    };

    // If there isn't any argument, then arguments following RPTL handshaking command are ill-formed
    if (not_evaluated_chars.empty())
        throw BadClientMessage { "Expected at least name argument for RPTL command \"LOGIN\"" };

    // Find next argument end, which is either ' ' or string end
    const auto name_argument_end { // Search begins after RPTL command separator
        std::find(not_evaluated_chars.cbegin() + 1, not_evaluated_chars.cend(), ' ')
    };

    // Name argument position, one separator after RPTL command
    const auto name_argument_i { rptl_command_end - client_handshake_begin + 1 };
    // Name argument length = index for name argument end - index for name argument begin
    const auto name_argument_len { (name_argument_end - client_handshake_begin) - name_argument_i };
    const std::string_view name_argument { // Substring beginning after RPTL command separator, ending at arg's end
        client_handshake_view.substr(name_argument_i, name_argument_len)
    };

    if (name_argument.empty()) // Name must NOT be empty
        throw BadClientMessage { "Actor name must NOT be empty" };

    // Following arguments are ignored for now. TODO: read optional UID argument (when Protocol class released)

    // New actor UID generated from UIDs counter, which is incremented
    const std::uint64_t new_client_actor { uid_count_ };
    // Required to be registered with corresponding UID
    std::string name_copy { name_argument };

    // Logged in, must be registered with name
    logged_in_actors_.insert({ new_client_actor, std::move(name_copy) });

    // Returns ready to be triggered input event
    return Core::JoinedEvent { new_client_actor };
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


}
