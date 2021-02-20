#ifndef RPTOGETHER_SERVER_INPUTOUTPUTINTERFACE_HPP
#define RPTOGETHER_SERVER_INPUTOUTPUTINTERFACE_HPP

#include <memory>
#include <optional>
#include <string>


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
    const std::string_view actor_;

public:
    /**
     * @brief Base constructor for initializing actor's name
     *
     * @param actor Actor's name access
     */
    explicit InputEvent(std::string_view actor);

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
     * @return `Type` representation to determine event nature
     */
    virtual Type type() const = 0;

    /**
     * @brief Get actor who emitted this event
     *
     * @return Actor's name
     */
    std::string_view actor() const;
};

/**
 * @brief Base class for input/output operations backend
 *
 * Subclasses will serve as backend, and every access inside rpt-server will be done using base class reference to
 * make a backend API.
 *
 * Input events refers to any event that affects `Executor` runtime and state, and which are external to the main
 * loop. Example : timer trigger, received service request, stop request...
 *
 * Output events refers to any event initiated by `Executor` main loop that must dispatched to clients. Example :
 * service request response and/or and service request dispatched to every clients...
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class InputOutputInterface {
public:
    /**
     * @brief Default constructor allowing base classes to be default constructed
     */
    InputOutputInterface() = default;

    // Entity class semantic :

    InputOutputInterface(const InputOutputInterface&) = delete;
    InputOutputInterface& operator=(const InputOutputInterface&) = delete;

    bool operator==(const InputOutputInterface&) const = delete;

    /**
     * @brief Blocks until any kind of input event occurs
     *
     * @return Pointer to the last input event that occurred, so polymorphism can be used
     */
    virtual std::unique_ptr<InputEvent> waitForInput() = 0;
};

}


#endif //RPTOGETHER_SERVER_INPUTOUTPUTINTERFACE_HPP
