#include <Minigames-Services/MinigameService.hpp>

#include <array>
#include <RpT-Core/ServiceEventRequestProtocol.hpp> // For BadServiceRequest exception
#include <utility>


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


MinigameService::MinigameService(RpT::Core::ServiceContext& run_context, BoardGameProvider rpt_minigame_provider)
: RpT::Core::Service { run_context }, rpt_minigame_provider_ { std::move(rpt_minigame_provider) },
white_player_actor_ { 0 }, black_player_actor_ { 0 } {} // Game didn't start, UID assigned to players doesn't matter

std::string_view MinigameService::name() const {
    return "Minigame";
}

void MinigameService::start(const std::uint64_t white_player_actor, const std::uint64_t black_player_actor) {
    if (current_game_) // Checks for another game to not be currently running
        throw BadBoardGameState { "Game is already running" };

    white_player_actor_ = white_player_actor;
    black_player_actor_ = black_player_actor;

    // Initializes RpT-Minigame board game with polymorphic value returned by provider
    current_game_ = rpt_minigame_provider_();

    // Sends to clients so they know minigame has begun, and which actor is which player
    emitEvent("START " + std::to_string(white_player_actor_) + ' ' + std::to_string(black_player_actor_));
    // Every minigame starts with the white player
    emitEvent("ROUND_FOR WHITE");
}

void MinigameService::terminateRound() {
    const Player next_player { current_game_->nextRound() }; // Tries to go for next round, might fail

    std::string round_command_arg;
    // Sets command argument depending on next round player
    if (next_player == Player::White)
        round_command_arg = "WHITE";
    else
        round_command_arg = "BLACK";

    // If nextRound() succeeded, sync clients with that information
    emitEvent("ROUND_FOR " + round_command_arg);
}

RpT::Utils::HandlingResult MinigameService::handleRequestCommand(
        const std::uint64_t actor, const std::string_view sr_command_data) {

    if (!current_game_) // Cannot handle any request if a game is not running to perform any action
        return RpT::Utils::HandlingResult { "Game is stopped" };

    /*
     * Checks for SR author to be the actor who's currently playing
     */

    std::uint64_t expected_actor;
    if (current_game_->currentRound() == Player::White)
        expected_actor = white_player_actor_;
    else
        expected_actor = black_player_actor_;

    if (actor != expected_actor)
        return RpT::Utils::HandlingResult { "This is not your turn" };

    // Parses SR command
    const MinigameRequestParser command_parser { sr_command_data };

    // Calls correct handler depending on performed action
    switch (command_parser.action()) {
    case Action::Move:
        handleMove(command_parser.moveCommand());
        break;
    case Action::End:
        terminateRound();
        break;
    }

    return {}; // Command was handled successfully, nothing more to do
}

void MinigameService::handleMove(const std::string_view move_command_args) {
    // Parses coordinates arguments
    const MoveActionParser move_parser { move_command_args };

    // Checks that any move can still be performed
    if (current_game_->isRoundTerminated())
        throw BadBoardGameState { "Cannot make any move, round terminated" };

    // Plays move for received coordinates saving every update which occurred into the grid
    const GridUpdate unsync_updates { current_game_->play(move_parser.from(), move_parser.to()) };

    // Syncs every square that were updated by this move
    for (const auto& [square, updatedState] : unsync_updates.updatedSquares) {
        std::string square_state_sync_arg; // Argument for SQUARE_STATE command corresponding to stringified state
        switch (updatedState) { // Stringifies new state to append it to command
        case Square::Free:
            square_state_sync_arg = "FREE";
            break;
        case Square::White:
            square_state_sync_arg = "WHITE";
            break;
        case Square::Black:
            square_state_sync_arg = "BLACK";
            break;
        }

        emitEvent("SQUARE_STATE " + std::to_string(square.line) + ' ' + std::to_string(square.column)
                  + ' ' + square_state_sync_arg);
    }

    const auto [from_line, from_column] { unsync_updates.moveOrigin };
    const auto [to_line, to_column] { unsync_updates.moveDestination };

    // Syncs clients with pawn concerned by this move
    emitEvent("MOVED " + std::to_string(from_line) + ' ' + std::to_string(from_column)
              + ' ' + std::to_string(to_line) + ' ' + std::to_string(to_column));

    // Syncs clients with new pawns count for each player, so they don't need to recalculate it themselves
    emitEvent("PAWN_COUNTS " + std::to_string(current_game_->pawnsFor(Player::White)) + ' '
              + std::to_string(current_game_->pawnsFor(Player::Black)));

    // Checks if move caused any Player to win the game
    const std::optional<Player> possible_winner { current_game_->victoryFor() };
    if (possible_winner.has_value()) { // If a player won, stops the game and sync clients with new session state
        std::string victory_command_arg;
        // Sets command argument depending on next round player
        if (*possible_winner == Player::White)
            victory_command_arg = "WHITE";
        else
            victory_command_arg = "BLACK";

        // Sync clients and stop game as a player has won
        emitEvent("VICTORY_FOR " + victory_command_arg);
        stop();

        current_game_.reset(); // Stops current game by deleting it once it has been stopped properly with SE sent
    } else if (current_game_->isRoundTerminated()) { // If this move caused round to terminate...
        // ...go to next round, syncing clients with new board game state
        terminateRound();
    }
}

void MinigameService::stop() {
    if (!current_game_) // Checks for a game to be currently running
        throw BadBoardGameState { "Game is not running" };

    current_game_.reset(); // Stops current game by deleting it

    emitEvent("STOP"); // Sync clients with new game state
}

bool MinigameService::isStarted() const {
    // If pointer is holding a value to a board game, then it is currently running
    return static_cast<bool>(current_game_);
}


}
