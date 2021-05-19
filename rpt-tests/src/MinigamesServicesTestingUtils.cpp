#include <RpT-Testing/MinigamesServicesTestingUtils.hpp>

#include <iostream> // Required for operator<< with string literals (const char*)


namespace MinigamesServices {


std::ostream& operator<<(std::ostream& out, Square square_state) {
    if (square_state == Square::Free)
        return out << "Free";
    else if (square_state == Square::White)
        return out << "White";
    else
        return out << "Black";
}


}
