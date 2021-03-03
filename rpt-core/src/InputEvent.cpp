#include <RpT-Core/InputEvent.hpp>

#include <cassert>


namespace RpT::Core {

/*
 * Base class
 */

InputEvent::InputEvent(uint64_t actor) : actor_ { actor } {}

std::uint64_t InputEvent::actor() const {
    return actor_;
}

/*
 * None
 */

NoneEvent::NoneEvent(uint64_t actor) : InputEvent { actor } {}

/*
 * ServiceRequest
 */

ServiceRequestEvent::ServiceRequestEvent(uint64_t actor, std::string service_request) :
    InputEvent { actor }, service_request_ { std::move(service_request) } {}

const std::string& ServiceRequestEvent::serviceRequest() const {
    return service_request_;
}

/*
 * TimerTrigger
 */

TimerEvent::TimerEvent(uint64_t actor) : InputEvent { actor } {}

/*
 * StopRequest
 */

StopEvent::StopEvent(uint64_t actor, std::uint8_t caught_signal) :
    InputEvent { actor }, caught_signal_ { caught_signal } {}

std::uint8_t StopEvent::caughtSignal() const {
    return caught_signal_;
}

/*
 * PlayerJoined
 */

JoinedEvent::JoinedEvent(uint64_t actor) : InputEvent { actor } {}

/*
 * PlayerLeft
 */

LeftEvent::LeftEvent(uint64_t actor, Reason disconnection_reason,
                     std::optional<std::string> err_msg) :
    InputEvent { actor }, disconnection_reason_ { disconnection_reason }, error_message_ { std::move(err_msg) } {

    // Error message must be present IF AND ONLY IF disconnection disconnectionReason is a crash
    assert((disconnection_reason_ == Reason::Crash) == error_message_.has_value());
}

LeftEvent::Reason LeftEvent::disconnectionReason() const {
    return disconnection_reason_;
}

const std::string& LeftEvent::errorMessage() const {
    if (!error_message_) // Disconnection reason must be an error if error_message_ is accessed
        throw NotAnErrorReason {};

    return *error_message_; // If it an error, retrieves message
}


}
