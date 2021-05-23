#ifndef RPT_MINIGAMES_SERVICES_AXIS_ITERATOR_HPP
#define RPT_MINIGAMES_SERVICES_AXIS_ITERATOR_HPP

/**
 * @file AxisIterator.hpp
 */

#include <Minigames-Services/Grid.hpp>


namespace MinigamesServices {


/**
 * @brief Represents a direction from one square to another inside a grid using bitflags
 *
 * - 1 = Up
 * - 2 = Down
 * - 4 = Left
 * - 8 = Right
 */
enum struct AxisType : unsigned int {
    Up      = 0b1000,       Down       = 0b0100,
    Left    = 0b0010,       Right      = 0b0001,
    UpLeft  = Up | Left,    DownRight  = Down | Right,
    UpRight = Up | Right,   DownLeft   = Down | Left
};


/**
 * @brief Checks if given axis has enabled the given bit flag
 *
 * @param axis Axis to check for
 * @param direction Direction to check if axis is toward it or not
 *
 * @returns `true` if `axis` is toward `direction`, `false` otherwise
 */
constexpr bool hasFlagOf(const AxisType axis, const AxisType direction) {
    // Checks for direction to be a single-bit flags (and not a flags combination)
    assert(direction == AxisType::Up || direction == AxisType::Down ||
           direction == AxisType::Left || direction == AxisType::Right);

    const unsigned int axis_flags { static_cast<unsigned int>(axis) };
    const unsigned int direction_flags { static_cast<unsigned int>(direction) };

    return (axis_flags & direction_flags) != 0b0000;
}


/**
 * @brief Iterates over orthogonal or diagonal axis linking one square inside a `Grid` to another
 *
 * Iterator will go forward for each square inside the axis between from and to coordinates until there is no more
 * square because end-of-grid has been reached.
 *
 * @note This axis is a view, an interface of a given grid, it doesn't have any copy of squares inside the grid.
 *
 * @author ThisALV, https://github.com/ThisALV/
 */
class AxisIterator {
private:
    /// Elements iterated by this class, composed of a square inside this axis, and its position inside ctor grid
    struct IteratedElement {
        Coordinates position;
        Square& state;
    };

    /// Direction to move the iterator coordinates forward
    struct DirectionVector {
        int x;
        int y;

        /// Retrieves the given coordinates if moved by current vector
        Coordinates moves(const Coordinates& from) const;
    };

    /// Facility to retrieve direction from `DIRECTION_VECTORS_`
    static constexpr DirectionVector directionFor(const AxisType axis) {
        DirectionVector direction { 0, 0 }; // Without flags, a direction vector is a null vector: no direction

        // If any up/down mutually exclusives flags is enabled, then Y component is set depending on flag
        if (hasFlagOf(axis, AxisType::Up))
            direction.y = -1;
        else if (hasFlagOf(axis, AxisType::Down))
            direction.y = 1;

        // Same thing for left/right flags
        if (hasFlagOf(axis, AxisType::Left))
            direction.x = -1;
        else if (hasFlagOf(axis, AxisType::Right))
            direction.x = 1;

        return direction;
    }

    /// Because std::abs() if from C Standard which doesn't provides constexpr
    static constexpr int abs(const int x) {
        return x >= 0 ? x : -x;
    }

    /// Calculates the axis linking two given squares
    static constexpr AxisType axisBetween(const Coordinates& from, const Coordinates& to) {
        const auto [from_y, from_x] { from };
        const auto [to_y, to_x] { to };

        // Calculates differences between the two squares coordinates
        const int relative_x { static_cast<int>(to_x - from_x) };
        const int relative_y { static_cast<int>(to_y - from_y) };

        // Axis must either be diagonal or orthogonal
        if (abs(relative_x) != abs(relative_y) && relative_x != 0 && relative_y != 0)
            throw BadCoordinates { "No orthogonal or diagonal axis linking these two squares" };

        // Appropriate flags will be enabled when differences will have been compared
        unsigned int axis_flags { 0b0000 };

        if (relative_x > 0) // Right
            axis_flags |= static_cast<unsigned int>(AxisType::Right);
        else if (relative_x < 0) // Left
            axis_flags |= static_cast<unsigned int>(AxisType::Left);

        if (relative_y > 0) // Down
            axis_flags |= static_cast<unsigned int>(AxisType::Down);
        else if (relative_y < 0) // Up
            axis_flags |= static_cast<unsigned int>(AxisType::Up);

        // We're sure with the previous check that there is an diagonal or an orthogonal axis between these two squares
        return static_cast<AxisType>(axis_flags);
    }

    const AxisType direction_;

    std::vector<IteratedElement> axis_;
    std::size_t current_pos_;
    std::size_t destination_pos_;

public:
    /// 8 diagonal AND orthogonal directions
    static constexpr std::initializer_list<AxisType> EVERY_DIRECTION {
            AxisType::Up, AxisType::Down, AxisType::Left, AxisType::Right,
            AxisType::UpLeft, AxisType::DownRight, AxisType::UpRight, AxisType::DownLeft
    };

    /// 4 orthogonal directions
    static constexpr std::initializer_list<AxisType> EVERY_ORTHOGONAL_DIRECTION {
            AxisType::Up, AxisType::Down, AxisType::Left, AxisType::Right
    };

    /// 4 diagonal directions
    static constexpr std::initializer_list<AxisType> EVERY_DIAGONAL_DIRECTION {
            AxisType::UpLeft, AxisType::DownRight, AxisType::UpRight, AxisType::DownLeft
    };

    /**
     * @brief Constructs axis linking square at `from` coordinates to square at `to` coordinates
     *
     * @param grid Grid to get squares references from
     * @param from Coordinates for the player current pawn
     * @param to Coordinates for the player destination square
     * @param allowed_directions Directions into which the player can move
     *
     * @throws BadCoordinates if there isn't any axis linking `from` and `to` squares, or if one of these squares
     * doesn't exist inside given `grid`
     */
    explicit AxisIterator(Grid& grid, const Coordinates& from, const Coordinates& to,
                          std::initializer_list<AxisType> allowed_directions = EVERY_DIRECTION);

    /**
     * @brief Retrieves calculated axis iterator direction
     *
     * @returns `AxisType` representing forward direction of this iterator
     */
    AxisType direction() const;

    /**
     * @brief Retrieves position of current square inside grid
     *
     * @returns Coordinates for square at current iterator position
     */
    Coordinates currentPosition() const;

    /**
     * @brief Checks if there is any square remaining in that direction
     *
     * @returns `true` if there next position into current axis is inside grid, `false` otherwise
     */
    bool hasNext() const;

    /**
     * @brief Retrieves the relative position from destination square
     *
     * @returns Number of squares between from and to +1, will be negative if destination hasn't been passed yet
     */
    int distanceFromDestination() const;

    /**
     * @brief Moves iterator current position to next square inside axis
     *
     * @returns Square state at iterator new position
     *
     * @throws BadCoordinates if `hasNext()` returns `false` (no longer square inside axis)
     */
    Square& moveForward();
};


}


#endif //RPT_MINIGAMES_SERVICES_AXIS_ITERATOR_HPP
