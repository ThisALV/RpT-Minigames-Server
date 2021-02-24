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
    const NoneEvent event { "Actor" };

    BOOST_CHECK_EQUAL(event.actor(), "Actor");
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * ServiceRequest event
 */

BOOST_AUTO_TEST_SUITE(ServiceRequest)

BOOST_AUTO_TEST_CASE(ActorNameAndInvalidRequest) {
    const ServiceRequestEvent event { "Actor", "A random string" };

    BOOST_CHECK_EQUAL(event.actor(), "Actor");
    // InputEvent shouldn't care about request validity
    BOOST_CHECK_EQUAL(event.serviceRequest(), "A random string");
}

BOOST_AUTO_TEST_CASE(ActorNameAndValidRequest) {
    const ServiceRequestEvent event { "Actor", "REQUEST Service command" };

    BOOST_CHECK_EQUAL(event.actor(), "Actor");
    BOOST_CHECK_EQUAL(event.serviceRequest(), "REQUEST Service command");
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * TimerTrigger event
 */

BOOST_AUTO_TEST_SUITE(TimerTrigger)

BOOST_AUTO_TEST_CASE(ActorName) {
    const TimerEvent event { "Actor" };

    BOOST_CHECK_EQUAL(event.actor(), "Actor");
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * StopRequest event
 */

BOOST_AUTO_TEST_SUITE(StopRequest)

BOOST_AUTO_TEST_CASE(ActorNameAndSignal0) {
    const StopEvent event { "Actor", 0 };

    BOOST_CHECK_EQUAL(event.actor(), "Actor");
    BOOST_CHECK_EQUAL(event.caughtSignal(), 0); // Check for minimal value
}

BOOST_AUTO_TEST_CASE(ActorNameAndSignal255) {
    const StopEvent event { "Actor", 255 };

    BOOST_CHECK_EQUAL(event.actor(), "Actor");
    BOOST_CHECK_EQUAL(event.caughtSignal(), 255); // Check for maximal value
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * PlayerJoined event
 */

BOOST_AUTO_TEST_SUITE(PlayerJoined)

BOOST_AUTO_TEST_CASE(ActorName) {
    const JoinedEvent event { "Actor" };

    BOOST_CHECK_EQUAL(event.actor(), "Actor");
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * PlayerLeft event
 */

BOOST_AUTO_TEST_SUITE(PlayerLeft)

BOOST_AUTO_TEST_CASE(ActorNameAndCleanDisconnection) {
    const LeftEvent event { "Actor", LeftEvent::Reason::Clean };

    BOOST_CHECK_EQUAL(event.actor(), "Actor");
    BOOST_CHECK_EQUAL(event.disconnectionReason(), LeftEvent::Reason::Clean);
    BOOST_CHECK_THROW(event.errorMessage(), NotAnErrorReason); // As DC was clean, there shouldn't be any error msg
}

BOOST_AUTO_TEST_CASE(ActorNameAndCrashDisconnectionWithErrorMsg) {
    const LeftEvent event { "Actor", LeftEvent::Reason::Crash, "A random error" };

    BOOST_CHECK_EQUAL(event.actor(), "Actor");
    BOOST_CHECK_EQUAL(event.disconnectionReason(), LeftEvent::Reason::Crash);
    BOOST_CHECK_EQUAL(event.errorMessage(), "A random error"); // There should be an error msg with this value
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
