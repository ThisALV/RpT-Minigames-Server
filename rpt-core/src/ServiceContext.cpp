#include <RpT-Core/ServiceContext.hpp>


namespace RpT::Core {


ServiceContext::ServiceContext() : events_count_ { 0 } {}

std::size_t ServiceContext::newEventPushed() {
    return events_count_++;
}


}
