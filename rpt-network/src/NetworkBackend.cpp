#include <RpT-Network/NetworkBackend.hpp>

#include <algorithm>


namespace RpT::Network {


Core::AnyInputEvent RpT::Network::NetworkBackend::handleClientMessage(const std::uint64_t client_actor,
                                                                      const std::string& client_message) {

    // Iterators to begin and end of client message string
    const auto client_message_begin { client_message.cbegin() };
    const auto client_message_end { client_message.cend() };

    // First space char found (or if not, end of string) determines end of RPTL protocol command invoked by client
    const auto rptl_command_end { std::find(client_message_begin, client_message_end, ' ') };
    // Necessary to access client_message first word substring without copying data
    const std::string_view client_message_view { client_message };
    // View to first word inside string, is invoked command
    const std::string rptl_invoked_command {
        client_message.substr(0, rptl_command_end - client_message_begin)
    };

    // Testing invoked command for each available RPTL command
    if (rptl_invoked_command == LOGIN_COMMAND) {
        // Given actor UID is ignored as it is not logged in yet
        const std::uint64_t new_client_actor { uid_count_++ };

        // Arguments which aren't parsed for now, from RPTL command end to string end
        const std::string_view not_evaluated_args {
            client_message_view.substr(rptl_command_end - client_message_begin)
        };

        if (not_evaluated_args.empty()) // If there isn't any argument, then name is missing
            throw BadClientMessage { "Expected new actor's name" };

        const std::string_view name_arg { // Name argument, begins after separator between LOGIN command and arguments
            not_evaluated_args.substr(1)
        };

        if (name_arg.empty()) // Name must NOT be empty
            throw BadClientMessage { "Actor's name must NOT be empty" };

        // Logged in, must be registered with name
        logged_in_actors_.insert({ new_client_actor, std::string { name_arg } });

        return Core::JoinedEvent { client_actor };
    } else if (rptl_invoked_command == LOGOUT_COMMAND) {
        // Logged out, must be removed from known client actors UID
        logged_in_actors_.erase(client_actor);

        // Clean disconnection requested by client
        return Core::LeftEvent { client_actor, Core::LeftEvent::Reason::Clean };
    } else { // If command not found, parsed as Service Request command
        return Core::ServiceRequestEvent { client_actor, client_message };
    }
}


}
