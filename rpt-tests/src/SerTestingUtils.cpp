#include <RpT-Testing/SerTestingUtils.hpp>

#include <iostream>


namespace RpT::Core {


bool operator==(const ServiceEvent& lhs, const ServiceEvent& rhs) {
    // First, checks for commands to be equals
    if (lhs.command() != rhs.command())
        return false;

    const bool lhs_targets_everyone { lhs.targetEveryone() };
    const bool rhs_targets_everyone { rhs.targetEveryone() };

    // Second, they must both be targeting everyone or both not targeting everyone
    if (lhs_targets_everyone != rhs_targets_everyone)
        return false;

    // Finally, if they are not targeting everyone, checks for the selected actors because targets() will not throw then
    if (!lhs_targets_everyone && lhs.targets() == rhs.targets())
        return false;

    return true;
}

std::ostream& operator<<(std::ostream& out, const ServiceEvent& event) {
    out << '"' << event.command() << "\" -> "; // Prints quoted command

    if (event.targetEveryone()) {
        out << '*'; // Asterisk means "Every actor"
    } else {
        // Prints each selected actor UID
        for (const std::uint64_t actor_uid : event.targets()) {
            out << actor_uid << ", ";
        }
    }

    return out;
}


}
