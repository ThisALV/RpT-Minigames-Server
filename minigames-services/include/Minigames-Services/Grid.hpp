#ifndef RPT_MINIGAMES_SERVICES_GRID_HPP
#define RPT_MINIGAMES_SERVICES_GRID_HPP

/**
 * @file Grid.hpp
 */

#include <cassert>
#include <functional>
#include <stdexcept>
#include <vector>


namespace MinigamesServices {


/// Represents coordinates of a square, for example `{ 2, 3 }` for the 3rd column inside the 2nd line
struct Coordinates {
    /// Number of square line, beginning at 1
    int line;
    /// Number of square column, beginning at 1
    int column;

    /// Checks if `line` and `column` are both equals
    constexpr bool operator==(const Coordinates& rhs) const {
        return line == rhs.line && column == rhs.column;
    }

    /// Checks if `operator==` return `false`
    constexpr bool operator!=(const Coordinates& rhs) const {
        return !(*this == rhs);
    }
};


/**
 * @brief Thrown if an operation required a specific state for a square that doesn't respect the preconditions
 */
class BadSquareState : public std::logic_error {
public:
    /// Constructs an error with provided message
    explicit BadSquareState(const std::string& reason) : std::logic_error { reason } {}
};

/**
 * @brief Thrown by `Grid` constructor when some list of columns have different lengths, or if any dimension is
 * null/zero
 */
class BadDimensions : public std::logic_error {
public:
    /// Constructs an error with provided message
    explicit BadDimensions(const std::string& reason) : std::logic_error { reason } {}
};

/**
 * @brief Thrown by an operation required specific coordinates which aren't meeting expected preconditions
 */
class BadCoordinates : public std::logic_error {
public:
    /// Constructs an error with provided error message
    explicit BadCoordinates(const std::string& reason) : std::logic_error { reason } {}
};


/**
 * @brief State of a square inside a `Grid`. Can be `Free`, or kept by a player (`Black` or `White`).
 */
enum struct Square {
    Free, Black, White
};

/**
 * @brief Get a square kept by the opponent of player which is currently
 *
 * @param currentSquare Square to flip
 *
 * @returns `White` if it was `Black`, `Black` if it was `White`.
 *
 * @throws BadSquareState if it is `None`
 */
constexpr Square flip(const Square currentSquare) {
    if (currentSquare == Square::Black)
        return Square::White;
    else if (currentSquare == Square::White)
        return Square::Black;
    else
        throw BadSquareState { "Flippable only if it is kept by a player" };
}

/// Constant shortcut for `Square::Free`
constexpr Square EMPTY { Square::Free };
/// Constant shortcut for `Square::White`
constexpr Square WHITE { Square::White };
/// Constant shortcut for `Square::Black`
constexpr Square BLACK { Square::Black };


/**
 * @brief Abstraction for a grid of squares which may contain pawns, used by minigames to make their game board
 *
 * A grid can be initialized with a specific configuration, provides access to specific axis to work easier into
 * diagonal and orthogonal bases and to check easily distance between two pawns.
 *
 * @author ThisALV, https://github.com/ThisALV/
 */
class Grid {
private:
    std::vector<std::vector<Square>> squares_matrix_;

public:
    /**
     * @brief Constructs a grid containing squares with state given by argument
     *
     * @param initial_configuration List of lines, each line is a list of columns containing squares with a specific
     * state. Each list of columns must have the same length. Length of lines list will be height of grid, length of
     * columns list will be width.
     *
     * @throws BadDimensions if `initial_configuration` isn't valid
     */
    Grid(std::initializer_list<std::initializer_list<Square>> initial_configuration);

    /**
     * @brief Checks if a square with given coordinates exists inside current grid
     *
     * @param coords Coordinates to search a square from
     *
     * @returns `true` if square exists inside grid, `false` otherwise
     */
    bool isInsideGrid(const Coordinates& coords) const;

    /**
     * @brief Retrieves square at given coordinates inside grid
     *
     * @param coords Coordinates to retrieve a square from
     *
     * @returns Reference to `Square` inside grid at given position
     *
     * @throws BadCoordinates if given `coords` are out of bound for current grid
     */
    Square& operator[](const Coordinates& coords);

    /// Same as non-const subscript operator, but with constness guarantee
    const Square& operator[](const Coordinates& coords) const;
};


}


#endif //RPT_MINIGAMES_SERVICES_GRID_HPP
