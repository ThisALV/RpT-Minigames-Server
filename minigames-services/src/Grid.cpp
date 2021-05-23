#include <Minigames-Services/Grid.hpp>


namespace MinigamesServices {


Grid::Grid(const std::initializer_list<std::initializer_list<Square>> initial_configuration) {
    if (std::empty(initial_configuration) || std::empty(*initial_configuration.begin()))
        throw BadDimensions { "Zero dimension for height or width isn't allowed" };

    // Copies dimensions from given game board
    const std::size_t lines_count { initial_configuration.size() };
    // Other line dimension may differ, but we're sure there is at least one line and one column
    const std::size_t expected_columns_count { initial_configuration.begin()->size() };

    if (lines_count > 26 || expected_columns_count > 26)
        throw BadDimensions { "A grid dimension cannot exceed 26" };

    squares_matrix_.reserve(lines_count);

    // For each line inside given game board matrix
    for (const std::initializer_list<Square>& line : initial_configuration) {
        const std::size_t actual_columns_count { line.size() };

        if (actual_columns_count != expected_columns_count) // Checks for each line dimension to be the same
            throw BadDimensions { "Every line must have the same number of columns" };

        squares_matrix_.emplace_back(line); // Pushes new line into matrix
    }
}

bool Grid::isInsideGrid(const Coordinates& coords) const {
    const int lines_count { static_cast<int>(squares_matrix_.size()) };
    const int columns_count { static_cast<int>(squares_matrix_.at(0).size()) };
    const auto [line, col] { coords };

    // Checks if line number if not above lines count, same for column number
    return line > 0 && line <= lines_count && col > 0 && col <= columns_count;
}

Square& Grid::operator[](const Coordinates& coords) {
    // Required to call const-qualified subscript operator
    const Grid& constant_this { const_cast<const Grid&>(*this) };

    // *this was initially non-const, so const_coast is exceptionally allowed here
    return const_cast<Square&>(constant_this[coords]);
}

const Square& Grid::operator[](const Coordinates& coords) const {
    if (!isInsideGrid(coords)) // Checks for a square to be associated with given coordinates
        throw BadCoordinates { "These coordinates aren't inside grid" };

    // We've already checked that a square exists for these coordinates, converts line/column numbers into indexes
    // with -1
    return squares_matrix_.at(coords.line - 1).at(coords.column - 1);
}


}
