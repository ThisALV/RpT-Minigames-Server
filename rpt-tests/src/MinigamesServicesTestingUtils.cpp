#include <RpT-Testing/MinigamesServicesTestingUtils.hpp>

#include <iostream> // Required for operator<< with string literals (const char*)


namespace MinigamesServices {


std::ostream& operator<<(std::ostream& out, const Square square_state) {
    if (square_state == Square::Free)
        return out << "Free";
    else if (square_state == Square::White)
        return out << "White";
    else
        return out << "Black";
}

std::ostream& operator<<(std::ostream& out, const Player player) {
    if (player == Player::White)
        return out << "White";
    else
        return out << "Black";
}

std::ostream& operator<<(std::ostream& out, const Coordinates& coordinates) {
    return out << "Line=" << std::to_string(coordinates.line) << " Col=" << std::to_string(coordinates.column);
}


}
