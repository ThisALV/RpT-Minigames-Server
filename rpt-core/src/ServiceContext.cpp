#include <RpT-Core/ServiceContext.hpp>


namespace RpT::Core {


ServiceContext::ServiceContext() : events_count_ { 0 }, timers_count_ { 0 } {}

std::size_t ServiceContext::newEventPushed() {
    return events_count_++;
}

std::size_t ServiceContext::newTimerCreated() {
    return timers_count_++;
}


}
