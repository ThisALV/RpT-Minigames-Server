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
