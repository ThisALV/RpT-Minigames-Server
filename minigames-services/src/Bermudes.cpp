#include <Minigames-Services/Bermudes.hpp>


namespace MinigamesServices {


void Bermudes::checkFreeTrajectory(AxisIterator& move_trajectory, const int until) {
    Square next { move_trajectory.moveForward() };
    // Iterates over each square between moved pawn and its destination
    while (move_trajectory.distanceFromDestination() != until) {
        if (next != Square::Free) { // Each squares between destination
            const auto [current_line, current_col] { move_trajectory.currentPosition() };

            throw BadSquareState {
                    "Square at " + std::to_string(current_line) + std::to_string(current_col) +
                    "inside trajectory isn't empty"
            };
        }

        // Go to next square for checking its state
        next = move_trajectory.moveForward();
    }
}

Bermudes::Bermudes() : BoardGame { INITIAL_GRID_, 27, 27, 6 } {}

void Bermudes::playElimination(GridUpdate& updates, AxisIterator move) {
    const Player current_player { currentRound() };

    if (-move.distanceFromDestination() < 2) // Checks to have at least one square between origin and destination
        throw BadCoordinates { "At least 1 square required between your pawn and the eliminated one" };

    checkFreeTrajectory(move);

    // Applies modifications to grid
    game_grid_[updates.moveOrigin] = Square::Free; // Selected pawn moves out of its square
    game_grid_[updates.moveDestination] = colorFor(current_player); // To replace pawn inside destination

    // One pawn of opponent was removed from grid
    if (current_player == Player::White)
        black_pawns_--;
    else
        white_pawns_--;

    // No more flips chaining can be performed for this round
    last_move_ = Move::Elimination;
}

void Bermudes::playFlip(GridUpdate& updates, AxisIterator move) {
    const Player current_player { currentRound() };
    const Square current_player_color { colorFor(current_player) };

    checkFreeTrajectory(move, -1); // Every squares between the square before which is flipped must be empty

    // Saves position of flipped square for later grid updates notification
    const Coordinates flipped_position { move.currentPosition() };
    // Flipped square reached, get its current state through iterator current position (because it's already there)
    Square& flipped { game_grid_[flipped_position] };
    // One square after the flipped square is the movement destination
    Square& destination { move.moveForward() };

    if (flipped != flip(current_player_color)) // Checks for flipped square to belong to the opponent
        throw BadSquareState { "Flipped square isn't kept by an opponent pawn" };

    // Applies modifications to grid
    game_grid_[updates.moveOrigin] = Square::Free; // Selected pawn moved from its previous square
    flipped = current_player_color; // Flips square of opponent
    destination = current_player_color;

    // One pawn of opponent was replaced by our new pawn
    if (current_player == Player::White) {
        white_pawns_++;
        black_pawns_--;
    } else {
        black_pawns_++;
        white_pawns_--;
    }

    // Saves additional updates for caller
    updates.updatedSquares.push_back({ flipped_position, current_player_color });

    // Flips chaining is still available for this round
    last_move_ = Move::Flip;
}

Player Bermudes::nextRound() {
    // New player hasn't do anything yet, no last move for him
    last_move_.reset();

    // Then normally switches player
    return BoardGame::nextRound();
}

bool Bermudes::isRoundTerminated() const {
    // Round is inevitably terminated if an elimination-take is performed because it will forbid any flips-take
    // chaining during this round
    return last_move_.has_value() && *last_move_ == Move::Elimination;
}

GridUpdate Bermudes::play(const Coordinates& from, const Coordinates& to) {
    const Square current_player_color { colorFor(currentRound()) };

    // A pawn will be moved from `from` to `to`, no square update as grid isn't modified yet
    GridUpdate updates { {}, from, to };
    // Move along axis selected by player, if any
    AxisIterator move { game_grid_, from, to };

    // Origin square must contains a pawn of current player color
    if (game_grid_[from] != current_player_color)
        throw BadSquareState { "Action target square must be kept by a pawn of current player" };

    // Depending on opponent pawn into destination square or not, a move is choosen
    const Square destination_state { game_grid_[to] };
    if (destination_state == Square::Free) // Destination empty, jumps over previous square to take by flip
        playFlip(updates, std::move(move));
    else if (destination_state == flip(current_player_color)) // Destination busy, jumps into destination to eliminate
        playElimination(updates, std::move(move));
    else
        throw BadSquareState { "Movement destination cannot be one of your pawns" };

    // At this point, a move is performed: round can now be terminated
    moved();

    return updates;
}


}
