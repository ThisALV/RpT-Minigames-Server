#include <RpT-Core/Service.hpp>


namespace RpT::Core {


Service::Service(ServiceContext& run_context,
                 const std::initializer_list<std::reference_wrapper<Timer>> watched_timers) :
                 run_context_ { run_context } {

    for (Timer& timer_to_watch : watched_timers) // Safe method with checks to watch each required timer
        watchTimer(timer_to_watch);
}

void Service::watchTimer(Timer& timer_to_watch) {
    // Watches timer from address obtained with given reference
    const auto insertion_result { watched_timers_.insert(&timer_to_watch) };

    if (!insertion_result.second) // If insertion failed because it is already watched...
        throw BadWatchedToken { timer_to_watch, "Already watched" };
}

void Service::forgetTimer(Timer& watched_timer) {
    // Removes timer using address obtained from given reference
    const std::size_t removed_count { watched_timers_.erase(&watched_timer) };

    if (removed_count != 1) // If given timer have NOT been removed because it isn't watched...
        throw BadWatchedToken { watched_timer, "Not watched" };
}

void Service::emitEvent(std::string event_command) {
    const std::size_t event_id { run_context_.newEventPushed() }; // Event counter is growing, ID is given so trigger order is kept

    events_queue_.push({ event_id, std::move(event_command) }); // Event command moved in queue with appropriate ID
}

std::optional<std::size_t> Service::checkEvent() const {
    return events_queue_.empty() ? EMPTY_QUEUE : events_queue_.front().first;
}

std::string Service::pollEvent() {
    if (events_queue_.empty()) // There must be at least one event to poll, checked with checkEvent() call
        throw EmptyEventsQueue { name() };

    // Command is moved from queue to local, and will be returned by copy-elision later
    std::string event_command { std::move(events_queue_.front().second) };
    // Event with moved command can now be destroyed
    events_queue_.pop();

    return event_command;
}

std::vector<std::reference_wrapper<Timer>> Service::getWaitingTimers() {
    std::vector<std::reference_wrapper<Timer>> ready_timers;
    ready_timers.reserve(watched_timers_.size()); // Max number of ready timers is number of current timers

    // Copies reference for each watched timer which is waiting for IO interface to begin countdown
    for (Timer* watched_timer : watched_timers_) {
        if (watched_timer->isWaitingCountdown()) // Filtering to get timers waiting for their countdown to begin
            ready_timers.emplace_back(*watched_timer);
    }

    return ready_timers;
}


}
