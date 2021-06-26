#include <RpT-Core/ServiceEvent.hpp>


namespace RpT::Core {


ServiceEvent::ServiceEvent(std::string command, std::optional<std::unordered_set<std::uint64_t>> actor_uids)
: targets_ { std::move(actor_uids) }, command_ { std::move(command) } {}

bool ServiceEvent::operator==(const ServiceEvent& rhs) const {
    return command_ == rhs.command_ && targets_ == rhs.targets_;
}

bool ServiceEvent::operator!=(const ServiceEvent& rhs) const {
    return !(*this == rhs);
}

ServiceEvent ServiceEvent::prefixWith(const std::string& higher_protocol_prefix) const {
    return ServiceEvent { higher_protocol_prefix + command_, targets_ };
}

std::string_view ServiceEvent::command() const {
    return command_;
}

bool ServiceEvent::targetEveryone() const {
    return !targets_.has_value(); // No UIDs list means every actor receives it
}

const std::unordered_set<std::uint64_t>& ServiceEvent::targets() const {
    if (targetEveryone()) // Checks for a UIDs list to be available
        throw NoUidsList {};

    return *targets_;
}


}
