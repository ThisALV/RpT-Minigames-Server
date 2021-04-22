#include <RpT-Core/Timer.hpp>


namespace RpT::Core {


void Timer::checkStateForOperation(const std::string_view operation_name) {
    TimerState expected_state { getExpectedState(operation_name) }; // Retrieves expected state for current operation

    if (expected_state != current_state_) // Checks for current state to match with expected one
        throw BadTimerState { std::string { operation_name }, expected_state, current_state_ };
}

Timer::Timer(ServiceContext& token_provider, const std::size_t countdown_ms)
: token_ { token_provider.newTimerCreated() }, countdown_ms_ { countdown_ms }, current_state_ { TimerState::Disabled } {}

std::uint64_t Timer::token() const {
    return token_;
}

std::size_t Timer::countdown() const {
    return countdown_ms_;
}

bool Timer::isFree() {
    return current_state_ == TimerState::Disabled;
}

bool Timer::isWaitingCountdown() const {
    return current_state_ == TimerState::Ready;
}

bool Timer::isPending() const {
    return current_state_ == TimerState::Pending;
}

bool Timer::hasTriggered() const {
    return current_state_ == TimerState::Triggered;
}

void Timer::clear() {
    checkStateForOperation("clear");
    current_state_ = TimerState::Disabled;
}

void Timer::requestCountdown() {
    checkStateForOperation("requestCountdown");
    current_state_ = TimerState::Ready;
}

std::size_t Timer::beginCountdown() {
    checkStateForOperation("beginCountdown");
    current_state_ = TimerState::Pending;

    return countdown(); // Caller must known how many time it should be pending
}

void Timer::trigger() {
    checkStateForOperation("trigger");
    current_state_ = TimerState::Triggered;
}


}
