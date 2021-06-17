#include <RpT-Testing/TestingUtils.hpp>
#include <RpT-Testing/MinigamesServicesTestingUtils.hpp>

#include <Minigames-Services/Acores.hpp>


using namespace MinigamesServices;


/// Provides initialized Açores RpT-Minigame and a reference to its immutable grid
class AcoresFixture {
public:
    Acores game;
    const Grid& grid;

    AcoresFixture() : game {}, grid { game.grid() } {}
};


BOOST_FIXTURE_TEST_SUITE(AcoresTests, AcoresFixture)


BOOST_AUTO_TEST_CASE(MoveFromInvalidSquare) {
    BOOST_CHECK_THROW(game.play({ 4, 3 }, { 3, 3 }), BadSquareState);
}

BOOST_AUTO_TEST_CASE(DestinationTooFar) {
    BOOST_CHECK_THROW(game.play({ 1, 3 }, { 5, 3 }), BadCoordinates);
}


/*
 * Normal move unit tests
 */
BOOST_AUTO_TEST_SUITE(Normal)


BOOST_AUTO_TEST_CASE(DestinationSquareBusy) {
    BOOST_CHECK_THROW(game.play({ 1, 2 }, { 1, 3 }), BadSquareState);
}

BOOST_AUTO_TEST_CASE(InsideJumpsChaining) {
    // Moves a white pawn in the first place, allowing black pawn to jump for the next round
    game.play({ 2, 2 }, { 3, 3 });
    game.nextRound();
    // Then black player performs a jump move
    game.play({ 4, 4 }, { 2, 2 });

    // Now we should fail to perform a normal move back to where a white pawn has been eaten, because a jumps chaining
    // has begun and only jumps are allowed for this turn from now
    BOOST_CHECK_THROW(game.play({ 2, 2 }, { 3, 3 }), BadCoordinates);
}

BOOST_AUTO_TEST_CASE(DestinationSquareFree) {
    const GridUpdate updates { game.play({ 3, 2 }, { 3, 3 }) };

    // Checks for move being sent for clients to be correct
    BOOST_CHECK_EQUAL(updates.moveOrigin, (Coordinates { 3, 2 }));
    BOOST_CHECK_EQUAL(updates.moveDestination, (Coordinates { 3, 3 }));
    BOOST_CHECK_EQUAL(updates.updatedSquares.size(), 0); // No additional changes, just normal pawn move
    // Checks for grid to have been modified as expected
    BOOST_CHECK_EQUAL((grid[{ 3, 2 }]), Square::Free);
    BOOST_CHECK_EQUAL((grid[{ 3, 3 }]), Square::White);
    // Checks for pawn counts to haven't been changed, as it is a normal move
    BOOST_CHECK_EQUAL(game.pawnsFor(Player::White), 12);
    BOOST_CHECK_EQUAL(game.pawnsFor(Player::Black), 12);
    // After a normal move, no jumps chaining is available, it should terminate current round
    BOOST_CHECK(game.isRoundTerminated());
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * Jump move unit tests
 */
BOOST_AUTO_TEST_SUITE(Jump)


BOOST_AUTO_TEST_CASE(JumpedOverSquareKeptByCurrentPlayer) {
    BOOST_CHECK_THROW(game.play({ 3, 1 }, { 3, 3 }), BadSquareState);
}

BOOST_AUTO_TEST_CASE(DestinationSquareBusy) {
    BOOST_CHECK_THROW(game.play({ 2, 2 }, { 2, 4 }), BadSquareState);
}

BOOST_AUTO_TEST_CASE(DestinationSquareFree) {
    // Moves a white pawn in the first place, allowing black pawn to jump for the next round
    game.play({ 2, 2 }, { 3, 3 });
    game.nextRound();

    const GridUpdate updates { game.play({ 4, 4 }, { 2, 2 }) };

    // Checks for move and square states being sent for clients to be correct
    BOOST_CHECK_EQUAL(updates.moveOrigin, (Coordinates { 4, 4 }));
    BOOST_CHECK_EQUAL(updates.moveDestination, (Coordinates { 2, 2 }));
    BOOST_CHECK_EQUAL(updates.updatedSquares.size(), 1); // 1 additional change for the jumped over white pawn
    BOOST_CHECK_EQUAL(updates.updatedSquares.at(0), (SquareUpdate { { 3, 3 }, Square::Free }));
    // Checks for grid to have been modified as expected
    BOOST_CHECK_EQUAL((grid[{ 2, 2 }]), Square::Black);
    BOOST_CHECK_EQUAL((grid[{ 3, 3 }]), Square::Free);
    BOOST_CHECK_EQUAL((grid[{ 3, 3 }]), Square::Free);
    // Checks for exactly 1 white pawn to have been removed as it has been jumped over by a black pawn
    BOOST_CHECK_EQUAL(game.pawnsFor(Player::White), 11);
    BOOST_CHECK_EQUAL(game.pawnsFor(Player::Black), 12);
    // After a jump move, jumps chaining is available, it should NOT terminate current round
    BOOST_CHECK(!game.isRoundTerminated());
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
