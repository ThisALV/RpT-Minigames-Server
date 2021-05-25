#include <RpT-Testing/TestingUtils.hpp>

#include <vector>
#include <RpT-Core/ServiceContext.hpp>
#include <RpT-Core/Timer.hpp>


using namespace RpT::Core;


/// Instantiates `ServiceContext` instance which will provide token for timers construction
struct TokenProviderFixture {
    ServiceContext tokens_provider;
};

BOOST_FIXTURE_TEST_SUITE(TimerTests, TokenProviderFixture)

BOOST_AUTO_TEST_CASE(Constructor) {
    std::vector<Timer> testing_timers;

    for (unsigned int i { 0 }; i < 3; i++) // 3 timers to test, with 0, 100 and 200 ms of countdown respectively
        testing_timers.emplace_back(tokens_provider, i * 100);

    for (unsigned int i { 0 }; i < 3; i++) { // Checks for each timer construction
        Timer& timer { testing_timers.at(i) };

        BOOST_CHECK_EQUAL(timer.token(), i); // Checks for token provided by ServiceContext
        BOOST_CHECK_EQUAL(timer.countdown(), i * 100); // Checks for countdown assigned by ctor args
        BOOST_CHECK(!timer.isWaitingCountdown()); // Timer should not be initialized Ready
        BOOST_CHECK_NO_THROW(timer.requestCountdown()); // It should be possible to set timer Ready as it is Disabled
    }
}

/*
 * Current state accessors unit tests
 */

BOOST_AUTO_TEST_CASE(IsFree) {
    Timer timer { tokens_provider, 0 };

    // Each step of timer lifecycle, checking each time if current state is the one expected or not
    BOOST_CHECK(timer.isFree());
    timer.requestCountdown();
    BOOST_CHECK(!timer.isFree());
    timer.beginCountdown();
    BOOST_CHECK(!timer.isFree());
    timer.trigger();
    BOOST_CHECK(!timer.isFree());
}

BOOST_AUTO_TEST_CASE(IsWaitingCountdown) {
    Timer timer { tokens_provider, 0 };

    // Each step of timer lifecycle, checking each time if current state is the one expected or not
    BOOST_CHECK(!timer.isWaitingCountdown());
    timer.requestCountdown();
    BOOST_CHECK(timer.isWaitingCountdown());
    timer.beginCountdown();
    BOOST_CHECK(!timer.isWaitingCountdown());
    timer.trigger();
    BOOST_CHECK(!timer.isWaitingCountdown());
}

BOOST_AUTO_TEST_CASE(IsPending) {
    Timer timer { tokens_provider, 0 };

    // Each step of timer lifecycle, checking each time if current state is the one expected or not
    BOOST_CHECK(!timer.isPending());
    timer.requestCountdown();
    BOOST_CHECK(!timer.isPending());
    timer.beginCountdown();
    BOOST_CHECK(timer.isPending());
    timer.trigger();
    BOOST_CHECK(!timer.isPending());
}

BOOST_AUTO_TEST_CASE(HasTriggered) {
    Timer timer { tokens_provider, 0 };

    // Each step of timer lifecycle, checking each time if current state is the one expected or not
    BOOST_CHECK(!timer.hasTriggered());
    timer.requestCountdown();
    BOOST_CHECK(!timer.hasTriggered());
    timer.beginCountdown();
    BOOST_CHECK(!timer.hasTriggered());
    timer.trigger();
    BOOST_CHECK(timer.hasTriggered());
}

/*
 * Lifecycle-control methods unit test
 */

BOOST_AUTO_TEST_CASE(Lifecycle) {
    Timer timer { tokens_provider, 42 };

    for (int i { 0 }; i < 2; i++) { // Lifecycle should be resettable
        /*
         * Disabled state
         */
        BOOST_REQUIRE_THROW(timer.beginCountdown(), BadTimerState);
        BOOST_REQUIRE_THROW(timer.trigger(), BadTimerState);
        BOOST_REQUIRE_NO_THROW(timer.requestCountdown());

        /*
         * Ready state
         */
        BOOST_REQUIRE_THROW(timer.requestCountdown(), BadTimerState);
        BOOST_REQUIRE_THROW(timer.trigger(), BadTimerState);
        BOOST_REQUIRE_EQUAL(timer.beginCountdown(), 42); // Retrieved countdown is 42 ms

        /*
         * Pending state
         */
        BOOST_REQUIRE_THROW(timer.beginCountdown(), BadTimerState);
        BOOST_REQUIRE_THROW(timer.requestCountdown(), BadTimerState);
        BOOST_REQUIRE_NO_THROW(timer.trigger());

        /*
         * Triggered state
         */
        BOOST_REQUIRE_THROW(timer.beginCountdown(), BadTimerState);
        BOOST_REQUIRE_THROW(timer.trigger(), BadTimerState);
        BOOST_REQUIRE_THROW(timer.requestCountdown(), BadTimerState);
        BOOST_REQUIRE_NO_THROW(timer.clear());
    }
}

/*
 * Checks that clear() can be called at any lifecycle state
 */

BOOST_AUTO_TEST_CASE(Clear) {
    Timer timer { tokens_provider, 42 };

    /*
     * Performing check for each state, pushing timer until another state each time
     */

    // Checks from Disabled
    timer.clear();
    BOOST_REQUIRE(timer.isFree());

    // Checks from Ready
    timer.requestCountdown();
    timer.clear();
    BOOST_REQUIRE(timer.isFree());

    // Checks from Pending
    timer.requestCountdown();
    timer.beginCountdown();
    timer.clear();
    BOOST_REQUIRE(timer.isFree());

    // Checks from Triggered
    timer.requestCountdown();
    timer.beginCountdown();
    timer.trigger();
    timer.clear();
    BOOST_REQUIRE(timer.isFree());
}

BOOST_AUTO_TEST_SUITE_END()
