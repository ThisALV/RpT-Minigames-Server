#ifndef RPTOGETHER_SERVER_NETWORKBACKEND_HPP
#define RPTOGETHER_SERVER_NETWORKBACKEND_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <RpT-Core/InputOutputInterface.hpp>
#include <RpT-Network/MessagesQueueView.hpp>
#include <RpT-Utils/HandlingResult.hpp>
#include <RpT-Utils/TextProtocolParser.hpp>

/**
 * @file NetworkBackend.hpp
 */


namespace RpT::Network {


/**
 * @brief Thrown by `NetworkBackend::handleMessage()` if received client RPTL message is ill-formed
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class BadClientMessage : public std::logic_error {
public:
    /**
     * @brief Constructs exception with custom error message
     *
     * @param reason Custom error message describing why client RPTL message is ill-formed
     */
    explicit BadClientMessage(const std::string& reason) : std::logic_error { reason } {}
};

/**
 * @brief Thrown when tried to parse empty RPTL command
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class EmptyRptlCommand : public BadClientMessage {
public:
    /**
     * @brief Constructs exception with basic error message
     */
    EmptyRptlCommand() : BadClientMessage { "RPTL command must NOT be empty" } {}
};

/**
 * @brief Thrown when too many arguments are given to specific RPTL command
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class TooManyArguments : public BadClientMessage {
public:
    /**
     * @brief Constructs basic error message for given command
     *
     * @param rptl_command Invoked command which is throwing error
     */
    explicit TooManyArguments(const std::string_view rptl_command)
    : BadClientMessage { "Too many arguments given to command: " + std::string { rptl_command } } {}
};


/**
 * @brief Thrown by `NetworkBackend::handleMessage()` if received RPTL command is valid but hasn't been properly
 * handled due to current server state
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class InternalError : public std::runtime_error {
public:
    /**
     * @brief Constructs exception with custom error message
     *
     * @param reason Custom error message to explain internal error which occurred
     */
    explicit InternalError(const std::string& reason) : std::runtime_error { "Registration failed: " + reason } {}
};


/**
 * @brief Thrown by `NetworkBackend::isAlive()` and `NetworkBackend::removeClient()` if given client to check status
 * for doesn't exist
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class UnknownClientToken : public std::logic_error {
public:
    /**
     * @brief Constructs basic error message for given token
     *
     * @param invalid_token Token not used by any connected client
     */
    explicit UnknownClientToken(const std::uint64_t invalid_token)
    : std::logic_error { "Client with token " + std::to_string(invalid_token) + " doesn't exist" } {}
};


/**
 * @brief Thrown by `NetworkBackend::addClient()` if given token is already in use
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class UnavailableClientToken : public std::logic_error {
public:
    /**
     * @brief Constructs basic error message for given token
     *
     * @param used_token Token used by another connected client
     */
    explicit UnavailableClientToken(const std::uint64_t used_token)
    : std::logic_error { "Client token " + std::to_string(used_token) + " is already in use" } {}
};


/**
 * @brief Thrown by `NetworkBackend::removeClient()` if given client is still alive
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class AliveClient : public std::logic_error {
public:
    /**
     * @brief Constructs error with basic error message for given client
     *
     * @param client_token Client tried to be removed
     */
    explicit AliveClient(const std::uint64_t client_token)
    : std::logic_error { "Client " + std::to_string(client_token) + " is alive" } {}
};


/**
 * @brief Thrown by `NetworkBackend::closePipelineWith()` if given toke is already in use
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class UnknownActorUID : public std::logic_error {
public:
    /**
     * @brief Constructs basic error message for given UID
     *
     * @param invalid_uid UID not used by any registered actor
     */
    explicit UnknownActorUID(const std::uint64_t invalid_uid)
    : std::logic_error { "No register actor with UID " + std::to_string(invalid_uid) } {}
};


