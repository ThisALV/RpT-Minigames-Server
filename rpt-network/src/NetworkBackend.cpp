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


Core::JoinedEvent NetworkBackend::handleHandshake(const std::uint64_t client_token,
                                                  const std::string& message_handshake) {

    try { // Tries to parse received RPTL command, will fail if command is empty
        const RptlCommandParser command_parser { message_handshake };

        if (!command_parser.isHandshake()) // Checks for invoked command, must be handshake command
            throw BadClientMessage { "Invoked command for connection handshaking must be \"HANDSHAKE\"" };

        const HandshakeParser handshake_parser { command_parser };
        const std::uint64_t new_actor_uid { handshake_parser.actorUID() };

        if (isRegistered(new_actor_uid)) // Checks if new actor UID is available
            throw InternalError { "Player UID \"" + std::to_string(new_actor_uid) + "\" is not available" };

        std::string new_actor_name { handshake_parser.actorName() };

        try { // Tries to register actor, implementation registration may fail
            registerActor(client_token, new_actor_uid, new_actor_name);

            // If registration hasn't been done at this point, this is an implementation error
            assert(isRegistered(new_actor_uid));
        } catch (const std::exception& err) { // It it fails, then registration must NOT have been done
            // If registration is still active at this point, this is an implementation error and server must stop
            assert(!isRegistered(new_actor_uid));

            // Handshaking is valid, but server is currently unable to register actor
            throw InternalError { err.what() };
        }

        // Returns event triggered by actor registration, takes reference to actor's name, no copy done on string
        return Core::JoinedEvent { new_actor_uid, std::move(new_actor_name) };
    } catch (const Utils::NotEnoughWords&) { // If command is empty, unable to parse invoked command name
        throw EmptyRptlCommand {};
    }
}

Core::AnyInputEvent RpT::Network::NetworkBackend::handleRegular(const std::uint64_t client_actor,
                                                                const std::string& regular_message) {

    try { // Tries to parse received RPTL command, will fail if command is empty
        const RptlCommandParser command_parser { regular_message };

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

            // Client who was owning this actor UID
            const std::uint64_t old_owner { actors_registry_.at(client_actor) };
            // Client owning actor is no longer alive
            connected_clients_.at(old_owner).first = false;

            unregisterActor(client_actor);

            // If actor is still registered, it is an implementation error
            assert(!isRegistered(client_actor));

            // Returns input event triggered by player disconnection (or unregistration)
            // RPTL command way disconnection, clean
            return Core::LeftEvent { client_actor };
        } else { // If none of available commands is being invoked, then invoked command is unknown
            throw BadClientMessage { "Unknown RPTL command: " + std::string { invoked_command_name } };
        }
    } catch (const Utils::NotEnoughWords&) { // If there isn't any word to parse (if command is empty)
        throw EmptyRptlCommand {};
    }
}

