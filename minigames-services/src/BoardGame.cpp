#include <Minigames-Services/BoardGame.hpp>


namespace MinigamesServices {


BoardGame::BoardGame(std::initializer_list<std::initializer_list<Square>> initial_grid)
: current_player_ { Player::White }, has_moved_ { false }, game_grid_ { initial_grid } {}

void BoardGame::moved() {
    has_moved_ = true;
}

Player BoardGame::nextRound() {
    if (!has_moved_) // Checks for at one move to have been done, as skipping turn isn't allowed
        throw MoveRequired {};

    has_moved_ = false; // Resets flag for the next round

    // Switches current player
    if (current_player_ == Player::White)
        current_player_ = Player::Black;
    else
        current_player_ = Player::White;

    // Retrieves it
    return current_player_;
}

Player BoardGame::currentRound() const {
    return current_player_;
}


}
