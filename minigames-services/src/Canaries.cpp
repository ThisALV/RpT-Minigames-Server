#include <Minigames-Services/Canaries.hpp>

#include <array>
#include <utility>


namespace MinigamesServices {


void Canaries::playNormal(GridUpdate& updates, AxisIterator move) {
    Square& destination { move.moveForward() }; // Tries to go for 1 square toward movement direction

    if (destination != Square::Free) // Target pawn should be moved to an empty square
        throw BadSquareState { "Movement destination is kept by another pawn" };

    // Applies movement modifications to grid
    game_grid_[updates.moveOrigin] = Square::Free; // Target pawn no longer into origin square
    destination = colorFor(currentRound()); // It appears into destination square
}

void Canaries::playEat(GridUpdate& updates, AxisIterator move) {
    const Player current_player { currentRound() };
    const Square current_player_color { colorFor(current_player) };

    // Tries to go for the pawn which will be jumped over, it must have the color of current player
    if (move.moveForward() != current_player_color)
        throw BadSquareState { "Jumped over square doesn't contain one of your pawns" };

    Square& eaten { move.moveForward() }; // Tries to got for the pawn which will be eaten by current player
    if (eaten != flip(current_player_color)) // Eaten pawn must belong to the opponent
        throw BadSquareState { "Movement destination doesn't contain an opponent pawn to eat" };

    // Applies movement modifications to grid
    game_grid_[updates.moveOrigin] = Square::Free;
    eaten = current_player_color; // Square where opponent's pawn is eaten now contain our moved pawn

    // Saves stats updates
    if (current_player == Player::White)
        black_pawns_--;
    else
        white_pawns_--;
}

bool Canaries::isBlocked(const Player player) const {
    const Square player_color { colorFor(player) };

    // For each square kept by given player, tests for every possible move if it is available
    for (int line { 1 }; line <= 4; line++) {
        for (int column { 1 }; column <= 4; column++) {
            const Coordinates checked_square { line, column };

            // For a move to be available from current square, it must be kept by given player
            if (game_grid_[checked_square] == player_color) {
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
                    // Possible destination for a jump move, one square after the direct neighbour calculated previously
                    const Coordinates after_neighbour { line + 2 * line_offset, column + 2 * col_offset };

                    // No move is possible if grid border is met right into current direction
                    if (!game_grid_.isInsideGrid(neighbour))
                        continue;

                    // Get content of 1st neighbour
                    const Square direct_neighbour { game_grid_[neighbour] };
                    // If it is empty, then a normal move can be performed, player isn't blocked
                    if (direct_neighbour == Square::Free)
                        return false;

                    // If it is kept by a player's own pawn, a jump/eat move might be possible if and only if next
                    // square is inside grid and is a pawn of the opponent color
                    const bool jump_eat_available {
                        direct_neighbour == player_color &&
                        game_grid_.isInsideGrid(after_neighbour) &&
                        game_grid_[after_neighbour] == flip(player_color)
                    };

                    if (jump_eat_available)
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
    AxisIterator move { game_grid_, from, to, AxisIterator::EVERY_ORTHOGONAL_DIRECTION };

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
