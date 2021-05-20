#include <Minigames-Services/MinigameService.hpp>


namespace MinigamesServices {


MinigameService::MinigameService(RpT::Core::ServiceContext& run_context, const Minigame rpt_minigame_type)
: RpT::Core::Service { run_context }, rpt_minigame_type_ { rpt_minigame_type } {}

std::string_view MinigameService::name() const {
    return "Minigame";
}

Player MinigameService::assignPlayerActor(const std::uint64_t actor_uid) {
    if (!white_player_actor_.has_value()) { // Tries to assign white player first
        white_player_actor_ = actor_uid;
        return Player::White;
    } else if (!black_player_actor_.has_value()) { // If unavailable, tries to assign black player second
        black_player_actor_ = actor_uid;
        return Player::Black;
    } else { // If both are unavailable, operation isn't allowed
        throw BadPlayersState { "Both 2 players are already assigned" };
    }
}

void MinigameService::removePlayerActor(const Player player) {
    std::optional<std::uint64_t>* selected_player;

    // Selects correct actor valeu depending on given color
    if (player == Player::White)
        selected_player = &white_player_actor_;
    else
        selected_player = &black_player_actor_;

    // On selected actor value, try to reset it, if and only if it already has a value, if not, player can't be
    // removed as it wasn't assigned in the first place
    if (!selected_player->has_value())
        throw BadPlayersState { "Player with given color isn't assigned" };

    selected_player->reset();
}

/*
 * Temporary: deleted when classes will be created. =>
 */

class UnimplementedBoardGame : public BoardGame {
public:
    UnimplementedBoardGame() : BoardGame { { Square::Free } } {}

    std::optional<Player> victoryFor() const override { return {}; }
    bool isRoundTerminated() const override { return false; }
    void play(const Coordinates& from, const Coordinates& to) override {}
};

class Acores : public UnimplementedBoardGame {};
class Bermudes : public UnimplementedBoardGame {};
class Canaries : public UnimplementedBoardGame {};

/*
 * <= Temporary: deleted when classes will be created.
 */

void MinigameService::start() {
    // Initializes RpT-Minigame board game depending on configured minigame type
    switch (rpt_minigame_type_) {
    case Minigame::Acores:
        current_game_ = std::make_unique<Acores>();
        break;
    case Minigame::Bermudes:
        current_game_ = std::make_unique<Bermudes>();
        break;
    case Minigame::Canaries:
        current_game_ = std::make_unique<Canaries>();
        break;
    }
}

RpT::Utils::HandlingResult MinigameService::handleRequestCommand(
        const std::uint64_t actor, const std::string_view sr_command_data) {

    return {};
}


}
