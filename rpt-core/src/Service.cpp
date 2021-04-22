#include <RpT-Core/Service.hpp>


namespace RpT::Core {


Service::Service(ServiceContext& run_context,
                 const std::initializer_list<std::reference_wrapper<Timer>>& watched_timers)
                 : run_context_ { run_context }, watched_timers_ { watched_timers } {}

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

    // Copies each watched timer which is waiting for IO interface to begin countdown, using push_back()
    std::copy_if(watched_timers_.cbegin(), watched_timers_.cend(), std::back_inserter(ready_timers),
                 [](const Timer& t) { return t.isWaitingCountdown(); });

    return ready_timers;
}


}
