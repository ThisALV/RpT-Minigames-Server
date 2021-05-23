#ifndef RPT_MINIGAMES_SERVICES_BOARDGAME_HPP
#define RPT_MINIGAMES_SERVICES_BOARDGAME_HPP

/**
 * @file BoardGame.hpp
 */

#include <optional>
#include <stdexcept>
#include <Minigames-Services/Grid.hpp>


namespace MinigamesServices {


/// Thrown by `BoardGame::nextRound()` if current player hasn't play any move during current round
class MoveRequired : public std::logic_error {
public:
    /// Constructs error with basic error message
    MoveRequired() : std::logic_error { "Player can't skip a round" } {}
};


/// Represents a player into board game which is owning a specific kind of pawns inside grid
enum struct Player {
    White, Black
};

/// Retrieves pawn color associated with given player color
constexpr Square colorFor(const Player player_color) {
    if (player_color == Player::White)
        return Square::White;
    else
        return Square::Black;
}


/**
 * @brief Represents an update about a `Square` inside a `Grid` after a call to `BoardGame::play()`
 */
struct SquareUpdate {
    /// Which square has been updated inside grid
    Coordinates square;
    /// The new state of that square
    Square updatedState;
};

/**
 * @brief Represents every update about a `Grid` after a call to `BoardGame::play()`
 */
struct GridUpdate {
    /// Every square which has been updated with that move, moved pawn excluded
    std::vector<SquareUpdate> updatedSquares;
    /// Square of moved pawn
    Coordinates moveOrigin;
    /// Square of this pawn after it was moved
    Coordinates moveDestination;
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
    const unsigned int pawns_count_thershold;

    Player current_player_;
    bool has_moved_;

protected:
    /// Grid used to store and manipulate squares and pawns for board game, should be modified inside `play()` by
    /// move actions
    Grid game_grid_;
    /// Number of pawns inside grid for white player
    unsigned int white_pawns_;
    /// Number of pawns inside grid for black player
    unsigned int black_pawns_;

    /**
     * @brief Constructs a game with a specific initial grid
     *
     * @param initial_grid Grid state at the beginning of a new game
     * @param white_pawns Number of white pawns inside the `initial_grid`
     * @param black_pawns Number of black pawns inside the `initial_grid`
     * @param pawns_count_threshold A minimum of pawns to have for a player. If current count is less than threshold,
     * opponent wins the game.
     *
     * @throws BadDimensions if initial grid is invalid
     * @throws std::invalid_argument if `pawns_count_threshold == 0`
     */
    BoardGame(std::initializer_list<std::initializer_list<Square>> initial_grid,
              unsigned int white_pawns, unsigned int black_pawns, unsigned int pawns_count_threshold);

    /**
     * @brief Enables moved flags, means that player into current round did at least one move, expected to be called
     * from `play()` implementation
     */
    void moved();

public:
    /*
     * Entity class semantic
     */

    BoardGame(const BoardGame&) = delete;
    BoardGame& operator=(const BoardGame&) = delete;

    bool operator==(const BoardGame&) const = delete;

    /**
     * @brief Switch current player to other player, definitely terminating current player round
     *
     * You can override this method calling the base-class method inside implementation to add some routine to
     * execute each time a round is terminated.
     *
     * @note Can be called even if `isRoundTerminated() == false`, will terminates the round anyways.
     *
     * @returns Player who's current round player after method call
     *
     * @throws MoveRequired if no move was done using `play()` before this call
     */
    virtual Player nextRound();

    /**
     * @brief Retrieves current round player
     *
     * @returns Current `Player`
     */
    Player currentRound() const;

    /**
     * @brief Retrieves number of pawns inside grid for given player
     *
     * @param pawns_owner `Player` to check pawns count for
     *
     * @returns Number of pawns
     */
    unsigned int pawnsFor(Player pawns_owner) const;

    /**
     * @brief Retrieves player who won this game, if terminated
     *
     * Default behavior returns opponent victory is a player has less than pawns constructor threshold.
     *
     * @returns Uninitialized if game isn't terminated, otherwise `Player` who won
     */
    virtual std::optional<Player> victoryFor() const;

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
     * @returns Every update which occurred into the game grid and that the clients must be synced with
     *
     * @throws std::exception depending on implementation, means that this move cannot be done
     */
    virtual GridUpdate play(const Coordinates& from, const Coordinates& to) = 0;

    /// `virtual` for polymorphism without memory leaks
    virtual ~BoardGame() = default;
};


}


#endif //RPT_MINIGAMES_SERVICES_BOARDGAME_HPP