/**
 * @brief Base class for `RpT::Core::InputOutputInterface` implementations based on networking protocol.
 *
 * Implements a networking protocol (RPTL, or RpT Login Protocol) which is managing players list, connecting and
 * disconnecting players and storing players common data (like name, or actor UID). Protocol made to exist under SER
 * layer so it can transmits received SR commands to `Core::ServiceEventRequestProtocol` and transmits SE commands
 * and SRR to actors client.
 *
 * This class only implements synchronous server state or server logic operations for RPTL protocol and RPTL
 * messages formatting and queuing. All asynchronous IO operations used to sync clients state and server state are
 * defined by implementation (subclass).
 *
 * Every triggered input event is pushed into an events queue checked each time `waitForInput()` is called. If queue
 * is empty, `waitForEvent()` (defined by implementation) waits for IO operations handled to push at least one input
 * event into queue.
 *
 * Each time interaction from clients is expected, call to `syncClient()` (defined by implementation) if performed to
 * ensure clients are aware about server and game state before doing any interaction.
 *
 * RPTL, text-based protocol specifications:
 *
 * Each RPTL frame is a message. Messages can be received by client from server, or by server from client. Each
 * message must follow that syntax: `<RPTL_command> [args]...` and `<RPTL_command>` must NOT be empty.
 *
 * A client connection will be in one of the following modes: registered or unregistered. Each new client (newly
 * opened connection) begins with unregistered state. When a client connects with server, server internally use new
 * client token so it identifies created connection among already existing ones. This token is an implementation
 * detail, it isn't visible outside %NetworkBackend and its RPTL protocol implementation.
 *
 * After connection established successfully, server waits for client to send handshaking message (client to server
 * message using handshaking RPTL command). This handshaking message contains actor UID and name which will be
 * associated with client token. Then, a client will go to registered mode, having an associated actor so it can
 * interact with other clients and services.
 *
 * Prior to send handshaking message, a client might send checkout message to check if maximum numbers of actors has
 * been reached or not. A handshake cannot be performed if server is already full.
 *
 * If any error occurres at RPTL / SER level, client connection is closed by server using RPTL command interrupts,
 * providing disconnection reason if any available. Interrupt message is the same for registered and unregistered
 * connected clients.
 *
 * When client is unregistered after having been registered (pipeline closed, interrupted, logout command...) client
 * is no longer alive. Implementation can checks for each client if it's alive or not. If not, connection can be
 * closed. An alive connection can only be closed in case of server being stopped, or if an error occurred.
 *
 * Messages from server to clients can take two forms : private message or broadcast message. Private messages are
 * sent to a specific registered or not client token while broadcast messages are sent to all registered clients.
 *
 * Commands summary:
 *
 * Client to server:
 * - Checkout: `CHECKOUT`, must NOT be registered
 * - Handshake: `LOGIN <uid> <name>`, must NOT be registered
 * - Log out (clean way): `LOGOUT`, must BE registered
 * - Send Service Request command: `SERVICE <SR_command>` (see `Core::ServiceEventRequestProtocol`), must BE registered
 *
 * Server to client, private:
 * - Check: `AVAILABILITY <actors_count> <max_actors_number>`, must NOT be registered
 * - Registration confirmation: `REGISTRATION [<uid_1> <actor_1>]...`, must NOT be registered
 * - Connection closed: `INTERRUPT [ERR_MSG]`, must BE registered
 * - Service Request Response: `SERVICE <SRR>`, must BE registered, see `Core::ServiceEventRequestProtocol` for SRR doc
 *
 * Server to clients, broadcast, must BE registered:
 * - Logged in actor: `LOGGED_IN <uid> <name>`
 * - Logged out actor: `LOGGED_OUT <uid>`
 * - Service Event command: `SERVICE <SE_command>`
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class NetworkBackend : public Core::InputOutputInterface {
private:
    /*
     * Prefixes for RPTL protocol commands invoked by clients
     */

    static constexpr std::string_view CHECKOUT_COMMAND { "CHECKOUT" };
    static constexpr std::string_view HANDSHAKE_COMMAND { "LOGIN" };
    static constexpr std::string_view LOGOUT_COMMAND { "LOGOUT" };
    static constexpr std::string_view SERVICE_COMMAND { "SERVICE" };

    /*
     * Prefixes for RPTL protocol commands invoked and formatted by server
     */

    static constexpr std::string_view AVAILABILITY_COMMAND { "AVAILABILITY" };
    static constexpr std::string_view REGISTRATION_COMMAND { "REGISTRATION" };
    static constexpr std::string_view INTERRUPT_COMMAND { "INTERRUPT" };
    static constexpr std::string_view LOGGED_IN_COMMAND { "LOGGED_IN" };
    static constexpr std::string_view LOGGED_OUT_COMMAND { "LOGGED_OUT" };

    /// Parser for RPTL Protocol command, only parsing command name
    class RptlCommandParser : public Utils::TextProtocolParser {
    public:
        explicit RptlCommandParser(std::string_view rptl_command);

        /// Retrieves invoked command name
        std::string_view invokedCommandName() const;

        /// Retrieves invoked command arguments
        std::string_view invokedCommandArgs() const;

    };

    /// Parser for RPTL `HANDSHAKE` command arguments
    class HandshakeParser : public Utils::TextProtocolParser {
    private:
        std::uint64_t parsed_actor_uid_;

    public:
        /**
         * @brief Parses handshake args from given parsed RPTL command
         *
         * @param parsed_rptl_command Parsed RPTL `LOGIN` command
         *
         * @throws BadClientMessage if parsed actor UID isn't a valid unsigned integer of 64bits, or if extra args
         * are given
         * @throws NotEnoughWords if arguments are missing
         */
        explicit HandshakeParser(const RptlCommandParser& parsed_rptl_command);

        /// Retrieves new actor UID
        std::uint64_t actorUID() const;

        /// Retrieves new actor name
        std::string_view actorName() const;
    };

    /// Parser for RPTL `SERVICE` command arguments
    class ServiceCommandParser : public Utils::TextProtocolParser {
    public:
        /**
         * @brief Parses service request args from given parsed RPTL command
         *
         * @param parsed_rptl_command Parsed RPTL `SERVICE` command
         */
        explicit ServiceCommandParser(const RptlCommandParser& parsed_rptl_command);

        /// Retrieves received Service Request command
        std::string_view serviceRequest() const;
    };

    /// Connected client status, providing alive/dead status and disconnection reason, if no longer alive
    struct ClientStatus {
        bool alive;
        Utils::HandlingResult disconnectionReason;
    };

    /// Registered client actor has an UID and a name
    struct Actor {
        std::uint64_t uid;
        std::string name;
    };

    std::size_t actors_limit_;
    // First value for client alive or not status; Second value uninitialized if unregistered
    std::unordered_map<std::uint64_t, std::pair<ClientStatus, std::optional<Actor>>> connected_clients_;
    // Actor UID with its owner client token
    std::unordered_map<std::uint64_t, std::uint64_t> actors_registry_;
    // Each client stream remaining messages to send, same message might be sent to many clients, so using sharde_ptr
    std::unordered_map<std::uint64_t, std::queue<std::shared_ptr<std::string>>> clients_remaining_messages_;
    // Input events emitted waiting to be handled
    std::queue<Core::AnyInputEvent> input_events_queue_;

    /**
     * @brief If input events queue isn't empty, take and retrive next event to handle
     *
     * Called at `waitForInput()` beginning to poll any queued event before waiting for another to be triggered.
     *
     * @returns Next triggered input event if any, uninitialized otherwise
     */
    std::optional<Core::AnyInputEvent> pollInputEvent();

    /**
     * @brief Initializes alive actor associated with given client using UID and name parameters
     *
     * @param client_token Client to initialize actor with given data
     * @param actor_uid New actor UID
     * @param name New actor name
     *
     * @throws std::invalid_argument if given UID or name is unavailable
     */
    void registerActor(std::uint64_t client_token, std::uint64_t actor_uid, std::string name);

    /**
     * @brief Remove actor using given UID, making associated client no longer alive
     *
     * @param actor_uid UID for registered actor to logout
     */
    void unregisterActor(std::uint64_t actor_uid);

    /**
     * @brief Pushes given message into queue for given client
     *
     * @param client_token Clients queue to be pushed
     * @param new_message Message to push into queue
     */
    void privateMessage(std::uint64_t client_token, std::string new_message);

    /**
     * @brief Pushes given message into queue for each registered client
     *
     * @param new_message Message to push into queues
     */
    void broadcastMessage(std::string new_message);

    /**
     * @brief Parses and handles received message from unregistered client
     *
     * Registers new actor for given client if message is a handshake or send `AVAILABILITY` private message to client
     * if it is a checkout.
     *
     * @param client_token Client to be passed into registered mode
     * @param message Handshake data received from connected client
     *
     * @returns Input event triggered by handshake, or null event if message is a checkout command
     *
     * @throws BadClientMessage if invoked command isn't valid connection handshake
     * @throws InternalError if invoked command is valid connection handshake but registration hasn't been done
     * (server internal state fault, example: unavailable UID)
     */
    Core::AnyInputEvent handleFromUnregistered(std::uint64_t client_token, const std::string& message);

    /**
     * @brief Parses given received RPTL message from client with associated registered actor UID and retrieves
     * triggered input event.
     *
     * @param client_actor UID for actor representing this client
     * @param regular_message Received client message (received network data) to handle
     *
     * @returns Event triggered by message, must be `Core::LeftEvent` or `Core::ServiceRequestEvent`, as only these
     * events can be triggered by a registered actor
     *
     * @throws BadClientMessage if given client message is ill-formed (missing args, unknown command...)
     */
    Core::AnyInputEvent handleFromActor(std::uint64_t client_actor, const std::string& regular_message);

    /**
     * @brief Generates RPTL Registration command message from current server state
     *
     * @returns Formatted RPTL message using `REGISTRATION` command
     */
    std::string formatRegistrationMessage() const;

