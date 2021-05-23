#include <Minigames-Services/AxisIterator.hpp>

#include <cassert>


namespace MinigamesServices {


// operator+ cannot be defined as a static member, so moves() is used instead
Coordinates AxisIterator::DirectionVector::moves(const Coordinates& from) const {
    return { from.line + y, from.column + x };
}


void AxisIterator::initializeAxis(const Grid& grid, const Coordinates& from, const Coordinates& to,
                                  std::initializer_list<AxisType> allowed_directions) {

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
        if (mutable_grid_) // Const_cast allowed as mutable_grid_ flag means grid ref is from mutable grid constructor
            axis_.push_back({ square, const_cast<Square&>(grid[square]) });
        else
            const_axis_.push_back({ square, grid[square] });

        // If destination has been reach inside axis
        if (square == to) {
            assert(scan_pos != 0); // It must be impossible as it would mean that from == to
            destination_pos_ = scan_pos;
        }

        scan_pos++;
    }
}

AxisIterator::AxisIterator(Grid& grid, const Coordinates& from, const Coordinates& to,
                           const std::initializer_list<AxisType> allowed_directions) :
        mutable_grid_ { true }, direction_ { axisBetween(from, to) }, current_pos_ { 0 } {

    initializeAxis(grid, from, to, allowed_directions);
}

AxisIterator::AxisIterator(const Grid& grid, const Coordinates& from, const Coordinates& to,
                           const std::initializer_list<AxisType> allowed_directions) :
        mutable_grid_ { false }, direction_ { axisBetween(from, to) }, current_pos_ { 0 } {

    initializeAxis(grid, from, to, allowed_directions);
}

AxisType AxisIterator::direction() const {
    return direction_;
}

Coordinates AxisIterator::currentPosition() const {
    return axis_.at(current_pos_).position;
}

bool AxisIterator::hasNext() const {
    std::size_t elements_count;
    // Elements container depending on mutable flag
    if (mutable_grid_)
        elements_count = axis_.size();
    else
        elements_count = const_axis_.size();

    // If current position isn't at the axis last square, then it can move to next position
    return (current_pos_ + 1) < elements_count;
}

int AxisIterator::distanceFromDestination() const {
    return static_cast<int>(current_pos_ - destination_pos_);
}

Square& AxisIterator::moveForward() {
    if (!mutable_grid_) // Checks for grid mutability as non-const ref is returned
        throw BadMutableFlag { true };

    if (!hasNext()) // Can the iterator be moved forward onto coordinates ?
        throw BadCoordinates { "End of axis reached" };

    current_pos_++; // Go te next coordinates inside iterated axis

    // Get square state for next square inside iterated axis inside mutable grid
    return axis_.at(current_pos_).state;
}

const Square& AxisIterator::moveForwardImmutable() {
    if (!mutable_grid_) // Checks for grid mutability as non-const ref is returned
        throw BadMutableFlag { false };

    if (!hasNext()) // Can the iterator be moved forward onto coordinates ?
        throw BadCoordinates { "End of axis reached" };

    current_pos_++; // Go te next coordinates inside iterated axis

    // Get square state for next square inside iterated axis inside immutable (const) grid
    return const_axis_.at(current_pos_).state;
}

}