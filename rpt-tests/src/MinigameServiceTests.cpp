#include <RpT-Testing/TestingUtils.hpp>
#include <RpT-Testing/MinigamesServicesTestingUtils.hpp> // Required by BOOST_CHECK_EQUAL for operartor<<

#include <Minigames-Services/MinigameService.hpp>
#include <RpT-Core/ServiceContext.hpp>


using namespace MinigamesServices;


namespace { // These testing features are only required inside this unit tests file, avoiding names clash


/// Mocked `BoardGame` keeping track of which virtual implemented methods were called by %Service with which
/// arguments, this class can also configures methods returned values for mocking purpose
/// Another feature is a customisable routine to emulates an implement/side effect for `play()` protected method.
class MockedBoardGame : public BoardGame {
public:
    /// Represents a set of arguments given to `play()`
    struct PlayedMove {
        Coordinates from;
        Coordinates to;
    };

    bool nextRoundCalled;
    std::optional<PlayedMove> playCallArguments;

    std::optional<Player> victoryForReturn;
    bool isRoundTerminatedReturn;
    GridUpdate playReturn;

    std::function<void()> playCallRoutine;

    /// Constructs game with unset method-called flags and defaulted mocked return values
    MockedBoardGame() : BoardGame { { EMPTY } },
    nextRoundCalled { false }, playCallArguments {}, victoryForReturn {}, isRoundTerminatedReturn { false },
    playReturn {}, playCallRoutine {} {}

    /// Calls base class `nextRound()`, keeping track that a call has been performed if base method succeeded
    Player nextRound() override {
        const Player current_player { BoardGame::nextRound() };
        nextRoundCalled = true;

        return current_player;
    }

    /// Returns mock value `victoryForReturn`
    std::optional<Player> victoryFor() const override {
        return victoryForReturn;
    }

    /// Returns mock value `isRoundTerminatedReturn`
    bool isRoundTerminated() const override {
        return isRoundTerminatedReturn;
    }

    /// Keeps track a method call has been performed and arguments used for this call, then call play() method mock,
    /// if any
    GridUpdate play(const Coordinates& from, const Coordinates& to) override {
        playCallArguments = { from, to };

        if (playCallRoutine)
            playCallRoutine();

        return playReturn;
    }

    /// Mocks a move which have been done successfully
    void makeMove() {
        moved();
    }
};

/// Tests if `play()` of given `MockedBoardGame` object was called with expected arguments
void boostCheckPlayCall(const MockedBoardGame& board_game,
                        const Coordinates& expected_from, const Coordinates& expected_to) {

    BOOST_CHECK(board_game.playCallArguments.has_value());
    BOOST_CHECK_EQUAL(board_game.playCallArguments->from, expected_from);
    BOOST_CHECK_EQUAL(board_game.playCallArguments->to, expected_to);
}

/// Provides testing instance using `MockedBoardGame` as minigame
class BoardGameFixture {
public:
    RpT::Core::ServiceContext context;
    MinigameService service;
    MockedBoardGame* boardGame;

    /// Initializes service and provides direct access to its mocked board game when started
    BoardGameFixture() : service { context, [this]() {
        auto mocked_board_game { std::make_unique<MockedBoardGame>() };
        boardGame = mocked_board_game.get(); // Keeps access to board game for mocking purposes

        return mocked_board_game;
    } }, boardGame { nullptr } {}
};


}


BOOST_FIXTURE_TEST_SUITE(MinigameServiceTests, BoardGameFixture)


/// UID used by actor assigned to the white player into these tests
constexpr std::uint64_t WHITE_PLAYER_ACTOR { 0 };
/// UID used by actor assigned to the black player into these tests
constexpr std::uint64_t BLACK_PLAYER_ACTOR { 1 };


/*
 * start() unit tests
 */
BOOST_AUTO_TEST_SUITE(Start)


