#include <RpT-Core/InputEvent.hpp>


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

JoinedEvent::JoinedEvent(const std::uint64_t new_actor_uid, const std::string_view new_actor_name) :
InputEvent { new_actor_uid }, new_actor_name_ { new_actor_name } {}

std::string_view JoinedEvent::playerName() const {
    return new_actor_name_;
}

/*
 * PlayerLeft
 */

LeftEvent::LeftEvent(const std::uint64_t actor, const LeftEvent::Reason reason) :
InputEvent { actor }, disconnection_reason_ { reason } {}

LeftEvent::Reason LeftEvent::disconnectionReason() const {
    return disconnection_reason_;
}


}
