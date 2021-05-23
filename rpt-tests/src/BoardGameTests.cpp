#include <RpT-Testing/TestingUtils.hpp>
#include <RpT-Testing/MinigamesServicesTestingUtils.hpp>

#include <Minigames-Services/BoardGame.hpp>


using namespace MinigamesServices;

BOOST_AUTO_TEST_SUITE(BoardGameTests)


/// Sample child class with empty method implementations to test for non-virtual methods of `BoardGame` class
class SampleBoardGame : public BoardGame {
public:
    /// Accessible ctor for protected parent ctor with a customizable threshold to test parent ctor threshold checking
    /// Initial numbers of pawns: White=5, Black=4
    SampleBoardGame(std::initializer_list<std::initializer_list<Square>> initial_grid, const unsigned int threshold = 1)
    : BoardGame { initial_grid, 5, 4, threshold } {}

    /// Accessor for protected inherited grid member variable
    Grid& grid() {
        return game_grid_;
    }

    /// Accessible protected method `moved()`
    void makeMove() {
        moved();
    }

    /// Mocks a specific number of pawns inside grid for white player
    void whitePawns(const unsigned int white_pawns) {
        white_pawns_ = white_pawns;
    }

    /// Mocks a specific number of pawns inside grid for black player
    void blackPawns(const unsigned int black_pawns) {
        black_pawns_ = black_pawns;
    }

    /*
     * These methods are defined by implementation, not tested in this unit tests file
     */

    bool isRoundTerminated() const override { return false; }
    GridUpdate play(const Coordinates&, const Coordinates&) override { return {}; }
};


/*
 * colorFor() free function unit tests
 */
BOOST_AUTO_TEST_SUITE(ColorFor)


BOOST_AUTO_TEST_CASE(WhitePlayer) {
    BOOST_CHECK_EQUAL(colorFor(Player::White), Square::White);
}

BOOST_AUTO_TEST_CASE(BlackPlayer) {
    BOOST_CHECK_EQUAL(colorFor(Player::Black), Square::Black);
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * Ctor unit tests
 */
BOOST_AUTO_TEST_SUITE(Constructor)


BOOST_AUTO_TEST_CASE(ZeroPawnsThreshold) {
    BOOST_CHECK_THROW((SampleBoardGame { { { EMPTY } }, 0 }), std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(ValidInitialGrid) {
    SampleBoardGame game {
            { EMPTY, EMPTY, EMPTY, EMPTY },
            { EMPTY, BLACK, EMPTY, EMPTY },
            { EMPTY, EMPTY, EMPTY, WHITE },
            { EMPTY, EMPTY, EMPTY, EMPTY }
    };

    // Checks for white player to play first
    BOOST_CHECK_EQUAL(game.currentRound(), Player::White);

    // Checks for game grid to have been initialized as expected
    Grid& grid { game.grid() };
    for (int line { 1 }; line <= 4; line++) { // Checks for each square inside grid to have the right state
        for (int column { 1 }; column <= 4; column++) {
            const Coordinates currently_checked_square { line, column };

            Square expected_state; // Initializes expected square state depending on current square coordinates
            if (currently_checked_square == Coordinates { 2, 2 })
                expected_state = BLACK;
            else if (currently_checked_square == Coordinates { 3, 4 })
                expected_state = WHITE;
            else // Every other square is empty
                expected_state = EMPTY;

            BOOST_CHECK_EQUAL(grid[currently_checked_square], expected_state);
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * pawnsFor() method unit tests
 */
BOOST_AUTO_TEST_SUITE(PawnsFor)


BOOST_AUTO_TEST_CASE(WhitePlayer) {
    const SampleBoardGame game { { EMPTY } };

    BOOST_CHECK_EQUAL(game.pawnsFor(Player::White), 5); // Mocked BoardGame initialized with 5 white pawns
}

BOOST_AUTO_TEST_CASE(BlackPlayer) {
    const SampleBoardGame game { { EMPTY } };

    BOOST_CHECK_EQUAL(game.pawnsFor(Player::Black), 4); // Mocked BoardGame initialized with 5 white pawns
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * victoryFor() method unit tests
 */
BOOST_AUTO_TEST_SUITE(VictoryFor)


BOOST_AUTO_TEST_CASE(EverybodyAboveOrEqualThreshold) {
    SampleBoardGame game { { { EMPTY } }, 5 };
    game.whitePawns(5); // whitePawns == threshold
    game.blackPawns(6); // blackPawns > threshold

    BOOST_CHECK(!game.victoryFor().has_value());
}

BOOST_AUTO_TEST_CASE(WhiteBehindThreshold) {
    SampleBoardGame game { { { EMPTY } }, 5 };
    game.whitePawns(4); // whitePawns < threshold, his opponent (black) won
    game.blackPawns(6); // blackPawns > threshold

    const std::optional<Player> winner { game.victoryFor() };
    BOOST_REQUIRE(winner.has_value());
    BOOST_CHECK_EQUAL(*winner, Player::Black);
}

BOOST_AUTO_TEST_CASE(BlackBehindThreshold) {
    SampleBoardGame game { { { EMPTY } }, 5 };
    game.whitePawns(5); // whitePawns == threshold
    game.blackPawns(4); // blackPawns < threshold, his opponent (white) won

    const std::optional<Player> winner { game.victoryFor() };
    BOOST_REQUIRE(winner.has_value());
    BOOST_CHECK_EQUAL(*winner, Player::White);
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * nextRound() method unit tests
 */
BOOST_AUTO_TEST_SUITE(NextRound)

/*
 * moved() called prior to nextRound() call
 */
BOOST_AUTO_TEST_SUITE(HasMoved)


BOOST_AUTO_TEST_CASE(CurrentPlayerIsWhite) {
    SampleBoardGame game { { EMPTY } };

    // Current player is white: switch to black player
    game.makeMove(); // Moves to be able to end current round
    BOOST_CHECK_EQUAL(game.nextRound(), Player::Black);
    BOOST_CHECK_EQUAL(game.currentRound(), Player::Black);
}

BOOST_AUTO_TEST_CASE(CurrentPlayerIsBlack) {
    SampleBoardGame game { { EMPTY } };

    // Go to black player in the first place
    game.makeMove();
    game.nextRound();

    // Current player is black: switch to white player
    game.makeMove();
    BOOST_CHECK_EQUAL(game.nextRound(), Player::White);
    BOOST_CHECK_EQUAL(game.currentRound(), Player::White);
}


BOOST_AUTO_TEST_SUITE_END()

/*
 * moved() not called
 */
BOOST_AUTO_TEST_SUITE(HasNotMoved)


BOOST_AUTO_TEST_CASE(CurrentPlayerIsWhite) {
    SampleBoardGame game { { EMPTY } };

    // Current player hasn't moved, he's gonna try to skip his round
    BOOST_CHECK_THROW(game.nextRound(), MoveRequired);
    BOOST_CHECK_EQUAL(game.currentRound(), Player::White); // He still has to plau has no move were made
}

BOOST_AUTO_TEST_CASE(CurrentPlayerIsBlack) {
    SampleBoardGame game { { EMPTY } };

    // Go to black player in the first place
    game.makeMove(); // Makes move for being able to go for next round which is black player's round
    game.nextRound();

    // Current player hasn't moved, he's gonna try to skip his round
    BOOST_CHECK_THROW(game.nextRound(), MoveRequired);
    BOOST_CHECK_EQUAL(game.currentRound(), Player::Black); // He still has to plau has no move were made
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
