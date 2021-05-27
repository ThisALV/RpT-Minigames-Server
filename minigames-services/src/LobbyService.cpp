#include <Minigames-Services/LobbyService.hpp>

#include <RpT-Core/ServiceEventRequestProtocol.hpp>


namespace MinigamesServices {


LobbyService::CommandParser::CommandParser(const std::string_view lobby_command)
: RpT::Utils::TextProtocolParser { lobby_command, 1 } {
    if (getParsedWord(0) != "READY") // This is the only available command for that service
        throw RpT::Core::BadServiceRequest { "Only READY command is available for Lobby" };
}


std::optional<LobbyService::Entrant>& LobbyService::playerFor(const std::uint64_t actor_uid) {
    /*
     * Checks for each player if an actor is assigned to it and if it is the looked for one
     */

    if (white_player_actor_.has_value() && white_player_actor_->actorUid == actor_uid)
        return white_player_actor_;
    else if (black_player_actor_.has_value() && black_player_actor_->actorUid == actor_uid)
        return black_player_actor_;
    else
        throw std::invalid_argument { "Actor " + std::to_string(actor_uid) + " isn't assigned to any player" };
}

void LobbyService::cancelCountdown() {
    // Clients will be notified if they were waiting for minigame to start
    if (starting_countdown_.isPending())
        emitEvent("END_COUNTDOWN");

    // If countdown hasn't begun, it will not have any effect, can be called in any state
    starting_countdown_.clear();
}

LobbyService::LobbyService(RpT::Core::ServiceContext& run_context, MinigameService& rpt_minigame,
                           const std::size_t countdown_ms) :
                           RpT::Core::Service { run_context, { starting_countdown_ } },
                           minigame_session_ { rpt_minigame },
                           ready_players_ { 0 },
                           starting_countdown_ { run_context, countdown_ms } {}

std::string_view LobbyService::name() const {
    return "Lobby";
}

Player LobbyService::assignActor(const std::uint64_t actor_uid) {
    if (!white_player_actor_.has_value()) {// Tries to assign white player first
        white_player_actor_ = { actor_uid, false };

        return Player::White;
    } else if (!black_player_actor_.has_value()) { // If cannot, try to assign black player then
        black_player_actor_ = { actor_uid, false };

        return Player::Black;
    } else { // If neither white nor black player can be assigned, the operation fails
        throw BadPlayersState { "No player available" };
    }
}

void LobbyService::removeActor(const std::uint64_t actor_uid) {
    try {
        std::optional<Entrant>& entrant_for_actor { playerFor(actor_uid) }; // Tries to get player assigned with UID

        // If removed player was ready, then count needs to be decremented and countdown to be updated
        if (entrant_for_actor->isReady) {
            cancelCountdown();
            ready_players_--;
        }

        entrant_for_actor.reset(); // After that check, player can be removed from minigame entrants
    } catch (const std::invalid_argument& err) { // Catches if given actor doesn't correspond to any player
        throw BadPlayersState { err.what() }; // In that case, rethrows for a BadPlayersState error type
    }
}

RpT::Utils::HandlingResult LobbyService::handleRequestCommand(
        const std::uint64_t actor, const std::string_view sr_command_data) {

    // Parses "READY" command
    const CommandParser parsed_command { sr_command_data };

    // Reference to the isReady field of the appropriate Entrant
    bool& ready_flag { playerFor(actor)->isReady };
    // Flag is complemented without calling playerFor() 2 times
    ready_flag = !ready_flag;

    // Updates the ready players count and syncs clients with ready flag value depending on new player state
    if (ready_flag) {
        ready_players_++;
        emitEvent("READY_PLAYER " + std::to_string(actor));
    } else {
        ready_players_--;
        emitEvent("WAITING_FOR_PLAYER " + std::to_string(actor));
    }

    if (ready_players_ == 2) { // If all players are ready now, begins countdown for minigame to start
        starting_countdown_.requestCountdown();

        // When countdown is done, starts minigame with configured/assigned players
        starting_countdown_.onNextTrigger([this]() {
            minigame_session_.start(white_player_actor_->actorUid, black_player_actor_->actorUid);
        });

        // Syncs clients with beginning countdown so they can perform a countdown on their side too
        emitEvent("BEGIN_COUNTDOWN " + std::to_string(starting_countdown_.countdown()));
    } else { // If not every player is ready, cancel countdown
        cancelCountdown();
    }

    return {}; // If SR command contains READY command, there is no way it could fail, success should be guaranteed
}


}
