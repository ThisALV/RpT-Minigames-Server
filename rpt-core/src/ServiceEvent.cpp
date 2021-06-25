#include <RpT-Core/ServiceEvent.hpp>


namespace RpT::Core {


ServiceEvent::ServiceEvent(std::string command, std::optional<std::initializer_list<std::uint64_t>> actor_uids)
: command_ { std::move(command) }, targets_ { actor_uids } {}

std::string_view ServiceEvent::command() const {
    return command_;
}

bool ServiceEvent::targetEveryone() const {
    return !targets_.has_value(); // No UIDs list means every actor receives it
}

const std::vector<std::uint64_t>& ServiceEvent::targets() const {
    if (targetEveryone()) // Checks for a UIDs list to be available
        throw NoUidsList();

    return *targets_;
}


}
