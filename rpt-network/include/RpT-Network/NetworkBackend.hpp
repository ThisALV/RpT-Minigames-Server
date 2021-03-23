#ifndef RPTOGETHER_SERVER_NETWORKBACKEND_HPP
#define RPTOGETHER_SERVER_NETWORKBACKEND_HPP

#include <cstdint>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <RpT-Core/InputOutputInterface.hpp>
#include <RpT-Utils/HandlingResult.hpp>
#include <RpT-Utils/TextProtocolParser.hpp>

/**
 * @file NetworkBackend.hpp
 */


namespace RpT::Network {


/**
 * @brief Thrown by `NetworkBackend::handleMessage()` and `NetworkBackend::handleMessage()` if received client RPTL
 * message is ill-formed.
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
 * @brief Thrown by `NetworkBackend::handleHandshake()` if handshaking is valid but actor hasn't been registered
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
 * @brief Thrown by `NetworkBackend::isAlive()` if given client to check status for doesn't exist
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class UnknownClientToken : public std::logic_error {
public:
    explicit UnknownClientToken(const std::uint64_t invalid_token)
    : std::logic_error { "Client with token " + std::to_string(invalid_token) + " doesn't exist" } {}
};


/**
 * @brief Base class for `RpT::Core::InputOutputInterface` implementations based on networking protocol.
 *
 * Implements a networking protocol (RPTL, or RpT Login Protocol) which is managing players list, connecting and
 * disconnecting players and storing players common data (like name, or actor UID). Protocol made to exist under SER
 * layer so it can transmits received SR commands to `Core::ServiceEventRequestProtocol` and transmits SE commands
 * and SRR to actors client.
 *
 * This class only implements synchronous server state and server logic operations for RPTL protocol. ALl
 * asynchronous IO operations used to sync clients state and server state are defined by implementation (subclass).
 *
 * Every triggered input event is pushed into an events queue checked each time
 * `waitForInput()` is called. If queue is empty, `waitForEvent()` (defined by implementation) waits for IO
 * operations handled to push at least one input event into queue.
 *
 * RPTL, text-based protocol specifications:
 *
 * Each RPTL frame is a message. Messages can be received by client from server, or by server from client. Each
 * message must follow that syntax: `<RPTL_command> [args]...` and <RPTL_command> must NOT be empty.
 *
 * A client connection will be in one of the following modes: registered or unregistered. Each new client (newly
 * opened connection) begins with unregistered state. When a client connects with server, server internally use new
 * client token so it identifies created connection among already existing ones. This token is an implementation
 * detail, it isn't visible outside %NetworkBackend.
 *
 * After connection established successfully, server waits for client to send handshaking message (client to server
 * message using handshaking RPTL command). This handshaking message contains actor UID and name which will be
 * associated with client token. Then, a client will go to registered mode, having an associated actor so it can
 * interact with other clients and services.
 *
 * If any error occurres at RPTL / SER level, client connection is closed by server using RPTL command interrupts,
 * providing disconnection reason if any available. Interrupt message is the same for registered and unregistered
 * connected clients.
 *
 * When client is unregistered after having been registered (pipeline closed, interrupted, logout command...) client
 * is no longer alive. Implementation can checks for each client if it's alive or not. If not, connection can be closed.
 *
 * Messages from server to clients can take two forms : private message or broadcast message. Private messages are
 * sent to a specific registered or not client token while broadcast messages are sent to all registered clients.
 *
 * Commands summary:
 *
 * Client to server:
 * - Handshake: `LOGIN <uid> <name>`, must NOT be registered
 * - Log out (clean way): `LOGOUT`, must BE registered
 * - Send Service Request command: `SERVICE <SR_command>` (see `RpT::Core::ServiceEventRequestProtocol`), must BE
 * registered
 *
 * Server to client, private:
 * - Registration confirmation: `REGISTRATION OK [<uid_1> <actor_1>]...` or `REGISTRATION KO <ERR_MSG>`, must NOT be
 * registered
 * - Connection closed: `INTERRUPT [ERR_MSG]`, might be registered OR not
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

    static constexpr std::string_view HANDSHAKE_COMMAND { "LOGIN" };
    static constexpr std::string_view LOGOUT_COMMAND { "LOGOUT" };
    static constexpr std::string_view SERVICE_COMMAND { "SERVICE" };

    /*
     * Prefixes for RPTL protocol commands invoked and formatted by server
     */

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

