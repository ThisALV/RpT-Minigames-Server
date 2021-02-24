#ifndef RPTOGETHER_SERVER_INPUTEVENT_HPP
#define RPTOGETHER_SERVER_INPUTEVENT_HPP

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

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
 * An input event is defined by an actor which has emitted event.
 * Each event type might provides more informations about input event. Should be used using visitor pattern to access
 * those informations.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class InputEvent {
private:
    std::string_view actor_;

public:
    /**
     * @brief Base constructor for initializing actor's name
     *
     * @param actor Actor's name access
     */
    explicit InputEvent(std::string_view actor);

    // Necessary for correct deletion of subclasses instances
    virtual ~InputEvent() = default;

    /**
     * @brief Get actor who emitted this event
     *
     * @returns Actor's name
     */
    std::string_view actor() const;
};

/// Event emitted if interface is closed
class NoneEvent : public InputEvent {
public:
    explicit NoneEvent(std::string_view actor);

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
     * @param actor Actor's name access
     * @param service_request Received SER request
     */
    ServiceRequestEvent(std::string_view actor, std::string service_request);

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
    explicit TimerEvent(std::string_view actor);
};

/// Event emitted when server stop is requested by a caughtSignal / a console ctrl
class StopEvent : public InputEvent {
private:
    std::uint8_t caught_signal_;

public:
    /**
     * @brief Constructs stop request event with given caught caughtSignal
     * 
     * @param actor Actor's name access
     * @param signal Integer value for caught caughtSignal
     */
    StopEvent(std::string_view actor, std::uint8_t caught_signal);

    /**
     * @brief Gets caught Posix stop caughtSignal
     *
     * @return Posix stop caughtSignal
     */
    std::uint8_t caughtSignal() const;
};

/// Event emitted when any new actor joins the server
class JoinedEvent : public InputEvent {
public:
    explicit JoinedEvent(std::string_view actor);
};


/**
 * @brief Thrown by `LeftEvent::errorMessage()` if disconnection wasn't an error
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class NotAnErrorReason : public std::logic_error {
public:
    /**
     * @brief Constructs error with basic error message
     */
    NotAnErrorReason() : std::logic_error { "Disconnection disconnectionReason wasn't an error" } {}
};

/// Event emitted when any actor leaves the server
class LeftEvent : public InputEvent {
public: // Required by private fields, so publicly declared before
    /// Reason for player disconnection
    enum struct Reason {
        /// Server shutdown or client-side regular disconnection
        Clean,
        /// Error occurred with this player
        Crash
    };

private:
    Reason disconnection_reason_;
    std::optional<std::string> error_message_;

public:
    /**
     * @brief Constructs player disconnection event with given disconnection disconnectionReason.
     *
     * @see LeftEvent::Reason
     *
     * @param actor Actor's name access
     * @param disconnection_reason The disconnectionReason for which the player disconnected
     * @param err_msg Optional error message, only accepted if disconnectionReason is `Reason::Crash`
     */
    LeftEvent(std::string_view actor, Reason disconnection_reason, std::optional<std::string> err_msg = {});

    /**
     * @brief Get disconnection disconnectionReason
     *
     * @returns `Reason` enum value
     */
    Reason disconnectionReason() const;

    /**
     * @brief Get error message for `Reason::Crash` disconnection disconnectionReason
     *
     * @returns Error message
     *
     * @throws NotAnErrorReason if `disconnectionReason()` doesn't return `Reason::Crash`
     */
    const std::string& errorMessage() const;
};


}


#endif //RPTOGETHER_SERVER_INPUTEVENT_HPP
