#ifndef RPT_MINIGAMES_SERVICES_LOBBY_HPP
#define RPT_MINIGAMES_SERVICES_LOBBY_HPP

/**
 * @file Lobby.hpp
 */

#include <Minigames-Services/MinigameService.hpp>
#include <RpT-Core/Service.hpp>
#include <RpT-Core/ServiceContext.hpp>
#include <RpT-Utils/TextProtocolParser.hpp>


namespace MinigamesServices {


/// Thrown by `LobbyService` method related to actors assigned to players if an operation failed
class BadPlayersState : public std::logic_error {
public:
    /// Constructs error with user-provided message
    explicit BadPlayersState(const std::string& reason) : std::logic_error { reason } {}
};


/**
 * @brief Implements a Lobby waiting for 2 actors to be ready and making them play given RpT-Minigame when they are
 * both ready
 *
 * There are 2 available actor slots: 1 for the white player, 1 for the black player.
 * When an new actor is registered, it is assigned to an available slot, if any.
 * As soon as both players are ready, the underlying timer is started, then at its time out RpT-Minigame session can
 * start with the 2 assigned actors to players.
 *
 * @author ThisALV, https://github.com/ThisALV/
 */
class LobbyService : public RpT::Core::Service {
private:
    /// Represent an assigned actor and its current state inside the %Lobby
    struct Entrant {
        std::uint64_t actorUid;
        bool isReady;
    };

    /// Parses received command to check if it matches with the only available command which is `READY`
    class CommandParser : public RpT::Utils::TextProtocolParser {
    public:
        /// Parses given SR command
        explicit CommandParser(std::string_view lobby_command);
    };

    MinigameService& minigame_session_;
    std::optional<Entrant> white_player_actor_;
    std::optional<Entrant> black_player_actor_;
    unsigned int ready_players_;
    RpT::Core::Timer starting_countdown_;

    /// Retrieves actor associated with given UID
    std::optional<Entrant>& playerFor(std::uint64_t actor_uid);

public:
    /**
     * @brief Initializes service to run given RpT-Minigame
     *
     * Minigame will be run `countdown_ms` after both players are ready if no player is no longer ready during
     * countdown.
     *
     * @param run_context Context used by every server `Service`
     * @param rpt_minigame Configured `MinigameService` to run by calling its `start()` method
     * @param countdown_ms Delay between both players ready and minigame start
     */
    LobbyService(RpT::Core::ServiceContext& run_context, MinigameService& rpt_minigame, std::size_t countdown_ms);

    /// Retrieves service name "Lobby".
    std::string_view name() const override;

    /**
     * @brief Assigns given actor UID to an available player for the minigame
     *
     * @param actor_uid UID of actor that will play for returned `Player`
     *
     * @returns `Player` assigned, played by given actor
     *
     * @throws BadPlayersState if both players are already assigned
     */
    Player assignActor(std::uint64_t actor_uid);

    /**
     * @brief Removes given actor from the player currently assigned with it
     *
     * @param actor_uid UID of actor that will no longer play minigame with associated `Player`
     *
     * @throws BadPlayersState if given UID isn't assigned to any player
     */
    void removeActor(std::uint64_t actor_uid);

    /// Handles READY command from actors to start the minigame, only READY command is available
    RpT::Utils::HandlingResult handleRequestCommand(std::uint64_t actor, std::string_view sr_command_data) override;
};


}


#endif // RPT_MINIGAMES_SERVICES_LOBBY_HPP
