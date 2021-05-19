#ifndef RPT_MINIGAMES_SERVICES_MINIGAMESERVICE_HPP
#define RPT_MINIGAMES_SERVICES_MINIGAMESERVICE_HPP

/**
 * @file MinigameService.hpp
 */

#include <memory>
#include <Minigames-Services/BoardGame.hpp>
#include <RpT-Core/Service.hpp>


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


/// Represents a specific RpT-Minigame to run with service
enum struct Minigame {
    Acores, Bermudes, Canaries
};


/**
 * @brief Runs specified `Minigame` when 2 actors are ready
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
    const Minigame rpt_minigame_type_;

    std::unique_ptr<BoardGame> current_game_;
    std::optional<std::uint64_t> while_player_actor_;
    std::optional<std::uint64_t> black_player_actor_;

public:
    /**
     * @brief Constructs service for given `Minigame`
     *
     * @param rpt_minigame_type RoT-Minigame to run with %Service
     */
    MinigameService(Minigame rpt_minigame_type);

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
     * @brief Starts RpT-Minigame board game session depending on `Minigame` type, with assigned players/actors
     *
     * @throws BadBoardGameState if a game is already running
     * @throws BadPlayersState if at least one of the 2 players isn't assigned to an actor
     */
    void start();

    /// Handles play from current player to make board game progress
    RpT::Utils::HandlingResult handleRequestCommand(std::uint64_t actor, std::string_view sr_command_data) override;
};


}


#endif // RPT_MINIGAMES_SERVICES_MINIGAMESERVICE_HPP
