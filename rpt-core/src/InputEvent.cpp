#include <RpT-Core/InputEvent.hpp>

#include <cassert>


namespace RpT::Core {

/*
 * Base class
 */

InputEvent::InputEvent(const std::string_view actor) : actor_ { actor } {}

std::optional<std::string> InputEvent::additionalData() const {
    return {};
}

std::string_view InputEvent::actor() const {
    return actor_;
}

/*
 * None
 */

NoneEvent::NoneEvent(std::string_view actor) : InputEvent { actor } {}

InputEvent::Type NoneEvent::type() const {
    return InputEvent::Type::None;
}

/*
 * ServiceRequest
 */

ServiceRequestEvent::ServiceRequestEvent(std::string_view actor, std::string service_request) :
    InputEvent { actor }, service_request_ { std::move(service_request) } {}

InputEvent::Type ServiceRequestEvent::type() const {
    return InputEvent::Type::ServiceRequest;
}

std::optional<std::string> ServiceRequestEvent::additionalData() const {
    return { service_request_ };
}

/*
 * TimerTrigger
 */

TimerEvent::TimerEvent(const std::string_view actor) : InputEvent { actor } {}

InputEvent::Type TimerEvent::type() const {
    return InputEvent::Type::TimerTrigger;
}

/*
 * StopRequest
 */

StopEvent::StopEvent(std::string_view actor, std::uint8_t caught_signal) :
    InputEvent { actor }, caught_signal_ { caught_signal } {}

InputEvent::Type StopEvent::type() const {
    return InputEvent::Type::StopRequest;
}

std::optional<std::string> StopEvent::additionalData() const {
    return std::to_string(caught_signal_);
}

/*
 * PlayerJoined
 */

JoinedEvent::JoinedEvent(std::string_view actor) : InputEvent { actor } {}

InputEvent::Type JoinedEvent::type() const {
    return InputEvent::Type::PlayerJoined;
}

/*
 * PlayerLeft
 */

LeftEvent::LeftEvent(std::string_view actor, Reason disconnection_reason,
                     const std::optional<std::string_view>& err_msg) :
    InputEvent { actor }, disconnection_reason_ { disconnection_reason }, error_message_ { err_msg } {

    // Error message must be present IF AND ONLY IF disconnection reason is a crash
    assert((disconnection_reason_ == Reason::Crash) == error_message_.has_value());
}

InputEvent::Type LeftEvent::type() const {
    return InputEvent::Type::PlayerLeft;
}

std::optional<std::string> LeftEvent::additionalData() const {
    switch (disconnection_reason_) {
    case Reason::Clean:
        return "Clean";
    case Reason::Crash:
        // Copy is necessary for additional data construction
        const std::string err_message_copy { *error_message_ };

        return "Crash;" + err_message_copy;
    }
}


}
