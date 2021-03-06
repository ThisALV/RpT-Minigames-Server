#ifndef RPTOGETHER_SERVER_SERVICE_HPP
#define RPTOGETHER_SERVER_SERVICE_HPP

#include <functional>
#include <optional>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <RpT-Core/ServiceContext.hpp>
#include <RpT-Core/ServiceEvent.hpp>
#include <RpT-Core/Timer.hpp>
#include <RpT-Utils/HandlingResult.hpp>

/**
 * @file Service.hpp
 */


namespace RpT::Core {


/**
 * @brief Thrown if trying to poll event when queue is empty
 *
 * @see Service
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class EmptyEventsQueue : public std::logic_error {
public:
    /**
     * @brief Constructs error with basic message including queue service name
     *
     * @param service Name of service the queue belongs to
     */
    explicit EmptyEventsQueue(const std::string_view service) :
            std::logic_error { "No more events for \"" + std::string { service } + '"' } {}
};


/// Thrown by `Service` timers-related methods when timer of a given cannot be watched or forgotten
class BadWatchedToken : public std::logic_error {
public:
    /// Constructs error with formatted message from given Timer token and given failure reason
    BadWatchedToken(const Timer& timer, const std::string& reason)
    : std::logic_error { "Timer " + std::to_string(timer.token()) + ':' + reason } {}
};


/**
 * @brief Service ran by `ServiceEventRequestProtocol`
 *
 * Service requirement is being able to handle SR commands, implementations must define `handleRequestCommand()`
 * virtual method.
 *
 * Implementations will access protected method `emitEvent()` so they can trigger events later polled by any SER
 * Protocol instance, and will uses superclass constructor arguments to watch timers so their state will be checked at
 * each %Executor loop iteration using `getWaitingTimers()`.
 *
 * Each service possesses its own events queue, and each event contains a event ID provided by `ServiceContext`, which
 * allows knowing what event was triggered first (as ID is growing from low to high) and an event command,
 * corresponding to words after `EVENT` prefix and service name inside Service Event command.
 *
 * Each service also has a references vector for watched timers which can be used by %Executor to survey any timer
 * that entered the Ready state. %Executor then will pass Ready state timers to `InputOutputInterface` implementation.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class Service {
private:
    ServiceContext& run_context_;
    std::queue<std::pair<std::size_t, ServiceEvent>> events_queue_;
    std::set<Timer*> watched_timers_; // Using pointers because reference_wrapper doesn't offer == operator

protected:
    /**
     * @brief Emits event command into service
     *
     * @param event_command Event command to emit (words coming after `EVENT` prefix and service name in SE command)
     * @param event_targets List of UIDs for actor which must receive that event. *If empty, every must receive it.*
     */
    void emitEvent(std::string event_command, std::initializer_list<std::uint64_t> event_targets = {});

public:
    /*
     * Entity class semantic
     */

    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;

    bool operator==(const Service&) const = delete;

    /// Returned by `checkEvent()` when service events queue is empty
    static constexpr std::optional<std::size_t> EMPTY_QUEUE {};

    /**
     * @brief Constructs service with empty events queue and given run context
     *
     * @param run_context Context, should be same instance for services registered in same SER Protocol
     * @param watched_timers All timers retrieved passed to `InputOutputInteface` implementation by `Executor` if in
     * Ready state
     */
    explicit Service(ServiceContext& run_context,
                     std::initializer_list<std::reference_wrapper<Timer>> watched_timers = {});

    /**
     * @brief Watches Ready state for given timer, it can now be returned by `getWatchingTimers()`
     *
     * @param timer_to_watch Disabled timer to watch for
     *
     * @throws BadTimerState if timer isn't `Disabled`
     * @throws BadWatchedToken if given timer is already watched
     */
    void watchTimer(Timer& timer_to_watch);

    /**
     * @brief Stops watching for Ready state, given timer can no longer be returned by `getWatchingTimers()`
     *
     * @param watched_timer Watched timer to stop being watching for
     *
     * @note If given timer was `Pending`, it will not be automatically cancelled.
     *
     * @throws BadWatchedToken if given timer isn't watched by this %Service
     */
    void forgetTimer(Timer& watched_timer);

    /**
     * @brief Get next event ID so check for newest event between services can be performed
     *
     * @note Called by `ServiceEventRequestProtocol` instance to perform described check, shouldn't be called by user.
     *
     * @returns ID for next queued event, or uninitialized if queue is empty
     */
    std::optional<std::size_t> checkEvent() const;

    /**
     * @brief Get next Service Event
     *
     * @note Called by `ServiceEventRequestProtocol` instance to dispatch across actors, shouldn't be called by user.
     *
     * @returns `ServiceEvent` with command and targets list for next queued event
     *
     * @throws EmptyEventsQueue if queue is empty so event cannot be polled
     */
    ServiceEvent pollEvent();

    /**
     * @brief Checks for every timers owned and watched by %Service which are waiting for their countdown to begin
     *
     * @returns Reference to every watched timer into Ready mode
     */
    std::vector<std::reference_wrapper<Timer>> getWaitingTimers();

    /**
     * @brief Get service name for registration
     *
     * @returns Service name
     */
    virtual std::string_view name() const = 0;

    /**
     * @brief Tries to handle a given command executed by a given actor
     *
     * @param actor UID for actor who's trying to execute the given SR command
     * @param sr_command_data Service Request command arguments (or command data words)
     *
     * @returns Result for command handling, evaluates to `true` is succeeded, else contains an error message
     */
    virtual Utils::HandlingResult handleRequestCommand(std::uint64_t actor, std::string_view sr_command_data) = 0;
};


}


#endif //RPTOGETHER_SERVER_SERVICE_HPP
