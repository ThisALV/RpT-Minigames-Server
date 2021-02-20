#define BOOST_TEST_MODULE Core
#include <boost/test/unit_test.hpp>

#include <cassert>
#include <RpT-Core/InputEvent.hpp>


using namespace RpT::Core;


namespace RpT::Core {

// Required for Boost Tests to print InputEvent::Type values, must be in RpT::Core to be located by C++ ADL
std::ostream& operator<<(std::ostream& out, const InputEvent::Type event_type) {
    switch (event_type) {
    case InputEvent::Type::None:
        return out << "None";
    case InputEvent::Type::ServiceRequest:
        return out << "ServiceRequest";
    case InputEvent::Type::TimerTrigger:
        return out << "TimerTrigger";
    case InputEvent::Type::StopRequest:
        return out << "StopRequest";
    case InputEvent::Type::PlayerJoined:
        return out << "PlayerJoined";
    case InputEvent::Type::PlayerLeft:
        return out << "PlayerLeft";
    }
}

}


BOOST_AUTO_TEST_SUITE(InputEventTests)

// Use anonymous namespace to prevent redefinition of util function
namespace {

// Necessary because Boost Test cannot natively print std::optional
// Also, it would be difficult to define a custom operator<<, as optional belongs to namespace "std"
template<typename T>
void boostCheckOptionalsEqual(std::optional<T>&& lhs, std::optional<T>&& rhs) {
    const bool l_has_value { lhs.has_value() };
    const bool r_has_value { rhs.has_value() };

    // Both optional must have the same state to be equals
    BOOST_CHECK_EQUAL(l_has_value, r_has_value);

    // And if they both have value...
    if (l_has_value && r_has_value) {
        // ...these values must be equals.
        BOOST_CHECK_EQUAL(*lhs, *rhs);
    }
}

}

/*
 * None event
 */

BOOST_AUTO_TEST_SUITE(None)

BOOST_AUTO_TEST_CASE(ActorName) {
    const NoneEvent event { "Actor" };

    BOOST_CHECK_EQUAL(event.type(), InputEvent::Type::None);
    BOOST_CHECK_EQUAL(event.actor(), "Actor");
    boostCheckOptionalsEqual(event.additionalData(), std::optional<std::string> {});
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * ServiceRequest event
 */

BOOST_AUTO_TEST_SUITE(ServiceRequest)

BOOST_AUTO_TEST_CASE(ActorNameAndEmptyRequest) {
    const ServiceRequestEvent event { "Actor", "" };

    BOOST_CHECK_EQUAL(event.type(), InputEvent::Type::ServiceRequest);
    BOOST_CHECK_EQUAL(event.actor(), "Actor");
    boostCheckOptionalsEqual(event.additionalData(), std::optional<std::string> { "" });
}

BOOST_AUTO_TEST_CASE(ActorNameAndRequest) {
    const ServiceRequestEvent event { "Actor", "this is a random request" };

    BOOST_CHECK_EQUAL(event.type(), InputEvent::Type::ServiceRequest);
    BOOST_CHECK_EQUAL(event.actor(), "Actor");
    boostCheckOptionalsEqual(event.additionalData(), std::optional<std::string> { "this is a random request" });
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * TimerTrigger event
 */

BOOST_AUTO_TEST_SUITE(TimerTrigger)

BOOST_AUTO_TEST_CASE(ActorName) {
    const TimerEvent event { "Actor" };

    BOOST_CHECK_EQUAL(event.type(), InputEvent::Type::TimerTrigger);
    BOOST_CHECK_EQUAL(event.actor(), "Actor");
    boostCheckOptionalsEqual(event.additionalData(), std::optional<std::string> {});
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * StopRequest event
 */

BOOST_AUTO_TEST_SUITE(StopRequest)

BOOST_AUTO_TEST_CASE(ActorNameAndSignal0) {
    const StopEvent event { "Actor", 0 };

    BOOST_CHECK_EQUAL(event.type(), InputEvent::Type::StopRequest);
    BOOST_CHECK_EQUAL(event.actor(), "Actor");
    boostCheckOptionalsEqual(event.additionalData(), std::optional<std::string> { "0" });
}

BOOST_AUTO_TEST_CASE(ActorNameAndSignal255) {
    const StopEvent event { "Actor", 255 };

    BOOST_CHECK_EQUAL(event.type(), InputEvent::Type::StopRequest);
    BOOST_CHECK_EQUAL(event.actor(), "Actor");
    boostCheckOptionalsEqual(event.additionalData(), std::optional<std::string> { "255" });
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * PlayerJoined event
 */

BOOST_AUTO_TEST_SUITE(PlayerJoined)

BOOST_AUTO_TEST_CASE(ActorName) {
    const JoinedEvent event { "Actor" };

    BOOST_CHECK_EQUAL(event.type(), InputEvent::Type::PlayerJoined);
    BOOST_CHECK_EQUAL(event.actor(), "Actor");
    boostCheckOptionalsEqual(event.additionalData(), std::optional<std::string> {});
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * PlayerLeft event
 */

BOOST_AUTO_TEST_SUITE(PlayerLeft)

BOOST_AUTO_TEST_CASE(ActorNameAndCleanDisconnection) {
    const LeftEvent event { "Actor", LeftEvent::Reason::Clean };

    BOOST_CHECK_EQUAL(event.type(), InputEvent::Type::PlayerLeft);
    BOOST_CHECK_EQUAL(event.actor(), "Actor");
    boostCheckOptionalsEqual(event.additionalData(), std::optional<std::string> { "Clean" });
}

BOOST_AUTO_TEST_CASE(ActorNameAndCrashDisconnectionWithErrorMsg) {
    const LeftEvent event { "Actor", LeftEvent::Reason::Crash, "a random error" };

    BOOST_CHECK_EQUAL(event.type(), InputEvent::Type::PlayerLeft);
    BOOST_CHECK_EQUAL(event.actor(), "Actor");
    boostCheckOptionalsEqual(event.additionalData(), std::optional<std::string> { "Crash;a random error" });
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
