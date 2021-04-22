#ifndef RPT_MINIGAMES_SERVER_TIMER_HPP
#define RPT_MINIGAMES_SERVER_TIMER_HPP

#include <stdexcept>
#include <string>
#include <string_view>
#include <RpT-Core/ServiceContext.hpp>

/**
 * @file Timer.hpp
 */


namespace RpT::Core {

/// State used internally by `Timer`, also used to format `BadTimerState` error message
enum struct TimerState : int {
    Disabled, Ready, Pending, Triggered
};


/**
 * @brief Thrown by `Timer` methods if an operation is applied while instance state does not allow it
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class BadTimerState : public std::logic_error {
public:
    BadTimerState(const std::string& operation, TimerState expected, TimerState current)
    : std::logic_error {
        operation + ": expected state " + std::to_string(static_cast<int>(expected))
        + ", current state is " + std::to_string(static_cast<int>(current))
    } {}
};


/**
 * @brief %Timer described by an unique token provided by a `ServiceContext` and a countdown in milliseconds
 *
 * Each timer has a state which is either disabled, ready, pending or triggered.
 *
 * Here is what a timer lifecycle looks like:
 * - Disabled: waiting for owning Service to signal it wants to run the timer, set state to Ready
 * - Ready: waiting for `InputOutputInterface` to handle all ready timers at loop end, will set state to Pending
 * - Pending: `InputOutputInterface` implementation began countdown, waiting for `TimerEvent` to be emitted, will set
 * state to Triggered
 * - Triggered: `TimerEvent` with corresponding token emitted, countdown is done and timer can be cleared to Disabled
 * again
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class Timer {
private:
    /// Retrieves expected state for given operation name
    static constexpr TimerState getExpectedState(const std::string_view operation_name) {
        // Checks for each possible operation
        if (operation_name == "clear")
            return TimerState::Triggered;
        else if (operation_name == "requestCountdown")
            return TimerState::Disabled;
        else if (operation_name == "beginCountdown")
            return TimerState::Ready;
        else if (operation_name == "trigger")
            return TimerState::Pending;
        else // Other operations are invalid
            throw std::invalid_argument { "Operation " + std::string { operation_name } + " doesn't exist" };
    }

    std::uint64_t token_;
    std::size_t countdown_ms_;
    TimerState current_state_;

    /// Throws `BadTimerState` if current state is not the one expected for given operation name
    void checkStateForOperation(std::string_view operation_name);

public:
    /**
     * @brief Constructs timer object with token provided by given `ServiceContext`, with given countdown, in disabled
     * state
     *
     * @param token_provider `ServiceContext` of service using and owning this timer
     * @param countdown_ms Time in milliseconds to stay into pending mode once countdown has begun
     */
    Timer(ServiceContext& token_provider, std::size_t countdown_ms);

    // Entity semantic class

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    Timer(Timer&&) = default;
    Timer& operator=(Timer&&) = default;

    /**
     * @brief Retrieves timer token
     *
     * @returns Timer token
     */
    std::uint64_t token() const;

    /**
     * @brief Retrieves timer countdown in milliseconds
     *
     * @returns Timer countdown in milliseconds
     */
    std::size_t countdown() const;

    /// Checks if current state is Disabled
    bool isFree();
    /// Checks if current state is Ready
    bool isWaitingCountdown() const;
    /// Checks if current state is Pending
    bool isPending() const;
    /// Checks if current state is Triggered
    bool hasTriggered() const;

    /**
     * @brief Marks timer as Disabled
     *
     * @throws BadTimerState if timer is not Triggered
     */
    void clear();

    /**
     * @brief Marks timer as Ready
     *
     * @throws BadTimerState if timer is not Ready
     */
    void requestCountdown();

    /**
     * @brief Marks timer as Pending
     *
     * @returns Timer countdown in milliseconds
     *
     * @throws BadTimerState if timer is not Ready
     */
    std::size_t beginCountdown();

    /**
     * @brief Marks timer as Triggered
     *
     * @throws BadTimerState if timer is not Pending
     */
    void trigger();
};


}


#endif //RPT_MINIGAMES_SERVER_TIMER_HPP