Core::AnyInputEvent NetworkBackend::handleMessage(const std::uint64_t client_token, const std::string& client_message) {
    // RPTL message source potential registered actor
    const std::optional<Actor> client_actor { connected_clients_.at(client_token).second };

    if (!client_actor.has_value()) // If no actor is registered for RPTL message client
        return handleHandshake(client_token, client_message);
    else // If any actor is actually registered for RPTL message client
        return handleRegular(client_actor->uid, client_message); // Handle command for registered actor
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

void NetworkBackend::registerActor(const std::uint64_t client_token, const std::uint64_t actor_uid, std::string name) {
    // Checks over all alive actors for UID availability, it must already exists inside actors registry
    for (const auto& client : connected_clients_) {
        const std::optional<Actor>& client_actor { client.second.second };  // Second value of client status, actor

        // Only checks for initialized actor (only alive actors can have initialized actor)
        if (client_actor.has_value()) {
            if (client_actor->uid == actor_uid) // First, checks for UID
                throw std::invalid_argument { "Actor UID " + std::to_string(actor_uid) + " unavailable" };

            if (client_actor->name == name) // Then, checks for name
                throw std::invalid_argument { "Actor name \"" + name + "\" unavailable" };
        }
    }

    // Checks for client to exists
    if (connected_clients_.count(client_token) == 0)
        throw UnknownClientToken { client_token };

    auto& [client_alive, client_actor] { connected_clients_.at(client_token) };

    // Checks for client to have alive connection
    if (!client_alive)
        throw std::invalid_argument { "Client with token " + std::to_string(client_token) + " is no longer alive" };

    // Initializes actor for given client
    client_actor = { actor_uid, std::move(name) };
    // Inserts initialized actor UID into registry
    const auto uid_insert_result { actors_registry_.insert({ actor_uid, client_token }) };

    assert(uid_insert_result.second); // Checks for UID insertion
}

void NetworkBackend::unregisterActor(const std::uint64_t actor_uid) {
    // Find actor UID entry with owner client token
    const auto uid_entry { actors_registry_.find(actor_uid) };

    // Reset actor object to uninitialized associated with owner client token and set alive status to false
    auto& [alive, actor] { connected_clients_.at(uid_entry->second) };
    actor.reset();
    alive = false;

    // Remove actor UID from registry, as it is no longer owned by any client
    actors_registry_.erase(uid_entry);
}

bool NetworkBackend::isRegistered(const std::uint64_t actor_uid) const {
    return actors_registry_.count(actor_uid) == 1; // Checks for UID entries to contain given actor UID
}

bool NetworkBackend::isAlive(std::uint64_t client_token) const {
    // Checks for client to exist
    if (connected_clients_.count(client_token) == 0)
        throw UnknownClientToken { client_token };

    return connected_clients_.at(client_token).first;
}

void NetworkBackend::addClient(const std::uint64_t new_token) {
    // Checks for token availability
    if (connected_clients_.count(new_token) == 1)
        throw UnavailableClientToken { new_token };

    // Inserts client alive and unregistered
    const auto insert_result {
        connected_clients_.insert({ new_token, std::make_pair<bool, std::optional<Actor>>(true, {}) })
    };

    assert(insert_result.second); // Checks for insertion to be successfully done
}

void NetworkBackend::killClient(const std::uint64_t client_token, const Utils::HandlingResult& disconnection_reason) {
    if (connected_clients_.count(client_token) == 0) // Checks for client to exist
        throw UnknownClientToken { client_token };

    auto& [alive, actor] { connected_clients_.at(client_token) }; // Retrieves alive property and potential actor

    if (actor.has_value()) { // If associated actor exists and is registered
        // Then pipeline must be closed, unregistering actor and making client to no longer be alive
        closePipelineWith(actor->uid, disconnection_reason);
    } else { // Else, only marks it as no longer alive
        alive = false;
    }
}

void NetworkBackend::removeClient(const std::uint64_t old_token) {
    if (connected_clients_.count(old_token) == 0) // Checks for client to exist
        throw UnknownClientToken { old_token };

    const bool alive_client { connected_clients_.at(old_token).first }; // Retrieves alive client property

    // Checks for client to no longer be alive
    if (alive_client)
        throw AliveClient { old_token };

    // Removes client from connected clients
    const std::size_t removed_clients { connected_clients_.erase(old_token) };

    assert(removed_clients == 1); // Must have removed exactly one connected client
}

void NetworkBackend::closePipelineWith(const std::uint64_t actor, const Utils::HandlingResult& clean_shutdown) {
    // Server must be notified by disconnection, clean if no error occurred, crash with error message otherwise
    if (clean_shutdown)
        pushInputEvent(Core::LeftEvent { actor });
    else
        pushInputEvent(Core::LeftEvent { actor, clean_shutdown.errorMessage() });

    // Checks for actor to be registered
    if (!isRegistered(actor))
        throw UnknownActorUID { actor };

    // Actor is no longer connected, removes it from register and marks it as no longer alive
    unregisterActor(actor);
}


}
