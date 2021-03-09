#ifndef RPTOGETHER_SERVER_NETWORKBACKEND_HPP
#define RPTOGETHER_SERVER_NETWORKBACKEND_HPP

#include <cstdint>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <RpT-Core/InputOutputInterface.hpp>
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
 * @brief Base class for `RpT::Core::InputOutputInterface` implementations based on networking protocol.
 *
 * Implements a common protocol (RPTL, or RpT Login Protocol) which is managing who is connected or disconnected, and
 * which is associating player informations (like name) with corresponding actor UID.
 *
 * Actors registration implementation is left to inheriting class.
 *
 * RPTL protocol can receive handshake from unregistered client connections to register them. When a client is
 * registered, it can send messages to server, which are following format: `<RPTL_command> <args>...`
 *
 * RPTL Protocol (after connection began):
 * - Handshake: `LOGIN <uid> <name>`, must NOT be registered
 * - Log out (clean way): `LOGOUT`, must BE registered
 * - Send Service Request command: `SERVICE <SR_command>` (see `RpT::Core::ServiceEventRequestProtocol`), must BE
 * registered
 *
 * An events queue is internally used for handling case of many input events triggered between `waitForInput()` calls.
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

    std::unordered_map<std::uint64_t, std::string> logged_in_actors_;
    std::queue<Core::AnyInputEvent> input_events_queue_;

    /**
     * @brief If input events queue isn't empty, take and retrive next event to handle
     *
     * Called at `waitForInput()` beginning to poll any queued event before waiting for another to be triggered.
     *
     * @returns Next triggered input event if any, uninitialized otherwise
     */
    std::optional<Core::AnyInputEvent> pollInputEvent();

protected:
    /**
     * @brief Parses received handshake and registers new actor
     *
     * @param client_handshake Handshake data received from connected client
     *
     * @returns Input triggered by handshake, means that it was handled successfully, and actor has been registered
     *
     * @throws BadClientMessage if invoked command isn't valid connection handshake (client message fault)
     * @throws InternalError if invoked command is valid connection handshake but registration hasn't been done
     * (server internal state fault, example: unavaiable UID)
     */
    Core::JoinedEvent handleHandshake(const std::string& client_handshake);

    /**
     * @brief Parses given received RPTL message from client with associated registered actor UID and retrieves
     * triggered input event.
     *
     * @param client_actor UID for actor representing this client
     * @param client_message Received client message (received network data) to handle
     *
     * @returns Event triggered by message, must be `Core::JoinedEvent`, `Core::LeftEvent` or
     * `Core::ServiceRequestEvent`, as only these events can be triggered by a registered actor
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
     * registration validity.
     *
     * @param actor_uid UID to check for availability
     *
     * @returns `true` is given actor UID is available, `false` otherwise
     */
    virtual bool isRegistered(std::uint64_t actor_uid) const = 0;

    /**
     * @brief Must register actor after valid handshaking message was received (called by superclass).
     *
     * It is guaranteed that `isRegistered(uid)` returns `false` before this call.
     *
     * Method implementation must offer following guarantees :
     * - If it throws exception, `isRegistered(uid)` will returns `false` after this call
     * - Else, `isRegistered(uid)` will returns `true` after this call
     *
     * @param uid New actor UID
     * @param name New actor name
     */
    virtual void registerActor(std::uint64_t uid, std::string_view name) = 0;

    /**
     * @brief Must unregister actor after valid logout message was received (called by superclass) OR after client
     * error occurred (might be called by subclass implementation).
     *
     * It is guaranteed that `isRegistered(uid)` returns `true` before this call.
     *
     * Method implementation CANNOT throws exception, and must guarantees that after this call, `isRegistered(uid)`
     * will returns `false`.
     *
     * @param uid UID for registered actor to logout
     */
    virtual void unregisterActor(std::uint64_t uid) = 0;

public:
    /**
     * @brief If any, poll input event inside queue. If queue is empty, wait until input event is triggered.
     *
     * The input event waiting stage, if queue is empty, is implementation-specific.
     *
     * @returns Last triggered input event
     */
    Core::AnyInputEvent waitForInput() final;
};


}


#endif //RPTOGETHER_SERVER_NETWORKBACKEND_HPP
