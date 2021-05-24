#include <RpT-Testing/TestingUtils.hpp>
#include <RpT-Testing/MinigamesServicesTestingUtils.hpp>

#include <Minigames-Services/Canaries.hpp>


using namespace MinigamesServices;


/// This mock allows to reset grid for required state at any moment, it is useful to test cases where players are
/// blocked in this game
class MockedCanaries : public Canaries {
public:
    MockedCanaries() : Canaries {} {}

    /// Resets underlying game grid to grid constructed with given initial configuration
    void resetGrid(const std::initializer_list<std::initializer_list<Square>> initial_configuration) {
        game_grid_ = Grid { initial_configuration };
    }
};


/// Provides initialized & mocked Canaries RpT-Minigame and a reference to its immutable grid
class CanariesFixture {
public:
    MockedCanaries game;
    const Grid& grid;

    CanariesFixture() : game {}, grid { game.grid() } {}
};


BOOST_FIXTURE_TEST_SUITE(CanariesTests, CanariesFixture)


BOOST_AUTO_TEST_CASE(MoveFromInvalidSquare) {
    BOOST_CHECK_THROW(game.play({ 1, 1 }, { 3, 1 }), BadSquareState);
}

BOOST_AUTO_TEST_CASE(DestinationTooFar) {
    BOOST_CHECK_THROW(game.play({ 4, 1 }, { 4, 4 }), BadCoordinates);
}

BOOST_AUTO_TEST_CASE(DiagonalAxis) {
    BOOST_CHECK_THROW(game.play({ 4, 1 }, { 3, 2 }), BadCoordinates);
}


/*
 * Normal move unit tests
 */
BOOST_AUTO_TEST_SUITE(Normal)


BOOST_AUTO_TEST_CASE(DestinationSquareBusy) {
    BOOST_CHECK_THROW(game.play({ 4, 1 }, { 3, 1 }), BadSquareState);
}

BOOST_AUTO_TEST_CASE(DestinationSquareFree) {
    // Let a free case into white side so a normal move can be done
    game.resetGrid({
                           { BLACK, BLACK, BLACK, BLACK },
                           { BLACK, BLACK, BLACK, BLACK },
                           { WHITE, WHITE, WHITE, WHITE },
                           { EMPTY, WHITE, WHITE, WHITE }
    });

    const GridUpdate updates { game.play({ 4, 2 }, { 4, 1 }) };

    // Checks for move being sent for clients to be correct
    BOOST_CHECK_EQUAL(updates.moveOrigin, (Coordinates { 4, 2 }));
    BOOST_CHECK_EQUAL(updates.moveDestination, (Coordinates { 4, 1 }));
    BOOST_CHECK_EQUAL(updates.updatedSquares.size(), 0); // No additional changes, just normal pawn move
    // Checks for grid to have been modified as expected
    BOOST_CHECK_EQUAL((grid[{ 4, 2 }]), Square::Free);
    BOOST_CHECK_EQUAL((grid[{ 4, 1 }]), Square::White);
    // Checks for pawn counts to haven't been changed, as it is a normal move (resetGrid() doesn't affect pawn counts)
    BOOST_CHECK_EQUAL(game.pawnsFor(Player::White), 8);
    BOOST_CHECK_EQUAL(game.pawnsFor(Player::Black), 8);
    // No chaining available for this game, round is always terminated after a move
    BOOST_CHECK(game.isRoundTerminated());
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * Eat move unit tests
 */
BOOST_AUTO_TEST_SUITE(Eat)


BOOST_AUTO_TEST_CASE(JumpedOverSquareIsEmpty) {
    // Let a free case into white side so a white pawn can try to jump over it
    game.resetGrid({
                           { BLACK, BLACK, BLACK, BLACK },
                           { BLACK, BLACK, BLACK, BLACK },
                           { WHITE, WHITE, EMPTY, WHITE },
                           { WHITE, WHITE, WHITE, WHITE }
    });

    BOOST_CHECK_THROW(game.play({ 4, 3 }, { 2, 3 }), BadSquareState);
}

BOOST_AUTO_TEST_CASE(DestinationSquareKeptByCurrentPlayer) {
    BOOST_CHECK_THROW(game.play({ 4, 3 }, { 4, 1 }), BadSquareState);
}

BOOST_AUTO_TEST_CASE(JumpWithEatAvailable) {
    const GridUpdate updates { game.play({ 4, 3 }, { 2, 3 }) };

    // Checks for move and square states being sent for clients to be correct
    BOOST_CHECK_EQUAL(updates.moveOrigin, (Coordinates { 4, 3 }));
    BOOST_CHECK_EQUAL(updates.moveDestination, (Coordinates { 2, 3 }));
    BOOST_CHECK_EQUAL(updates.updatedSquares.size(), 0); // 0 additional change, jumped over square isn't modified
    // Checks for grid to have been modified as expected
    BOOST_CHECK_EQUAL((grid[{ 4, 3 }]), Square::Free);
    BOOST_CHECK_EQUAL((grid[{ 2, 3 }]), Square::White);
    // Checks for exactly 1 black pawn to have been removed as it has been eaten by the moved white pawn
    BOOST_CHECK_EQUAL(game.pawnsFor(Player::White), 8);
    BOOST_CHECK_EQUAL(game.pawnsFor(Player::Black), 7);
    // No chaining available for this game, round is always terminated after a move
    BOOST_CHECK(game.isRoundTerminated());
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * victoryFor() overridden method unit tests
 */
BOOST_AUTO_TEST_SUITE(VictoryFor)


BOOST_AUTO_TEST_CASE(WhitePlayerBlocked) {
    game.resetGrid({
                           { WHITE, BLACK, BLACK, WHITE },
                           { BLACK, EMPTY, EMPTY, BLACK },
                           { EMPTY, EMPTY, EMPTY, EMPTY },
                           { EMPTY, EMPTY, EMPTY, EMPTY }
    });

    BOOST_CHECK_EQUAL(game.victoryFor().value(), Player::Black);
}

BOOST_AUTO_TEST_CASE(BlackPlayerBlocked) {
    game.resetGrid({
                           { BLACK, WHITE, WHITE, BLACK },
                           { WHITE, EMPTY, EMPTY, WHITE },
                           { EMPTY, EMPTY, EMPTY, EMPTY },
                           { EMPTY, EMPTY, EMPTY, EMPTY }
    });

    BOOST_CHECK_EQUAL(game.victoryFor().value(), Player::White);
}

BOOST_AUTO_TEST_CASE(NormalMoveAvailable) {
    game.resetGrid({ // Normal move from (1, 1) to (1, 2) available
                           { WHITE, EMPTY, BLACK, WHITE },
                           { BLACK, EMPTY, EMPTY, BLACK },
                           { EMPTY, EMPTY, EMPTY, EMPTY },
                           { EMPTY, EMPTY, EMPTY, EMPTY }
    });

    BOOST_CHECK(!game.victoryFor().has_value());
}

BOOST_AUTO_TEST_CASE(EatMoveAvailable) {
    game.resetGrid({ // Eat move from (1, 1) to (1, 3) available
                           { WHITE, WHITE, BLACK, WHITE },
                           { BLACK, EMPTY, EMPTY, BLACK },
                           { EMPTY, EMPTY, EMPTY, EMPTY },
                           { EMPTY, EMPTY, EMPTY, EMPTY }
    });

    BOOST_CHECK(!game.victoryFor().has_value());
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
