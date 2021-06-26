#include <RpT-Testing/TestingUtils.hpp>
#include <RpT-Testing/MinigamesServicesTestingUtils.hpp>
#include <RpT-Testing/SerTestingUtils.hpp>

#include <Minigames-Services/Acores.hpp>
#include <Minigames-Services/LobbyService.hpp>
#include <Minigames-Services/MinigameService.hpp>
#include <RpT-Core/ServiceContext.hpp>


using namespace MinigamesServices;


/// Provides a new Rpt-Minigame Açores. The minigame doesn't matter, only its owner %Service MinigameService state will
/// be checked for
std::unique_ptr<Acores> provideTestingMinigame() {
    return std::make_unique<Acores>();
}


/// Provides a minigame service for %Service construction and an initialized service using this MinigameService
class MinigameFixture {
private:
    RpT::Core::ServiceContext context_;

public:
    MinigameService minigame;
    LobbyService service;

    MinigameFixture()
    : minigame { context_, provideTestingMinigame },
    service { context_, minigame, 42 } {}
};


BOOST_FIXTURE_TEST_SUITE(LobbyServiceTests, MinigameFixture)


/// UID used by actor assigned to the white player into these tests
constexpr std::uint64_t WHITE_PLAYER_ACTOR { 0 };
/// UID used by actor assigned to the black player into these tests
constexpr std::uint64_t BLACK_PLAYER_ACTOR { 1 };


/*
 * assignActor() method unit tests
 */
BOOST_AUTO_TEST_SUITE(AssignActor)


BOOST_AUTO_TEST_CASE(NoPlayerAssigned) {
    // Try to assign white player will be performed first
    BOOST_CHECK_EQUAL(service.assignActor(WHITE_PLAYER_ACTOR), Player::White);
    // No other actor is already ready
    BOOST_CHECK(!service.checkEvent());
}

BOOST_AUTO_TEST_CASE(WhitePlayerAssigned) {
    service.assignActor(WHITE_PLAYER_ACTOR); // Assigned to white player
    // As white player is assigned, try to assign black player will be performed second
    BOOST_CHECK_EQUAL(service.assignActor(BLACK_PLAYER_ACTOR), Player::Black);
    // No other actor is already ready
    BOOST_CHECK(!service.checkEvent());
}

BOOST_AUTO_TEST_CASE(BlackPlayerAssigned) {
    service.assignActor(WHITE_PLAYER_ACTOR); // Assigned to white player
    service.assignActor(BLACK_PLAYER_ACTOR); // Assigned to black player
    service.removeActor(WHITE_PLAYER_ACTOR); // Only black player remains

    // Try to assign white player will be performed first
    BOOST_CHECK_EQUAL(service.assignActor(WHITE_PLAYER_ACTOR), Player::White);
    // No other actor is already ready
    BOOST_CHECK(!service.checkEvent());
}

BOOST_AUTO_TEST_CASE(BothPlayersAssigned) {
    service.assignActor(WHITE_PLAYER_ACTOR); // Assigned to white player
    service.assignActor(BLACK_PLAYER_ACTOR); // Assigned to black player

    // No more slot available
    BOOST_CHECK_THROW(service.assignActor(2), BadPlayersState);
    // No other actor is already ready
    BOOST_CHECK(!service.checkEvent());
}

BOOST_AUTO_TEST_CASE(WhitePlayerReady) {
    service.assignActor(WHITE_PLAYER_ACTOR); // Assigned to white player

    service.handleRequestCommand(WHITE_PLAYER_ACTOR, "READY"); // White player is ready
    service.pollEvent(); // Clears event emitted by black player being ready

    // As white player is assigned, try to assign black player will be performed second
    BOOST_CHECK_EQUAL(service.assignActor(BLACK_PLAYER_ACTOR), Player::Black);
    // White player was ready at assignation, new player must be notified about it
    BOOST_CHECK_EQUAL(service.pollEvent(), (RpT::Core::ServiceEvent { "READY_PLAYER 0", { { BLACK_PLAYER_ACTOR } } }));
    BOOST_CHECK(!service.checkEvent());
}

