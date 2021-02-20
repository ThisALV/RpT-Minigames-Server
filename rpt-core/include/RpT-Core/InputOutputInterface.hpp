#ifndef RPTOGETHER_SERVER_INPUTOUTPUTINTERFACE_HPP
#define RPTOGETHER_SERVER_INPUTOUTPUTINTERFACE_HPP

#include <memory>
#include <RpT-Core/InputEvent.hpp>

/**
 * @file InputOutputInterface.hpp
 */


namespace RpT::Core {


/**
 * @brief Base class for input/output operations backend
 *
 * Subclasses will serve as backend, and every access inside rpt-server will be done using base class reference to
 * make a backend API.
 *
 * Input events refers to any event that affects `Executor` runtime and state, and which are external to the main
 * loop. Example : timer trigger, received service request, stop request...
 *
 * Output events refers to any event initiated by `Executor` main loop that must dispatched to clients. This might
 * either be a service request which was successfully handled or an event emitted by a service.
 *
 * @note Interface is automatically closed at destruction.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class InputOutputInterface {
protected:
    bool closed_;

public:
    /**
     * @brief Constructs an open IO interface
     *
     * @note This default constructor allows sub classes default construction
     */
    InputOutputInterface();

    // Virtual destructor allows correct deletion for sub classes instances
    virtual ~InputOutputInterface() = default;

    // Entity class semantic :

    InputOutputInterface(const InputOutputInterface&) = delete;
    InputOutputInterface& operator=(const InputOutputInterface&) = delete;

    bool operator==(const InputOutputInterface&) const = delete;

    /**
     * @brief Blocks until any kind of input event occurs
     *
     * @returns Pointer to the last input event that occurred, so polymorphism can be used
     */
    virtual std::unique_ptr<InputEvent> waitForInput() = 0;

    /**
     * @brief Output response to actor for a given service request
     *
     * Allows to inform an actor if a requested succeeded, and if not, what error happened.
     *
     * @param service_request Request which this response is replying to
     * @param success Was the request successfully handled ?
     * @param error_message If and only if request wasn't handled successfully, why ?
     */
    virtual void replyTo(const ServiceRequestEvent& service_request, bool success,
                         const std::optional<std::string>& error_message) = 0;


    /**
     * @brief Dispatch a successfully handled service request to actors who aren't actor of `service_request`
     *
     * @param service_request Successfully handled request to dispatch. Will not be dispatched to its own actor.
     */
    virtual void outputRequest(const ServiceRequestEvent& service_request) = 0;

    /**
     * @brief Dispatch an event emitted by a service to all actors
     *
     * @param event Event string representation based on SER Protocol (see `ServiceEventRequestProtocol` doc)
     */
    virtual void outputEvent(const std::string& event) = 0;

    /**
     * @brief Free interface IO ressources and mark it as closed
     *
     * After call, `closed()` must return `true`.
     */
    virtual void close();

    /**
     * @brief Returns if IO interface was closed
     *
     * @return `true` if `close()` has been called
     */
    bool closed() const;
};


}


#endif //RPTOGETHER_SERVER_INPUTOUTPUTINTERFACE_HPP
