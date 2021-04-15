#ifndef RPTOGETHER_SERVER_INPUTOUTPUTINTERFACE_HPP
#define RPTOGETHER_SERVER_INPUTOUTPUTINTERFACE_HPP

#include <boost/variant.hpp>
#include <RpT-Core/InputEvent.hpp>
#include <RpT-Utils/HandlingResult.hpp>

/**
 * @file InputOutputInterface.hpp
 */


namespace RpT::Core {


/// For using visitor pattern on received input event. See `InputOutputInterface::waitForInput()`.
using AnyInputEvent = boost::variant<NoneEvent, ServiceRequestEvent, TimerEvent, JoinedEvent, LeftEvent>;


/**
 * @brief Base class for input/output operations backend
 *
 * Subclasses will serve as backend, and every access inside executor will be done using base class reference to
 * make a backend API.
 *
 * Input events refers to any event that affects `Executor` runtime and state, and which are external to the main
 * loop. Example : timer trigger, received service request, stop request...
 *
 * Output events refers to any event initiated by `Executor` main loop that must dispatched to clients. It basically
 * is Service Events emitted by SER Protocol instance.
 *
 * An IO interface might have its own protocol over SER Protocol. This custom protocol usually manages server
 * relative features, like name for players associated with specific UID, or players who are (dis)connecting from/to
 * server.
 *
 * IO interface instance can be closed so input events are no longer received and server stop.
 * Pipeline with actor can also be individually closed if broken using `closePipelineWith()` method.
 *
 * @note Interface is NOT automatically closed at destruction.
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
     * @brief Blocks until any kind of input event occurs, then retrieves it
     *
     * @returns An instance of `InputEvent` subclass. Type depends on input type.
     */
    virtual AnyInputEvent waitForInput() = 0;

    /**
     * @brief Output response to actor for a given service request
     *
     * Allows to inform an actor if a requested succeeded, and if not, what error happened.
     *
     * @param sr_actor Actor for SR command that this SRR is replying for
     * @param sr_response SRR for received SR command
     */
    virtual void replyTo(std::uint64_t sr_actor, const std::string& sr_response) = 0;

    /**
     * @brief Dispatch an event emitted by a service to all actors
     *
     * @param event Event string representation based on SER Protocol (see `ServiceEventRequestProtocol` doc)
     */
    virtual void outputEvent(const std::string& event) = 0;

    /**
     * @brief Closes pipeline with given actor and shutdown reason so it no longer can emit input events
     *
     * @param actor UID for actor to close pipeline with
     * @param clean_shutdown Used to determine what caused pipeline closing
     */
    virtual void closePipelineWith(std::uint64_t actor, const Utils::HandlingResult& clean_shutdown) = 0;

    /**
     * @brief Free interface IO ressources and mark it as closed
     *
     * After call, `closed()` must return `true`.
     *
     * Default implementation only marks instance as closed.
     */
    virtual void close();

    /**
     * @brief Returns if IO interface was closed
     *
     * @returns `true` if `close()` has been called
     */
    bool closed() const;
};


}


#endif //RPTOGETHER_SERVER_INPUTOUTPUTINTERFACE_HPP