protected:
    /**
     * @brief Parses given RPTL message from given client and retrieves triggered input event. Messages required to
     * sync clients with new server state are pushed into corresponding messages queues.
     *
     * Parsing mode (currently available commands) depends on current client connection mode (unregistered/registered).
     *
     * @param client_token
     * @param client_message
     *
     * @returns Event triggered by message, type must be `Core::LeftEvent`, `Core::ServiceRequestEvent`,
     * `Core::JoinedEvent` or `Core::NoneEvent` as only these events can be triggered by a client RPTL message
     *
     * @note `Core::NoneEvent` is returned on case of a server checkout using `CHECKOUT` command.
     *
     * @throws BadClientMessage if invoked command is ill-formed (invalid arguments, unknown command...)
     * @throws InternalError if invoked command is valid but server state makes it unable to propery handles command
     * (example: unavailable new actor UID for handshake command)
     */
    Core::AnyInputEvent handleMessage(std::uint64_t client_token, const std::string& client_message);

    /**
     * @brief Checks if given actor UID is available or not, called before `registerActor()` to check for
     * registration validity and determines client connection current mode (registered/unregistered)
     *
     * @param actor_uid UID to check for availability
     *
     * @returns `true` is given actor UID is available, `false` otherwise
     */
    bool isRegistered(std::uint64_t actor_uid) const;

    /**
     * @brief Push given triggered input event into queue
     *
     * @param input_event Triggered input event to push
     */
    void pushInputEvent(Core::AnyInputEvent input_event);

    /**
     * @brief Wait for external input event to happen and to be queued by running implementation asynchronous events
     * loop, must blocks until `inputReady()` evaluates to `true`
     *
     * Called by `waitForInput()` when events queue is empty, must be overridden by `NetworkBackend` implementations,
     * but not called.
     */
    virtual void waitForEvent() = 0;

    /**
     * @brief Ensures clients state are same than current server state by calling implementation-defined `syncClient
     * ()` method
     *
     * This method must be called by implementation. It is recommended to put call inside `waitForEvent()`
     * implementation, so it will be called each time interaction with clients might occurres.
     */
    void synchronize();

    /**
     * @brief Flushes messages queue in argument queue, must sends asynchronously all messages in flushed queue to
     * corresponding client
     *
     * Called by `synchronize()` to sync each client after messages queue has been flushed, must be overridden by
     * `NetworkBackend` implementations, but not called.
     *
     * @param client_token Token for client to send messages for
     * @param client_messages_queue Real_time pop-only queue for messages that still need to be sent
     */
    virtual void syncClient(std::uint64_t client_token, MessagesQueueView client_messages_queue) = 0;

    /**
     * @brief Checks if `waitForInput()` will immediately return or if it still has to wait for input event
     *
     * Should be called by `waitForEvent()` implementation to figure out when to stop waiting for next input event.
     *
     * @returns `true` if any input event is pushed into queue, `false` otherwise
     */
    bool inputReady() const;

    /**
     * @brief Add new connected client with given token, alive and unregistered
     *
     * @param new_token Token used by new client
     *
     * @throws UnavailableClientToken if `new_token` is already used by another connected client
     */
    void addClient(std::uint64_t new_token);

    /**
     * @brief Makes sure that client is no longer alive, closing pipeline with actor if required and push private to
     * interrupt connection
     *
     * @param client_token Token for client to no longer be alive
     * @param disconnection_reason Reason for disconnection, useful for pipeline closure handling, ignored if
     * unregistered
     *
     * @throws UnknownClientToken if no client is connected using given token
     */
    void killClient(std::uint64_t client_token, const Utils::HandlingResult& disconnection_reason = {});

    /**
     * @brief Removes client which isn't alive
     *
     * @param old_token Token used by old client, will be available after method call
     *
     * @throws UnknownClientToken if no client is connected using given token
     * @throws AliveClient if given client is still alive so it cannot be removed
     */
    void removeClient(std::uint64_t old_token);

    /**
     * @brief Checks if given client is alive or if its connection can be closed by implementation
     *
     * @param client_token Client token to check status for
     *
     * @returns `true` if status is alive, `false` otherwise
     *
     * @throws UnknownClientToken if given client doesn't exist
     */
    bool isAlive(std::uint64_t client_token) const;

    /**
     * @brief Retrieves reason for given client to no longer be alive
     *
     * @param client_token Client to check disconnection reason for
     *
     * @returns `Utils::HandlingResult` for client disconnection
     *
     * @throws UnknownClientToken if given client doesn't exist
     * @throws AliveClient if client is still alive
     */
    const Utils::HandlingResult& disconnectionReason(std::uint64_t client_token) const;

