#ifndef RPT_MINIGAMES_SERVER_MINIGAMESSERVICESTESTINGUTILS_HPP
#define RPT_MINIGAMES_SERVER_MINIGAMESSERVICESTESTINGUTILS_HPP

#include <Minigames-Services/BoardGame.hpp>
#include <Minigames-Services/Grid.hpp>

/*
 * Testing functions utilities for minigames-services static library unit tests
 */


// Required for ADL to detect operator<< for Square inside MinigamesServices namespace
namespace MinigamesServices {


/// Required by BOOST_CHECK_EQUAL macro usage with Square enum type
std::ostream& operator<<(std::ostream& out, Square square_state);
/// Required by BOOST_CHECK_EQUAL macro usage with Player enum type
std::ostream& operator<<(std::ostream& out, Player player);
/// Required by BOOST_CHECK_EQUAL macro usage with Coordinates type, format: "Line=<line> Col=<column>"
std::ostream& operator<<(std::ostream& out, const Coordinates& coordinates);

}


#endif //RPT_MINIGAMES_SERVER_MINIGAMESSERVICESTESTINGUTILS_HPP