BOOST_AUTO_TEST_CASE(BlackPlayerReady) {
    service.assignActor(WHITE_PLAYER_ACTOR); // Assigned to white player
    service.assignActor(BLACK_PLAYER_ACTOR); // Assigned to black player
    service.removeActor(WHITE_PLAYER_ACTOR); // Only black player remains

    service.handleRequestCommand(BLACK_PLAYER_ACTOR, "READY"); // White player is ready
    service.pollEvent(); // Clears event emitted by white player being ready

    // As white player is assigned, try to assign black player will be performed second
    BOOST_CHECK_EQUAL(service.assignActor(WHITE_PLAYER_ACTOR), Player::White);
    // White player was ready at assignation, new player must be notified about it
    BOOST_CHECK_EQUAL(service.pollEvent(), (RpT::Core::ServiceEvent { "READY_PLAYER 1", { { WHITE_PLAYER_ACTOR } } }));
    BOOST_CHECK(!service.checkEvent());
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * removeActor() method unit tests
 */
BOOST_AUTO_TEST_SUITE(RemoveActor)


BOOST_AUTO_TEST_CASE(ActorNotAssigned) {
    BOOST_CHECK_THROW(service.removeActor(WHITE_PLAYER_ACTOR), BadPlayersState);
}

BOOST_AUTO_TEST_CASE(ActorAssignedToWhite) {
    service.assignActor(WHITE_PLAYER_ACTOR); // Assigned to white player

    BOOST_CHECK_NO_THROW(service.removeActor(WHITE_PLAYER_ACTOR));
    // Was removed, so it should be assigned to white player which is available again
    BOOST_CHECK_EQUAL(service.assignActor(WHITE_PLAYER_ACTOR), Player::White);
}

BOOST_AUTO_TEST_CASE(ActorAssignedToBlack) {
    service.assignActor(WHITE_PLAYER_ACTOR); // Assigned to white player
    service.assignActor(BLACK_PLAYER_ACTOR); // Assigned to black player

    BOOST_CHECK_NO_THROW(service.removeActor(BLACK_PLAYER_ACTOR));
    // Was removed, so it should be assigned to white player which is available again
    BOOST_CHECK_EQUAL(service.assignActor(BLACK_PLAYER_ACTOR), Player::Black);
}

BOOST_AUTO_TEST_CASE(ActorWasReady) {
    service.assignActor(WHITE_PLAYER_ACTOR); // Assigned to white player
    service.assignActor(BLACK_PLAYER_ACTOR); // Assigned to black player

    // Both players are ready, starting countdown
    service.handleRequestCommand(WHITE_PLAYER_ACTOR, "READY");
    service.handleRequestCommand(BLACK_PLAYER_ACTOR, "READY");
    // Consumes events emitted by test preparation
    while (service.checkEvent().has_value())
        service.pollEvent();

    // Keeps an access to countdown to check if it will be disabled as expected
    RpT::Core::Timer& countdown_before_cancellation { service.getWaitingTimers().at(0).get() };
    // And to put it into Pending state
    countdown_before_cancellation.beginCountdown();

    service.removeActor(WHITE_PLAYER_ACTOR); // Countdown is cancelled, not enough players to start
    // Exactly one SE emitted to sync clients about countdown cancellation
    BOOST_CHECK_EQUAL(service.pollEvent(), RpT::Core::ServiceEvent { "END_COUNTDOWN" });
    BOOST_CHECK(!service.checkEvent().has_value());
    // Checks for countdown timer cancellation to have been done
    BOOST_CHECK(countdown_before_cancellation.isFree());
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * SR commands handling unit tests
 */
BOOST_AUTO_TEST_SUITE(HandleRequestCommand)


BOOST_AUTO_TEST_CASE(NotAssignedAuthor) {
    // No actor assigned to any player
    BOOST_CHECK_THROW(service.handleRequestCommand(WHITE_PLAYER_ACTOR, " READY  "), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(NewReadyNotStarting) {
    service.assignActor(WHITE_PLAYER_ACTOR); // Assigned to white player

    service.handleRequestCommand(WHITE_PLAYER_ACTOR, "READY"); // White player is ready
    // Notifies that a player is ready using 1 SE
    BOOST_CHECK_EQUAL(service.pollEvent(), RpT::Core::ServiceEvent { "READY_PLAYER " + std::to_string(WHITE_PLAYER_ACTOR) });
    BOOST_CHECK(!service.checkEvent().has_value());

    // Countdown hasn't begun, only 1 player ready
    BOOST_CHECK_EQUAL(service.getWaitingTimers().size(), 0);
}

BOOST_AUTO_TEST_CASE(NewReadyStarting) {
    service.assignActor(WHITE_PLAYER_ACTOR); // Assigned to white player
    service.assignActor(BLACK_PLAYER_ACTOR); // Assigned to black player

    // Both players are ready, begins countdown
    service.handleRequestCommand(WHITE_PLAYER_ACTOR, "READY");
    service.handleRequestCommand(BLACK_PLAYER_ACTOR, "READY");
    // 3 SE emitted: both players are ready and the countdown has begun
    BOOST_CHECK_EQUAL(service.pollEvent(), RpT::Core::ServiceEvent { "READY_PLAYER " + std::to_string(WHITE_PLAYER_ACTOR) });
    BOOST_CHECK_EQUAL(service.pollEvent(), RpT::Core::ServiceEvent { "READY_PLAYER " + std::to_string(BLACK_PLAYER_ACTOR) });
    BOOST_CHECK_EQUAL(service.pollEvent(), RpT::Core::ServiceEvent { "BEGIN_COUNTDOWN 42" });
    BOOST_CHECK(!service.checkEvent().has_value());

    // Countdown has begun because players are both ready
    const auto waiting_timers { service.getWaitingTimers() };
    BOOST_CHECK_EQUAL(waiting_timers.size(), 1);
    // Starting countdown timer has token 0 because it is the only timer initialized for this test
    BOOST_CHECK_EQUAL(waiting_timers.at(0).get().token(), 0);
}

BOOST_AUTO_TEST_CASE(StartCancelledWere2Ready) {
    service.assignActor(WHITE_PLAYER_ACTOR); // Assigned to white player
    service.assignActor(BLACK_PLAYER_ACTOR); // Assigned to black player

    // Both players are ready, begins countdown
    service.handleRequestCommand(WHITE_PLAYER_ACTOR, "READY");
    service.handleRequestCommand(BLACK_PLAYER_ACTOR, "READY");
    // Consumes events emitted by test preparation
    while (service.checkEvent().has_value())
        service.pollEvent();

    // Keeps an access to countdown to check if it will be disabled as expected
    RpT::Core::Timer& countdown_before_cancellation { service.getWaitingTimers().at(0).get() };
    // And to put it into Pending state
    countdown_before_cancellation.beginCountdown();

    // No more enough players to start because one player is no longer ready
    service.handleRequestCommand(BLACK_PLAYER_ACTOR, "READY");
    // 2 SE emitted: player is no longer ready and countdown is cancelled
    BOOST_CHECK_EQUAL(service.pollEvent(), RpT::Core::ServiceEvent { "WAITING_FOR_PLAYER " + std::to_string(BLACK_PLAYER_ACTOR) });
    BOOST_CHECK_EQUAL(service.pollEvent(), RpT::Core::ServiceEvent { "END_COUNTDOWN" });
    BOOST_CHECK(!service.checkEvent().has_value());
    // Checks for countdown timer cancellation to have been done
    BOOST_CHECK(countdown_before_cancellation.isFree());
}

BOOST_AUTO_TEST_CASE(StartCancelledWas1Ready) {
    service.assignActor(WHITE_PLAYER_ACTOR); // Assigned to white player
    service.handleRequestCommand(WHITE_PLAYER_ACTOR, "READY"); // White player is ready
    // Consumes events emitted by test preparation
    while (service.checkEvent().has_value())
        service.pollEvent();

    service.handleRequestCommand(WHITE_PLAYER_ACTOR, "READY");
    // Notifies that a player is ready using 1 SE
    BOOST_CHECK_EQUAL(service.pollEvent(), RpT::Core::ServiceEvent { "WAITING_FOR_PLAYER " + std::to_string(WHITE_PLAYER_ACTOR) });
    BOOST_CHECK(!service.checkEvent().has_value());
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * notifyWaiting() method unit tests
 */
BOOST_AUTO_TEST_SUITE(NotifyWaiting)


BOOST_AUTO_TEST_CASE(GameRunning) {
    // Mocks game is already started
    minigame.start(WHITE_PLAYER_ACTOR, BLACK_PLAYER_ACTOR);

    BOOST_CHECK_THROW(service.notifyWaiting(), BadBoardGameState);
}

BOOST_AUTO_TEST_CASE(GameStopped) {
    service.notifyWaiting(); // Send WAITING Service Event

    // Checks it has been sent as expected, in only 1 Service Event command
    BOOST_CHECK_EQUAL(service.pollEvent(), RpT::Core::ServiceEvent { "WAITING" });
    BOOST_CHECK(!service.checkEvent().has_value());
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