public:
    /**
     * @brief Constructs backend without connected client, and with given actors number limits
     *
     * @param actors_limit Maximum number of actors registered
     */
    explicit NetworkBackend(std::size_t actors_limit);

    /**
     * @brief If any, poll input event inside queue. If queue is empty, wait until input event is triggered.
     *
     * The input event waiting stage, if queue is empty, is implementation-specific.
     *
     * @returns Last triggered input event
     */
    Core::AnyInputEvent waitForInput() final;

    /**
     * @brief Unregisters actor using given UID, emits input event for player disconnection and syncs clients about
     * player disconnection sending appropriate messages
     *
     * @param actor UID which will has it's connection closed
     * @param clean_shutdown Error message, if any clean_shutdown caused actor disconnection
     *
     * @throws UnknownActorUID if given actor UID isn't registered
     */
    void closePipelineWith(std::uint64_t actor, const Utils::HandlingResult& clean_shutdown) final;

    /**
     * @brief Fetches client for given actor and calls RPTL implementation to send given SRR formatted for RPTL protocol
     *
     * @param sr_actor Actor UID to fetch client for
     * @param sr_response Service Request Response formatted as SER command (see `Core::ServiceEventRequestProtocol`)
     *
     * @throws UnknownActorUID if given actor doesn't exist
     */
    void replyTo(std::uint64_t sr_actor, const std::string &sr_response) final;

    /**
     * @brief Calls RPTL implementation to send given SE formatted for RPTL protocol
     *
     * @param event Service Event command formatted as SER command (see `Core::ServiceEventRequestProtocol`)
     */
    void outputEvent(const std::string &event) final;
};


}


#endif //RPTOGETHER_SERVER_NETWORKBACKEND_HPP
