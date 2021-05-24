#include <RpT-Testing/TestingUtils.hpp>
#include <RpT-Testing/MinigamesServicesTestingUtils.hpp>

#include <Minigames-Services/Bermudes.hpp>


using namespace MinigamesServices;


/// Provides initialized Bermudes RpT-Minigame and a reference to its immutable grid
class BermudesFixture {
public:
    Bermudes game;
    const Grid& grid;

    BermudesFixture() : game {}, grid { game.grid() } {}
};


BOOST_FIXTURE_TEST_SUITE(BermudesTests, BermudesFixture)


BOOST_AUTO_TEST_CASE(MoveFromInvalidSquare) {
    BOOST_CHECK_THROW(game.play({ 4, 1 }, { 3, 1 }), BadSquareState);
}

BOOST_AUTO_TEST_CASE(DestinationKeptByCurrentPlayer) {
    BOOST_CHECK_THROW(game.play({ 7, 1 }, { 7, 2 }), BadSquareState);
}


/*
 * Elimination-take move unit tests
 */
BOOST_AUTO_TEST_SUITE(Elimination)


BOOST_AUTO_TEST_CASE(EliminatedPawnIsDirectNeighbour) {
    // Move white pawn to eat a black pawn in front of another black pawn so it will try to take a pawn which is a
    // direct neighbour for the next round
    game.play({ 7, 1 }, { 3 , 1 });
    game.nextRound();

    BOOST_CHECK_THROW(game.play({ 2, 1 }, { 3, 1 }), BadCoordinates);
}

BOOST_AUTO_TEST_CASE(TrajectoryBlocked) {
    BOOST_CHECK_THROW(game.play({ 8, 1 }, { 3, 1 }), BadSquareState);
}

BOOST_AUTO_TEST_CASE(DestinationSquareFree) {
    const GridUpdate updates { game.play({ 7, 2 }, { 3, 6 }) };

    // Checks for move being sent for clients to be correct
    BOOST_CHECK_EQUAL(updates.moveOrigin, (Coordinates { 7, 2 }));
    BOOST_CHECK_EQUAL(updates.moveDestination, (Coordinates { 3, 6 }));
    // No additional changes, just a pawn moved into another pawn to take it
    BOOST_CHECK_EQUAL(updates.updatedSquares.size(), 0);
    // Checks for grid to have been modified as expected
    BOOST_CHECK_EQUAL((grid[{ 7, 2 }]), Square::Free);
    BOOST_CHECK_EQUAL((grid[{ 3, 6 }]), Square::White);
    // Checks for pawn counts to have been changed, as every move in this game takes an opponent pawn
    BOOST_CHECK_EQUAL(game.pawnsFor(Player::White), 27);
    BOOST_CHECK_EQUAL(game.pawnsFor(Player::Black), 26);
    // After an elimination-take move, no flips chaining is available, it should terminate current round
    BOOST_CHECK(game.isRoundTerminated());
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * Flips-take move unit tests
 */
BOOST_AUTO_TEST_SUITE(Flip)


BOOST_AUTO_TEST_CASE(FlippedSquareIsEmpty) {
    BOOST_CHECK_THROW(game.play({ 7, 9 }, { 4, 6 }), BadSquareState);
}

BOOST_AUTO_TEST_CASE(TrajectoryBlocked) {
    // Move white pawn to eat a black pawn in front of another black pawn A with a black pawn B behind A. On the next
    // round, A will be able to try for flip-taking moved white pawn, but will be blocked because A is inside the
    // trajectory
    game.play({ 7, 1 }, { 3 , 1 });
    game.nextRound();

    BOOST_CHECK_THROW(game.play({ 1, 1 }, { 4, 1 }), BadSquareState);
}

BOOST_AUTO_TEST_CASE(TajectoryFreeAndFlippedSquareKeptByOpponent) {
    // Move white pawn to eat a black pawn in next to another black pawn so it will be able to flip-take black pawn
    // for the next round
    game.play({ 7, 2 }, { 3 , 6 });
    game.nextRound();

    const GridUpdate updates { game.play({ 2, 7 }, { 4, 5 }) };

    // Checks for move and square states being sent for clients to be correct
    BOOST_CHECK_EQUAL(updates.moveOrigin, (Coordinates { 2, 7 }));
    BOOST_CHECK_EQUAL(updates.moveDestination, (Coordinates { 4, 5 }));
    BOOST_CHECK_EQUAL(updates.updatedSquares.size(), 1); // 1 additional change for the flipped pawn
    BOOST_CHECK_EQUAL(updates.updatedSquares.at(0), (SquareUpdate { { 3, 6 }, Square::Black }));
    // Checks for grid to have been modified as expected
    BOOST_CHECK_EQUAL((grid[{ 7, 2 }]), Square::Free);
    BOOST_CHECK_EQUAL((grid[{ 3, 6 }]), Square::Black);
    BOOST_CHECK_EQUAL((grid[{ 2, 7 }]), Square::Free);
    // Checks for exactly 1 white pawn to have been replaced by a black pawn after a black pawn has been removed
    BOOST_CHECK_EQUAL(game.pawnsFor(Player::White), 26);
    BOOST_CHECK_EQUAL(game.pawnsFor(Player::Black), 27);
    // After a flip-take move, flips chaining is available, it should NOT terminate current round
    BOOST_CHECK(!game.isRoundTerminated());
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
