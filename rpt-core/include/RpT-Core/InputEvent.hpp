#ifndef RPTOGETHER_SERVER_INPUTEVENT_HPP
#define RPTOGETHER_SERVER_INPUTEVENT_HPP

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <RpT-Utils/HandlingResult.hpp>

/**
 * @file InputEvent.hpp
 */


namespace RpT::Core {


/**
 * @brief Input event, like timer trigger, service request or server termination request
 *
 * Represents any kind of input event which can occur at IO interface level, returned by
 * `InputOutputInterface::waitForInput()`.
 *
 * Each event type might provides more informations about input event. Should be visited using visitor pattern to access
 * those informations.
 *
 * Events are received and emitted by actors. An actor is a connected client who can interferes with server
 * execution, by sending Service Request command, as an example. Each actor is identified with UID, a 64bits unsigned
 * integer.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class InputEvent {
private:
    std::uint64_t actor_;

public:
    /**
     * @brief Base constructor for initializing emitted UID
     *
     * @param actor UID for actor emitting input event
     */
    explicit InputEvent(std::uint64_t actor);

    // Necessary for correct deletion of subclasses instances
    virtual ~InputEvent() = default;

    /**
     * @brief Get actor who emitted this event
     *
     * @returns Actor UID
     */
    std::uint64_t actor() const;
};

/// Event emitted if interface is closed
class NoneEvent : public InputEvent {
public:
    explicit NoneEvent(std::uint64_t actor);

};

/// Event emitted when service request is received
class ServiceRequestEvent : public InputEvent {
private:
    std::string service_request_;

public:
    /**
     * @brief Constructs input event with given service request.
     *
     * Learn more about Service Event Request Protocol at `ServiceEventRequestProtocol` class doc.
     *
     * @param actor Actor UID
     * @param service_request Received SER request
     */
    ServiceRequestEvent(std::uint64_t actor, std::string service_request);

    /**
     * @brief Get received service request using SR command format
     *
     * @returns SR command
     */
    const std::string& serviceRequest() const;
};

/// Event emitted when a timer is timed out
class TimerEvent : public InputEvent {
public:
    explicit TimerEvent(std::uint64_t actor);
};

/// Event emitted when any new actor joins the server
class JoinedEvent : public InputEvent {
private:
    std::string new_actor_name_;
public:
    /**
     * @brief Constructs player joined event with given player informations
     *
     * @param new_actor_uid New player UID
     * @param new_actor_name New player name
     */
    explicit JoinedEvent(std::uint64_t new_actor_uid, std::string new_actor_name);

    /**
     * @brief Gets joined player's name
     *
     * @returns Name for new player
     */
    const std::string& playerName() const;
};


/**
 * @brief Thrown by `LeftEvent::errorMessage()` if disconnection was done properly without crash reason
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class NoErrorMessage : public std::logic_error {
public:
    /**
     * @brief Constructs error with basic error message
     *
     * @param actor_uid UID for actor who has been disconnected using the clean way
     */
    explicit NoErrorMessage(const std::uint64_t actor_uid)
    : std::logic_error { "Actor " + std::to_string(actor_uid) + " didn't crash, no error message" } {}
};

/// Event emitted when any actor leaves the server
class LeftEvent : public InputEvent {
private:
    Utils::HandlingResult disconnection_reason_;

public:
    /**
     * @brief Constructs player disconnection event for clean way logout
     *
     * @param actor UID for disconnected actor
     */
    LeftEvent(std::uint64_t actor);

    /**
     * @brief Constructs player disconnection for crashed player with given error message
     *
     * @param actor UID for disconnected actor
     * @param error_message Message explaining why crash occurred
     */
    LeftEvent(std::uint64_t actor, std::string error_message);

    /**
     * @brief Get disconnection reason
     *
     * @returns Disconnection reason, contains error message if player crashed
     */
    Utils::HandlingResult disconnectionReason() const;
};


}


#endif //RPTOGETHER_SERVER_INPUTEVENT_HPP
