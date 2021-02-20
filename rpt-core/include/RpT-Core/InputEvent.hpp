#ifndef RPTOGETHER_SERVER_INPUTEVENT_HPP
#define RPTOGETHER_SERVER_INPUTEVENT_HPP

#include <cstdint>
#include <optional>
#include <string>

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
 * An input is defined by a type and an actor, which emitted the event.
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
     * @brief Kind of input
     *
     * @author ThisALV, https://github.com/ThisALV
     */
    enum struct Type {
        /// Returned to indicate that interface has been closed
        None,
        /// Any player sent service request
        ServiceRequest,
        /// Any active timer actually timed out
        TimerTrigger,
        /// Any signal or console ctrl asked the server to stop
        StopRequest,
        /// A new player joined the server
        PlayerJoined,
        /// A player left the server, no matter the reason
        PlayerLeft
    };

    /**
     * @brief Get what kind of event it is
     *
     * @returns `Type` representation to determine event nature
     */
    virtual Type type() const = 0;

    /**
     * @brief Get additional data about this input event, example : service request, stop request reason...
     *
     * @returns String representation for event additional data
     */
    virtual std::optional<std::string> additionalData() const;

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

    Type type() const override;
};

/// Event emitted when service request is received
class ServiceRequestEvent : InputEvent {
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

    /// Returns `InputEvent::Type::ServiceRequest`
    Type type() const override;
    /// Returns received SER request
    std::optional<std::string> additionalData() const override;
};

/// Event emitted when a timer is timed out
class TimerEvent : public InputEvent {
public:
    explicit TimerEvent(std::string_view actor);

    /// Returns `InputEvent::Type::TimerTrigger`
    Type type() const override;
};

/// Event emitted when server stop is requested by a signal / a console ctrl
class StopEvent : public InputEvent {
private:
    std::uint8_t caught_signal_;

public:
    /**
     * @brief Constructs stop request event with given caught signal
     * 
     * @param actor Actor's name access
     * @param signal Integer value for caught signal
     */
    StopEvent(std::string_view actor, std::uint8_t caught_signal);
    
    /// Returns `InputEvent::Type::StopRequest
    Type type() const override;
    /// Returns string representation for signal code
    std::optional<std::string> additionalData() const override;
};

/// Event emitted when any new actor joins the server
class JoinedEvent : public InputEvent {
public:
    explicit JoinedEvent(std::string_view actor);

    /// Returns `InputEvent::Type::PlayerJoined`
    Type type() const override;
};

/// Event emitted when any actor leaves the server
class LeftEvent : public InputEvent {
public:
    /// Reason for player disconnection
    enum struct Reason {
        /// Server shutdown or client-side regular disconnection
        Clean,
        /// Error occurred with this player
        Crash
    };
private:
    Reason disconnection_reason_;
    std::optional<std::string_view> error_message_;

public:
    /**
     * @brief Constructs player disconnection event with given disconnection reason.
     *
     * @see LeftEvent::Reason
     *
     * @param actor Actor's name access
     * @param disconnection_reason The reason for which the player disconnected
     * @param err_msg Optional error message, only accepted if reason is `Reason::Crash`
     */
    LeftEvent(std::string_view actor, Reason disconnection_reason, const std::optional<std::string_view>& err_msg = {});

    /// Returns `InputEvent::Type::PlayerLeft`
    Type type() const override;
    /// Returns string representation of disconnection `LeftEvent::Reason`
    std::optional<std::string> additionalData() const override;
};


}


#endif //RPTOGETHER_SERVER_INPUTEVENT_HPP
