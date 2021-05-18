#include <Minigames-Services/AxisIterator.hpp>

#include <cassert>


namespace MinigamesServices {


// operator+ cannot be defined as a static member, so moves() is used instead
Coordinates AxisIterator::DirectionVector::moves(const Coordinates& from) const {
    return { from.line + y, from.column + x };
}


AxisIterator::AxisIterator(Grid& grid, const Coordinates& from, const Coordinates& to,
                           std::initializer_list<AxisType> allowed_directions) :
        direction_ { axisBetween(from, to) },
        current_pos_ { 0 } {

    if (!grid.isInsideGrid(from) || !grid.isInsideGrid(to)) // Every square in the axis must be inside the grid
        throw BadCoordinates { "Both of the two squares forming the axis must be inside grid" };

    // Checks for the direction between the two given squares inside grid to be allowed
    if (std::find(allowed_directions.begin(), allowed_directions.end(), direction_) == allowed_directions.end())
        throw BadCoordinates { "Direction between origin and destination isn't allowed" };

    const DirectionVector axis_vector { directionFor(direction_) };

    std::size_t scan_pos { 0 };
    // Moves coordinates with axis vector to scan for every square into this axis inside given grid
    for (Coordinates square { from }; grid.isInsideGrid(square); square = axis_vector.moves(square)) {
        // Adds next square inside this axis
        axis_.emplace_back(grid[square]);

        // If destination has been reach inside axis
        if (square == to) {
            assert(scan_pos != 0); // It must be impossible as it would mean that from == to
            destination_pos_ = scan_pos;
        }

        scan_pos++;
    }
}

AxisType AxisIterator::direction() const {
    return direction_;
}

bool AxisIterator::hasNext() const {
    // If current position isn't at the axis last square, then it can move to next position
    return current_pos_ < axis_.size();
}

int AxisIterator::distanceFromDestination() const {
    return static_cast<int>(current_pos_ - destination_pos_);
}

Square& AxisIterator::moveForward() {
    if (!hasNext()) // Can the iterator be moved forward onto coordinates ?
        throw BadCoordinates { "End of axis reached" };

    current_pos_++; // Go te next coordinates inside iterated axis

    // Get square state for next square inside iterated axis
    return axis_.at(current_pos_).get();
}

}