        /// Checks if command is an handshake for actor registering
        bool isHandshake() const;
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

    /// Registered client actor has an UID and a name
    struct Actor {
        std::uint64_t uid;
        std::string name;
    };

    // First value for client alive or not status; Second value uninitialized if unregistered
    std::unordered_map<std::uint64_t, std::pair<bool, std::optional<Actor>>> connected_clients_;
    // Actor UID with its owner client token
    std::unordered_map<std::uint64_t, std::uint64_t> actors_registry_;
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
     * @brief Remove actor using given UID, `noexcept` as called inside critical error handling code
     *
     * @param actor_uid UID for registered actor to logout
     */
    void unregisterActor(std::uint64_t actor_uid) noexcept;

protected:
    /**
     * @brief Parses received handshake and registers new actor for given client
     *
     * @param client_token Client to be passed into registered mode
     * @param client_handshake Handshake data received from connected client
     *
     * @returns Input triggered by handshake, means that it was handled successfully, and actor has been registered
     *
     * @throws BadClientMessage if invoked command isn't valid connection handshake (client message fault)
     * @throws InternalError if invoked command is valid connection handshake but registration hasn't been done
     * (server internal state fault, example: unavailable UID)
     */
    Core::JoinedEvent handleHandshake(std::uint64_t client_token, const std::string& client_handshake);

    /**
     * @brief Parses given received RPTL message from client with associated registered actor UID and retrieves
     * triggered input event.
     *
     * @param client_actor UID for actor representing this client
     * @param client_message Received client message (received network data) to handle
     *
     * @returns Event triggered by message, must be `Core::LeftEvent` or `Core::ServiceRequestEvent`, as only these
     * events can be triggered by a registered actor
     *
     * @throws BadClientMessage if given client message is ill-formed (missing args, unknown command...)
     */
    Core::AnyInputEvent handleMessage(std::uint64_t client_actor, const std::string& client_message);

    /**
     * @brief Push given triggered input event into queue
     *
     * @param input_event Triggered input event to push
     */
    void pushInputEvent(Core::AnyInputEvent input_event);

    /**
     * @brief Wait for external input event to happen
     *
     * Called by `waitForInput()` when events queue is empty implementation, must be overridden by `NetworkBackend`
     * implementations, but not called.
     *
     * @returns Input event triggered by external cause
     */
    virtual Core::AnyInputEvent waitForEvent() = 0;



    /**
     * @brief Checks if given actor UID is available or not, called before `registerActor()` to check for
     * registration validity and determines client connection current mode (registered/unregistered).s
     *
     * @param actor_uid UID to check for availability
     *
     * @returns `true` is given actor UID is available, `false` otherwise
     */
    bool isRegistered(std::uint64_t actor_uid) const;

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

public:
    /**
     * @brief If any, poll input event inside queue. If queue is empty, wait until input event is triggered.
     *
     * The input event waiting stage, if queue is empty, is implementation-specific.
     *
     * @returns Last triggered input event
     */
    Core::AnyInputEvent waitForInput() final;

    /**
     * @brief Unregisters actor using given UID and emits input event for player disconnection
     *
     * @param actor UID which will has it's connection closed
     * @param clean_shutdown Error message, if any clean_shutdown caused actor disconnection
     */
    void closePipelineWith(std::uint64_t actor, const Utils::HandlingResult& clean_shutdown) final;
};


}


#endif //RPTOGETHER_SERVER_NETWORKBACKEND_HPP
