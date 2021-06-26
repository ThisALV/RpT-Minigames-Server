#include <RpT-Testing/SerTestingUtils.hpp>

#include <iostream>


namespace RpT::Core {


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
