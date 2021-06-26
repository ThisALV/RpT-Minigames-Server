#include <RpT-Testing/TestingUtils.hpp>
#include <RpT-Testing/SerTestingUtils.hpp>

#include <RpT-Core/Service.hpp>


using namespace RpT::Core;


/**
 * @brief Basic implementation to build Service class and test its defined methods
 *
 * `name()` returns empty string and `handleRequestCommand()` fires event with actor as event command to every actor,
 * then send "FIRE" event to SR author, then returns successfully.
 */
class TestingService : public Service {
public:
    template<typename... Timers>
    explicit TestingService(ServiceContext& run_context, Timers& ...watched_timers)
    : Service { run_context, { watched_timers... } } {}

    std::string_view name() const override {
        return "";
    }

    RpT::Utils::HandlingResult handleRequestCommand(const std::uint64_t actor, const std::string_view) override {
        emitEvent(std::to_string(actor));
        emitEvent("FIRE", { actor });

        return {};
    }
};


/**
 * @brief Provides `TestingService` instance with just initialized `ServiceContext` required for Service construction.
 *
 * Also provides a facility te create new timers using the underlying private `ServiceContext`.
 *
 * Also, few timers are provided to test `getWatchingTimers()` features and manipulate directly timers without using
 * %Service.
 */
class ServiceTestFixture {
private:
    ServiceContext context_;

public:
    Timer timerA;
    Timer timerB;
    Timer timerC;
    TestingService service;

    ServiceTestFixture() : context_ {},
    timerA { context_, 0 }, timerB { context_, 0 }, timerC { context_, 0 },
    service { context_, timerA, timerB, timerC } {}

    /// Provides a new timer using underlying `ServiceContext`
    Timer timer() {
        return Timer { context_, 0 };
    }
};


BOOST_FIXTURE_TEST_SUITE(ServiceTests, ServiceTestFixture)

/*
 * Empty queue Service
 */

BOOST_AUTO_TEST_CASE(EmptyQueue) {
    // At construction, queue must be empty
    BOOST_CHECK(!service.checkEvent().has_value());
    // So polling event should throw error
    BOOST_CHECK_THROW(service.pollEvent(), EmptyEventsQueue);
}

/*
 * Single event triggered Service
 */

BOOST_AUTO_TEST_CASE(OneQueuedEvent) {
    // Triggers one event (see TestingService doc)
    service.handleRequestCommand(42, {});
    // Checks if event was triggered with correct ID
    RpT::Testing::boostCheckOptionalsEqual(service.checkEvent(), std::optional<std::size_t> { 0 });
    // Checks if event commands are correctly retrieved
    BOOST_CHECK_EQUAL(service.pollEvent(), ServiceEvent { "42" }); // A first event is broadcast with author UID
    BOOST_CHECK_EQUAL(service.pollEvent(), (ServiceEvent { "FIRE", { { 42 } } })); // Then author receives FIRE
    // There should not be any event still in queue
    BOOST_CHECK(!service.checkEvent().has_value());
}

/*
 * Many events triggered Service
 */

BOOST_AUTO_TEST_CASE(ManyQueuedEvents) {
    // Push 3 events for 3 different actor UIDs
    for (std::uint64_t i { 0 }; i < 3; i++)
        service.handleRequestCommand(i, {});

    // Checks for each event in queue
    for (std::uint64_t i { 0 }; i < 3; i++) {
        // Checks for correct order with event ID
        RpT::Testing::boostCheckOptionalsEqual(service.checkEvent(), std::optional<std::size_t> { i });
        // Checks for correct event commands
        BOOST_CHECK_EQUAL(service.pollEvent(), ServiceEvent { std::to_string(i) });
        BOOST_CHECK_EQUAL(service.pollEvent(), (ServiceEvent { "FIRE", { { i } } }));
    }

    // Now, queue should be empty
    BOOST_CHECK(!service.checkEvent().has_value());
}

/*
 * getWaitingTimers() unit tests
 */

BOOST_AUTO_TEST_SUITE(GetWaitingTimers)

BOOST_AUTO_TEST_CASE(AllTimersDisabled) {
    BOOST_CHECK(service.getWaitingTimers().empty()); // All timers are constructed as Disabled
}

BOOST_AUTO_TEST_CASE(AllTimersDisabledOrPending) {
    // Puts timer into Pending state
    timerA.requestCountdown();
    timerA.beginCountdown();

    BOOST_CHECK(service.getWaitingTimers().empty()); // Still not any ready timer
}

BOOST_AUTO_TEST_CASE(SomeTimersReady) {
    // Puts timers into Ready state
    timerA.requestCountdown();
    timerC.requestCountdown();

    const auto waiting_timers { service.getWaitingTimers() };
    BOOST_CHECK_EQUAL(waiting_timers.size(), 2); // 2 timers put into Ready state
    BOOST_CHECK_EQUAL(waiting_timers.at(0).get().token(), 0); // 1st one is A
    BOOST_CHECK_EQUAL(waiting_timers.at(1).get().token(), 2); // 2nd one is C
}

BOOST_AUTO_TEST_CASE(AllTimersReady) {
    // Puts timers into Ready state
    timerA.requestCountdown();
    timerB.requestCountdown();
    timerC.requestCountdown();

    const auto waiting_timers { service.getWaitingTimers() };
    BOOST_CHECK_EQUAL(waiting_timers.size(), 3); // 2 timers put into Ready state
    BOOST_CHECK_EQUAL(waiting_timers.at(0).get().token(), 0); // 1st one is A
    BOOST_CHECK_EQUAL(waiting_timers.at(1).get().token(), 1); // 2nd one is B
    BOOST_CHECK_EQUAL(waiting_timers.at(2).get().token(), 2); // 3rd one is C
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * watchTimer() method unit tests
 */

BOOST_AUTO_TEST_SUITE(WatchTimer)

BOOST_AUTO_TEST_CASE(AlreadyWatched) {
    BOOST_CHECK_THROW(service.watchTimer(timerB), BadWatchedToken);
}

BOOST_AUTO_TEST_CASE(NotWatched) {
    Timer timerD { timer() };

    BOOST_CHECK_NO_THROW(service.watchTimer(timerD));

    // Puts new timer into Ready state so we can check if it is returned by getWatchingTimers()
    timerD.requestCountdown();
    const std::vector<std::reference_wrapper<Timer>> watching_timers { service.getWaitingTimers() };
    // Only timerD was put into Ready state
    BOOST_CHECK_EQUAL(watching_timers.size(), 1);
    BOOST_CHECK_EQUAL(watching_timers.at(0).get().token(), timerD.token());
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * forgetTimer() method unit tests
 */

BOOST_AUTO_TEST_SUITE(ForgetTimer)

BOOST_AUTO_TEST_CASE(Watched) {
    BOOST_CHECK_NO_THROW(service.forgetTimer(timerA));

    // Puts timerA into Ready state so we can check if it is not returned by getWatchingTimers()
    timerA.requestCountdown();
    const std::vector<std::reference_wrapper<Timer>> watching_timers { service.getWaitingTimers() };
    // Only timerA was put into Ready state, and it is no longer watched
    BOOST_CHECK_EQUAL(watching_timers.size(), 0);
}

BOOST_AUTO_TEST_CASE(NotWatched) {
    Timer timerD { timer() };

    BOOST_CHECK_THROW(service.forgetTimer(timerD), BadWatchedToken);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
