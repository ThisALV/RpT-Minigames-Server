#ifndef RPT_MINIGAMES_SERVICES_MINIGAMESERVICE_HPP
#define RPT_MINIGAMES_SERVICES_MINIGAMESERVICE_HPP

/**
 * @file MinigameService.hpp
 */

#include <functional>
#include <memory>
#include <Minigames-Services/BoardGame.hpp>
#include <RpT-Core/Service.hpp>
#include <RpT-Utils/TextProtocolParser.hpp>


namespace MinigamesServices {


/// Thrown by `MinigameService` methods when players aren't into expected state (assigned/not assigned)
class BadPlayersState : public std::logic_error {
public:
    /// Constructs error with user-provided message
    explicit BadPlayersState(const std::string& reason)
    : std::logic_error { "Bad players state: " + reason } {}
};

/// Thrown by `MinigameService` methods when game isn't into expected state (started/not started)
class BadBoardGameState : public std::logic_error {
public:
    /// Constructs error with user-provided message
    explicit BadBoardGameState(const std::string& reason)
    : std::logic_error { "Bad board game state: " + reason } {}
};


/// These functions are used by `MinigameService` to obtain `BoardGame` objects from user
using BoardGameProvider = std::function<std::unique_ptr<BoardGame>()>;


/**
 * @brief Runs `BoardGame` minigame returned by given provider when 2 actors are ready
 *
 * This %Service controls the basic execution flow of a RpT-Minigame by calling it's virtual methods depending on
 * it's current state.
 *
 * Each connected actor will be assigned to a `Player` (black or white) and when the two players are assigned, the
 * game can be started at any moment using `start()` method.
 *
 * @author ThisALV, https://github.com/ThisALV/
 */
class MinigameService : public RpT::Core::Service {
private:
    /// An action the player wants to perform at his round (moving a pawn, or going to the next round)
    enum struct Action {
        Move, End
    };

    /// Parses an action to perform for current player round
    class MinigameRequestParser : public RpT::Utils::TextProtocolParser {
    private:
        Action parsed_action_;

    public:
        /// Parses given SR command
        explicit MinigameRequestParser(std::string_view sr_command);

        /// `Action` requested by given SR command
        Action action() const;

        /// Retrieves unparsed coordinates given as arguments for `MOVE` command
        std::string_view moveCommand() const;
    };

    /// Parses a player movement (a pawn from a square to another)
    class MoveActionParser : public RpT::Utils::TextProtocolParser {
    private:
        Coordinates parsed_from_;
        Coordinates parsed_to_;

    public:
        /// Parses given Move action
        explicit MoveActionParser(std::string_view move_action_command);

        /// Parsed coordinates for square with pawn to move
        Coordinates from() const;

        /// Parsed coordinates for square to move pawn into
        Coordinates to() const;
    };

    const BoardGameProvider rpt_minigame_provider_;

    std::unique_ptr<BoardGame> current_game_;
    std::optional<std::uint64_t> white_player_actor_;
    std::optional<std::uint64_t> black_player_actor_;

    /// Goes to next round for board game and emits ROUND_FOR %Service Event
    void terminateRound();

    /// Handler for move action command
    void handleMove(std::string_view move_command_args);

public:
    /**
     * @brief Constructs service for given `Minigame`
     *
     * @param run_context Context into which Services run
     * @param rpt_minigame_provider Function which return new RpT-Minigame to run with %Service
     */
    MinigameService(RpT::Core::ServiceContext& run_context, BoardGameProvider rpt_minigame_provider);

    /// Retrieves service name "Minigame"
    std::string_view name() const override;

    /**
     * @brief Assigns given actor UID to the next available player
     *
     * @param actor_uid UID to assign for an available player
     *
     * @returns `Player` assigned with given UID
     *
     * @throws BadPlayersState if black and white players both are already assigned
     */
    Player assignPlayerActor(std::uint64_t actor_uid);

    /**
     * @brief Removes actor for given player color
     *
     * @param player Color of player to remove actor for
     *
     * @throws BadPlayersState if given player isn't assigned
     */
    void removePlayerActor(Player player);

    /**
     * @brief Starts RpT-Minigame board game session using RpT-Minigame returned by given provider, with assigned
     * players/actors
     *
     * @throws BadBoardGameState if a game is already running
     * @throws BadPlayersState if at least one of the 2 players isn't assigned to an actor
     */
    void start();

    /**
     * @brief Stops RpT-Minigame board game session
     *
     * @throws BadBoardGameState if game isn't running
     */
    void stop();

    /// Handles play from current player to make board game progress
    RpT::Utils::HandlingResult handleRequestCommand(std::uint64_t actor, std::string_view sr_command_data) override;
};


}


#endif // RPT_MINIGAMES_SERVICES_MINIGAMESERVICE_HPP
