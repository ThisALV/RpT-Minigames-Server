#include <Minigames-Services/BoardGame.hpp>


namespace MinigamesServices {


BoardGame::BoardGame(std::initializer_list<std::initializer_list<Square>> initial_grid, unsigned int white_pawns,
                     unsigned int black_pawns, unsigned int pawns_count_threshold) :
pawns_count_thershold { pawns_count_threshold }, current_player_ { Player::White }, has_moved_ { false },
game_grid_ { initial_grid }, white_pawns_ { white_pawns }, black_pawns_ { black_pawns } {
    if (pawns_count_threshold == 0) // A minimal value for pawns count cannot be 0, or it would never be reached
        throw std::invalid_argument { "Pawns count loose threshold must be positive strict" };
}

void BoardGame::moved() {
    has_moved_ = true;
}

bool BoardGame::hasMoved() {
    return has_moved_;
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

unsigned int BoardGame::pawnsFor(const Player pawns_owner) const {
    // Returns appropriate counter depending on given player color
    if (pawns_owner == Player::White)
        return white_pawns_;
    else
        return black_pawns_;
}

std::optional<Player> BoardGame::victoryFor() const {
    // If a has less then threshold count of pawns inside grid, his opponent won
    if (white_pawns_ < pawns_count_thershold)
        return Player::Black;
    else if (black_pawns_ < pawns_count_thershold)
        return Player::White;
    else // Otherwise game must continue
        return {};
}


}
