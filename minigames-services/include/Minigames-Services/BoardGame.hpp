#ifndef RPT_MINIGAMES_SERVICES_BOARDGAME_HPP
#define RPT_MINIGAMES_SERVICES_BOARDGAME_HPP

/**
 * @file BoardGame.hpp
 */

#include <Minigames-Services/Grid.hpp>


namespace MinigamesServices {


/// Represents a player into board game which is owning a specific kind of pawns inside grid
enum struct Player {
    White, Black
};


/**
 * @brief Base class to implement a round-by-round board minigame played with 2 players onto a `Grid`
 *
 * Minigame implementations must overrides virtual methods to define game's behavior. A %Service can access public
 * methods to control the execution flow of the game and emit appropriate events.
 *
 * @author ThisALV, https://github.com/ThisALV/
 */
class BoardGame {
private:
    Player current_player_;

protected:
    Grid game_grid_;

    /**
     * @brief Constructs a game with a specific initial grid
     *
     * @param initial_grid Grid state at the beginning of a new game
     *
     * @throws BadDimensions if initial grid is invalid
     */
    BoardGame(std::initializer_list<std::initializer_list<Square>> initial_grid);

    /**
     * @brief Switch current player to other player, definitely terminating current player round
     *
     * @note Can be called even if `isRoundTerminated() == false`, will terminates the round anyways.
     *
     * @returns Player who's current round player after method call
     */
    Player nextRound();

    /**
     * @brief Retrieves player who won this game, if terminated
     *
     * @returns Uninitialized if game isn't terminated, otherwise `Player` who won
     */
    virtual std::optional<Player> victoryFor() const = 0;

    /**
     * @brief Checks if current player can do other actions or not
     *
     * @returns `true` if current player can no longer do anything and `nextRound()` should be called, `false` otherwise
     */
    virtual bool isRoundTerminated() const = 0;

    /**
     * @brief Plays given move for current player
     *
     * @param from Square containing the moved pawn before method call
     * @param to Square containing the moved pawn after method call
     *
     * @throws std::exception depending on implementation, means that this move cannot be done
     */
    virtual void play(const Coordinates& from, const Coordinates& to) = 0;
};


}


#endif //RPT_MINIGAMES_SERVICES_BOARDGAME_HPP
