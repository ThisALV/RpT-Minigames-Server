#include <Minigames-Services/BoardGame.hpp>


namespace MinigamesServices {


BoardGame::BoardGame(std::initializer_list<std::initializer_list<Square>> initial_grid)
: current_player_ { Player::White }, game_grid_ { initial_grid } {}

Player BoardGame::nextRound() {
    // Switches current player
    if (current_player_ == Player::White)
        current_player_ = Player::Black;
    else
        current_player_ = Player::White;

    // Retrieves it
    return current_player_;
}


}
