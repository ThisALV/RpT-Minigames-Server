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
    return out << "Line=" << coordinates.line << " Col=" << coordinates.column;
}

std::ostream& operator<<(std::ostream& out, const SquareUpdate& square_update) {
    return out << "Coords={" << square_update.square << "} State=" << square_update.updatedState;
}


}
