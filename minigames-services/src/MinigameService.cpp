#include <Minigames-Services/MinigameService.hpp>

#include <array>
#include <RpT-Core/ServiceEventRequestProtocol.hpp> // For BadServiceRequest exception


namespace MinigamesServices {


MinigameService::MinigameRequestParser::MinigameRequestParser(const std::string_view sr_command)
: RpT::Utils::TextProtocolParser { sr_command, 1 } {
    const std::string_view unparsed_action { getParsedWord(0) };

    // Tries to parse for each available command
    if (unparsed_action == "MOVE")
        parsed_action_ = Action::Move;
    else if (unparsed_action == "END")
        parsed_action_ = Action::End;
    else // If no known command could have been parsed, then request is ill-formed
        throw RpT::Core::BadServiceRequest { "Unknown action: " + std::string { unparsed_action } };
}

MinigameService::Action MinigameService::MinigameRequestParser::action() const {
    return parsed_action_;
}

std::string_view MinigameService::MinigameRequestParser::moveCommand() const {
    // To access move command arguments, we must be sure to have a MOVE command in the first place
    if (parsed_action_ != Action::Move)
        throw RpT::Core::BadServiceRequest { "Cannot get args for a non-MOVE action command" };

    return unparsedWords(); // MOVE command, retrieves unparsed arguments
}


MinigameService::MoveActionParser::MoveActionParser(const std::string_view move_action_command)
: RpT::Utils::TextProtocolParser { move_action_command, 4 }, parsed_from_ {}, parsed_to_ {} {

    // Access for each integer to be parsed into that command, into the right apparition order
    const std::array<int*, 4> coordinates_to_parse {
        &parsed_from_.line, &parsed_from_.column, &parsed_to_.line, &parsed_to_.column
    };

    // Parses each integer argument in the right order
    for (std::size_t arg_i { 0 }; arg_i < coordinates_to_parse.size(); arg_i++) {
        const std::string arg_copy { getParsedWord(arg_i) }; // Arg needs to be copied for integer conversion

        try { // Tries to parse as integer
            *(coordinates_to_parse[arg_i]) = std::stoi(arg_copy);
        } catch (const std::logic_error& err) { // If failed, throw custom error with message
            throw RpT::Core::BadServiceRequest {
                "Unable to parse MOVE arg #" + std::to_string(arg_i) + ": " + err.what()
            };
        }
    }
}

Coordinates MinigameService::MoveActionParser::from() const {
    return parsed_from_;
}

Coordinates MinigameService::MoveActionParser::to() const {
    return parsed_to_;
}


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
