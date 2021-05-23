#include <Minigames-Services/Canaries.hpp>

#include <array>
#include <utility>


namespace MinigamesServices {


void Canaries::playNormal(GridUpdate& updates, AxisIterator move) {

}

void Canaries::playEat(GridUpdate& updates, AxisIterator move) {

}

bool Canaries::isBlocked(const Player player) const {
    const Square player_color { colorFor(player) };

    // For each square kept by given player, tests for every possible move if it is available
    for (int line { 1 }; line <= 4; line++) {
        for (int column { 0 }; column <= 4; column++) {
            const Coordinates checked_square { line, column };

            // For a move to be available from current square, it must be kept by given player
            if (game_grid_[checked_square] != player_color) {
                // Each orthogonal axis will be checked for available moves
                const std::array<std::pair<int, int>, 4> orthogonal_vectors {
                        std::make_pair<int, int>(1, 0),
                        std::make_pair<int, int>(-1, 0),
                        std::make_pair<int, int>(0, 1),
                        std::make_pair<int, int>(0, -1),
                };

                for (const auto& [line_offset, col_offset] : orthogonal_vectors) {
                    // Next orthogonal direction to check for
                    const Coordinates neighbour { line + line_offset, column + col_offset };

                    AxisIterator move_direction { game_grid_, checked_square, neighbour };

                    // Go to direct neighbour
                    const Square direct_neighbour { move_direction.moveForward() };
                    // If it is empty, then a normal move can be performed, player isn't blocked
                    if (direct_neighbour == Square::Free)
                        return false;

                    // If it is kept by a player's own pawn, a jump/eat move might be possible if and only if next
                    // square is a pawn of the opponent color
                    if (direct_neighbour == player_color && move_direction.moveForward() == flip(player_color))
                        return false;
                }
            }
        }
    }

    // If no move was detected as available, then player is blocked
    return true;
}

Canaries::Canaries() : BoardGame { INITIAL_GRID_, 8, 8, 2 } {}

std::optional<Player> Canaries::victoryFor() const {
    // First, checks if any player can no longer play anymore. In that case, he lose
    if (isBlocked(Player::White))
        return Player::Black;
    else if (isBlocked(Player::Black))
        return Player::White;
    else // If both players can move, checks for their pawns count compared with pawns count threshold
        return BoardGame::victoryFor();
}

bool Canaries::isRoundTerminated() const {
    return hasMoved();
}

GridUpdate Canaries::play(const Coordinates& from, const Coordinates& to) {
    // A pawn will be moved from `from` to `to`, no square update as grid isn't modified yet
    GridUpdate updates { {}, from, to };
    // Move along axis selected by player, if any
    AxisIterator move { game_grid_, from, to };

    // Origin square must contains a pawn of current player color
    if (game_grid_[from] != colorFor(currentRound()))
        throw BadSquareState { "Action target square must be kept by a pawn of current player" };

    const int move_range { -move.distanceFromDestination() }; // Destination not passed, returned distance is negative

    switch (move_range) {
    case 1: // Distance of 1: normal move
        playNormal(updates, std::move(move));
        break;
    case 2: // Distance of 2: jumps & eats move
        playEat(updates, std::move(move));
        break;
    default:
        throw BadCoordinates { "Selected squares are too far, no available move" };
    }

    // At this point, a move is performed: round can now be terminated
    moved();

    return updates;
}


}
