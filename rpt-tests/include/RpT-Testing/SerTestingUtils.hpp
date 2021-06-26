#ifndef RPT_MINIGAMES_SERVER_SERTESTINGUTILS_HPP
#define RPT_MINIGAMES_SERVER_SERTESTINGUTILS_HPP

#include <RpT-Core/ServiceEvent.hpp>


namespace RpT::Core {


/**
 * As `ServiceEvent` is an Entity Class, testing equality with it only make sense on a testing content to checks for
 * its field values
 *
 * @param lhs Left-Hand side event
 * @param rhs Right-Hand side event
 *
 * @returns `true` if command and optional UIDs list are equals for both `lhs` and `rhs`, `false` otherwise
 */
bool operator==(const ServiceEvent& lhs, const ServiceEvent& rhs);

/**
 * Prints event command data in quotes then a comma-separated list of actor UIDs
 *
 * @param out Stream to be written
 * @param event Event to write
 *
 * @returns Written `out`
 */
std::ostream& operator<<(std::ostream& out, const ServiceEvent& event);


}


#endif //RPT_MINIGAMES_SERVER_SERTESTINGUTILS_HPP
