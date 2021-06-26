#ifndef RPT_MINIGAMES_SERVER_SERTESTINGUTILS_HPP
#define RPT_MINIGAMES_SERVER_SERTESTINGUTILS_HPP

#include <RpT-Core/ServiceEvent.hpp>


namespace RpT::Core {


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
