#include <Minigames-Services/Acores.hpp>

#include <Minigames-Services/AxisIterator.hpp>


namespace MinigamesServices {


Acores::Acores() : BoardGame { INITIAL_GRID_ }, white_pawns_ { 12 }, black_pawns_ { 12 } {}

void Acores::playNormal(GridUpdate& updates, AxisIterator move) {
    Square& destination { move.moveForward() }; // Tries to go for 1 square toward movement direction

    if (destination != Square::Free) // Target pawn should be moved to an empty square
        throw BadSquareState { "Movement destination is kept by another pawn" };

    // Applies movement modifications to grid
    game_grid_[updates.moveOrigin] = Square::Free; // Target pawn no longer into origin square
    destination = colorFor(currentRound()); // It appears into destination square

    // No jumps chaining available after a normal move
    last_move_ = Move::Normal;
}

void Acores::playJump(GridUpdate& updates, AxisIterator move) {
    const Square current_player_color { colorFor(currentRound()) };

    Square& skipped_square { move.moveForward() }; // Tries to move a first time on the square to jump
    const Coordinates skipped_square_position { move.currentPosition() }; // Saved for GridUpdate updated squares

    if (skipped_square != flip(current_player_color)) // Must jump (or eat) an opponent pawn
        throw BadSquareState { "Jumped square must contains a pawn of opponent color" };

    if (move.moveForward() != Square::Free) // Pawn which is jumping must land on a free square
        throw BadSquareState { "Movement destination is kept by another pawn" };

    // Applies movement modifications to grid
    game_grid_[updates.moveOrigin] = Square::Free; // Target pawn no longer into origin square
    skipped_square = Square::Free; // Pawn inside square between two players is eaten
    game_grid_[updates.moveDestination] = current_player_color; // It appears into destination square

    // One pawn from opponent is removed from board
    if (currentRound() == Player::White)
        black_pawns_--;
    else
        white_pawns_--;

    // Saves additional updates for caller
    updates.updatedSquares.push_back({ skipped_square_position, Square::Free });

    // Jumps chaining available after another jump move
    last_move_ = Move::Jump;
}

Player Acores::nextRound() {
    // New player hasn't do anything yet, no last move for him
    last_move_.reset();

    // Then normally switches player
    return BoardGame::nextRound();
}

std::optional<Player> Acores::victoryFor() const {
    // If a player hasn't any pawn on grid, he lose the game
    if (white_pawns_ == 0)
        return Player::White;
    else if (black_pawns_ == 0)
        return Player::Black;
    else // Otherwise game must continue
        return {};
}

bool Acores::isRoundTerminated() const {
    // Round is terminated inevitably terminated if there is no jumps chaining to perform (=> an action has been
    // performed and it is normal move)
    return last_move_.has_value() && *last_move_ == Move::Normal;
}

GridUpdate Acores::play(const Coordinates& from, const Coordinates& to) {
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
        playJump(updates, std::move(move));
        break;
    default:
        throw BadCoordinates { "Selected squares are too far, no available move" };
    }

    return updates;
}


}
