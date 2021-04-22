#include <RpT-Testing/TestingUtils.hpp>

#include <RpT-Core/Service.hpp>


using namespace RpT::Core;


/**
 * @brief Basic implementation to build Service class and test its defined methods
 *
 * `name()` returns empty string and `handleRequestCommand()` fires event with actor as event command, then returns
 * successfully.
 */
class TestingService : public Service {
public:
    template<typename... Timers>
    explicit TestingService(ServiceContext& run_context, Timers& ...watched_timers)
    : Service { run_context, { watched_timers... } } {}

    std::string_view name() const override {
        return "";
    }

    RpT::Utils::HandlingResult handleRequestCommand(uint64_t actor,
                                                    std::string_view) override {

        emitEvent(std::to_string(actor));

        return {};
    }
};


/**
 * @brief Provides `TestingService` instance with just initialized `ServiceContext` required for Service construction.
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
    // Checks if event command is correctly retrieved
    BOOST_CHECK_EQUAL(service.pollEvent(), "42");
    // There should not be any event still in queue
    BOOST_CHECK(!service.checkEvent().has_value());
}

/*
 * Many events triggered Service
 */

BOOST_AUTO_TEST_CASE(ManyQueuedEvents) {
    // Push 3 events
    for (int i { 0 }; i < 3; i++)
        service.handleRequestCommand(i, {});

    // Checks for each event in queue
    for (int i { 0 }; i < 3; i++) {
        // Checks for correct order with event ID
        RpT::Testing::boostCheckOptionalsEqual(service.checkEvent(), std::optional<std::size_t> { i });
        // Checks for correct event command
        BOOST_CHECK_EQUAL(service.pollEvent(), std::to_string(i));
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

BOOST_AUTO_TEST_SUITE_END()
