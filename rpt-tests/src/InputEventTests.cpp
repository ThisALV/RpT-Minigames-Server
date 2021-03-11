#include <RpT-Testing/TestingUtils.hpp>

#include <cassert>
#include <iostream>
#include <RpT-Core/InputEvent.hpp>


using namespace RpT::Core;


// << overloads must be defined inside this namespace, so ADL can be performed to find them when boost use <<
// operator on RpT::Core types
namespace RpT::Core {


// Required for Boost.Test logging
std::ostream& operator<<(std::ostream& out, const LeftEvent::Reason dc_reason) {
    switch (dc_reason) {
    case LeftEvent::Reason::Clean:
        return out << "Clean";
    case LeftEvent::Reason::Crash:
        return out << "Crash";
    }
}


}


BOOST_AUTO_TEST_SUITE(InputEventTests)

/*
 * None event
 */

BOOST_AUTO_TEST_SUITE(None)

BOOST_AUTO_TEST_CASE(ActorName) {
    const NoneEvent event { 42 };

    BOOST_CHECK_EQUAL(event.actor(), 42);
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * ServiceRequest event
 */

BOOST_AUTO_TEST_SUITE(ServiceRequest)

BOOST_AUTO_TEST_CASE(ActorNameAndInvalidRequest) {
    const ServiceRequestEvent event { 42, "A random string" };

    BOOST_CHECK_EQUAL(event.actor(), 42);
    // InputEvent shouldn't care about request validity
    BOOST_CHECK_EQUAL(event.serviceRequest(), "A random string");
}

BOOST_AUTO_TEST_CASE(ActorNameAndValidRequest) {
    const ServiceRequestEvent event { 42, "REQUEST Service command" };

    BOOST_CHECK_EQUAL(event.actor(), 42);
    BOOST_CHECK_EQUAL(event.serviceRequest(), "REQUEST Service command");
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * TimerTrigger event
 */

BOOST_AUTO_TEST_SUITE(TimerTrigger)

BOOST_AUTO_TEST_CASE(ActorName) {
    const TimerEvent event { 42 };

    BOOST_CHECK_EQUAL(event.actor(), 42);
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * StopRequest event
 */

BOOST_AUTO_TEST_SUITE(StopRequest)

BOOST_AUTO_TEST_CASE(ActorNameAndSignal0) {
    const StopEvent event { 42, 0 };

    BOOST_CHECK_EQUAL(event.actor(), 42);
    BOOST_CHECK_EQUAL(event.caughtSignal(), 0); // Check for minimal value
}

BOOST_AUTO_TEST_CASE(ActorNameAndSignal255) {
    const StopEvent event { 42, 255 };

    BOOST_CHECK_EQUAL(event.actor(), 42);
    BOOST_CHECK_EQUAL(event.caughtSignal(), 255); // Check for maximal value
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * PlayerJoined event
 */

BOOST_AUTO_TEST_SUITE(PlayerJoined)

BOOST_AUTO_TEST_CASE(ActorName) {
    const JoinedEvent event { 42, "NewActor" };

    BOOST_CHECK_EQUAL(event.actor(), 42);
    BOOST_CHECK_EQUAL(event.playerName(), "NewActor");
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * PlayerLeft event
 */

BOOST_AUTO_TEST_SUITE(PlayerLeft)

BOOST_AUTO_TEST_CASE(ActorNameAndCleanDisconnection) {
    const LeftEvent event { 42, LeftEvent::Reason::Clean };

    BOOST_CHECK_EQUAL(event.actor(), 42);
    BOOST_CHECK_EQUAL(event.disconnectionReason(), LeftEvent::Reason::Clean);
}

BOOST_AUTO_TEST_CASE(ActorNameAndCrashDisconnectionWithErrorMsg) {
    const LeftEvent event { 42, LeftEvent::Reason::Crash };

    BOOST_CHECK_EQUAL(event.actor(), 42);
    BOOST_CHECK_EQUAL(event.disconnectionReason(), LeftEvent::Reason::Crash);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