BOOST_AUTO_TEST_CASE(GameAlreadyRunning) {
    // Starts game a first time
    service.start(WHITE_PLAYER_ACTOR, BLACK_PLAYER_ACTOR);

    // Should fail to start it a second time
    BOOST_CHECK_THROW(service.start(WHITE_PLAYER_ACTOR, BLACK_PLAYER_ACTOR), BadBoardGameState);
}

BOOST_AUTO_TEST_CASE(GameStopped) {
    service.start(WHITE_PLAYER_ACTOR, BLACK_PLAYER_ACTOR);

    // Starting game should emit exactly one event to sync clients with new state
    BOOST_CHECK_EQUAL(service.pollEvent(), "START 0 1");
    BOOST_CHECK(!service.checkEvent().has_value());
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * stop() method unit tests
 */
BOOST_AUTO_TEST_SUITE(Stop)


BOOST_AUTO_TEST_CASE(GameAlreadyStopped) {
    // No game running right after construction, shouldn't be able to stop
    BOOST_CHECK_THROW(service.stop(), BadBoardGameState);
}

BOOST_AUTO_TEST_CASE(GameRunning) {
    // Starts game in the first place
    service.start(WHITE_PLAYER_ACTOR, BLACK_PLAYER_ACTOR);
    service.pollEvent(); // Flushes event emitted by start()

    // Stop should sync with clients for new state by sending exactly one event
    service.stop();
    BOOST_CHECK_EQUAL(service.pollEvent(), "STOP");
    BOOST_CHECK(!service.checkEvent().has_value());
}


BOOST_AUTO_TEST_SUITE_END()

/*
 * Service Request handling unit tests
 */
BOOST_AUTO_TEST_SUITE(HandleRequestCommand)


/// Provides a service with an already started board game session
class StartedBoardGameFixture : public BoardGameFixture {
public:
    /// Applies parent fixture and starts game, then flush SE emitted by game start
    StartedBoardGameFixture() : BoardGameFixture {} {
        service.start(WHITE_PLAYER_ACTOR, BLACK_PLAYER_ACTOR);
        service.pollEvent();
    }
};


BOOST_AUTO_TEST_CASE(GameStopped) {
    // No game running, cannot handle actions from players
    BOOST_CHECK_EQUAL(service.handleRequestCommand(WHITE_PLAYER_ACTOR, "").errorMessage(), "Game is stopped");
}


BOOST_FIXTURE_TEST_SUITE(GameRunning, StartedBoardGameFixture)


BOOST_AUTO_TEST_CASE(UnknownActorUID) {
    // Actor UID 2 wasn't a start() argument
    BOOST_CHECK_EQUAL(service.handleRequestCommand(2, "").errorMessage(), "This is not your turn");
}

BOOST_AUTO_TEST_CASE(BadActorUID) {
    // White player which is actor 0 should play instead of 1
    BOOST_CHECK_EQUAL(service.handleRequestCommand(BLACK_PLAYER_ACTOR, "").errorMessage(),
                      "This is not your turn");
}


/*
 * End command unit test
 */
BOOST_AUTO_TEST_CASE(End) {
    // Emulates that white player already moved at least once
    boardGame->makeMove();
    // White player go for next round
    BOOST_CHECK(service.handleRequestCommand(WHITE_PLAYER_ACTOR, "END"));

    // Should have emit exactly one event to go for next round
    BOOST_CHECK_EQUAL(service.pollEvent(), "ROUND_FOR BLACK");
    BOOST_CHECK(!service.checkEvent().has_value());

    // nextRound() should have been called for board game
    BOOST_CHECK(boardGame->nextRoundCalled);
}


/*
 * Move command unit tests
 */
BOOST_AUTO_TEST_SUITE(Move)


BOOST_AUTO_TEST_CASE(RoundAlreadyTerminated) {
    // Emulates that round is already terminated
    boardGame->isRoundTerminatedReturn = true;
    // Should fail to do move as it should go for next round
    BOOST_CHECK_THROW(service.handleRequestCommand(WHITE_PLAYER_ACTOR, "MOVE 1 2 3 4"), BadBoardGameState);
}

BOOST_AUTO_TEST_CASE(MakeVictory) {
    // Emulates that victory is caused by this play() call which is moving a pawn eating another pawn at (2, 3) and
    // making a pawn appear at (5, 5)
    boardGame->playReturn = {
            {
                { Coordinates { 2, 3 }, Square::Free },
                { Coordinates { 5, 5 }, Square::White }
            },
            { 3, 3 }, { 1, 3 }
    };
    boardGame->playCallRoutine = [this]() {
        boardGame->victoryForReturn = Player::Black;
        boardGame->makeMove();
    };

    // Makes a move which will give victory to the black player
    BOOST_CHECK(service.handleRequestCommand(WHITE_PLAYER_ACTOR, "MOVE 1 2 3 4"));

    // Expect grid updates, victory and stop to be sync with clients with exactly 5 Service Events
    BOOST_CHECK_EQUAL(service.pollEvent(), "SQUARE_UPDATE 2 3 FREE");
    BOOST_CHECK_EQUAL(service.pollEvent(), "SQUARE_UPDATE 5 5 WHITE");
    BOOST_CHECK_EQUAL(service.pollEvent(), "MOVED 3 3 1 3");
    BOOST_CHECK_EQUAL(service.pollEvent(), "VICTORY_FOR BLACK");
    BOOST_CHECK_EQUAL(service.pollEvent(), "STOP");
    BOOST_CHECK(!service.checkEvent().has_value());

    // play() should have been called with parsed coordinates
    boostCheckPlayCall(*boardGame, { 1, 2 }, { 3, 4 });
}

BOOST_AUTO_TEST_CASE(TerminatedRound) {
    // Emulates that round termination is caused by this play() call which is moving a pawn
    boardGame->playReturn = {
            {}, { 5, 5 }, { 1, 1 }
    };
    boardGame->playCallRoutine = [this]() {
        boardGame->isRoundTerminatedReturn = true;
        boardGame->makeMove(); // At least one mocked move required to go for the next round
    };

    // Makes a move which will terminate the current round
    BOOST_CHECK(service.handleRequestCommand(WHITE_PLAYER_ACTOR, "MOVE 1 2 3 4"));

    // Expect nextRound() to have been called
    BOOST_CHECK(boardGame->nextRoundCalled);
    // Expect grid update and nextRound() call to be sync with clients with exactly 2 Service Events
    BOOST_CHECK_EQUAL(service.pollEvent(), "MOVED 5 5 1 1");
    BOOST_CHECK_EQUAL(service.pollEvent(), "ROUND_FOR BLACK");
    BOOST_CHECK(!service.checkEvent().has_value());
    // play() should have been called with parsed coordinates
    boostCheckPlayCall(*boardGame, { 1, 2 }, { 3, 4 });
}

BOOST_AUTO_TEST_CASE(StillInsideRound) {
    // Emulates play() only moving a pawn, without any side effect
    boardGame->playReturn = {
            {}, { 5, 5 }, { 1, 1 }
    };
    boardGame->playCallRoutine = [this]() {
        boardGame->makeMove(); // At least one mocked move required to go for the next round
    };

    // Makes a move which just perform a pawn move, neither round termination nor victory
    BOOST_CHECK(service.handleRequestCommand(WHITE_PLAYER_ACTOR, "MOVE 1 2 3 4"));

    // Just 1 SE emitted by play() call move
    BOOST_CHECK_EQUAL(service.pollEvent(), "MOVED 5 5 1 1");
    BOOST_CHECK(!service.checkEvent().has_value());
    // play() should have been called with parsed coordinates
    boostCheckPlayCall(*boardGame, { 1, 2 }, { 3, 4 });
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
