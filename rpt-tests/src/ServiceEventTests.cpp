#include <RpT-Testing/TestingUtils.hpp>

#include <RpT-Core/ServiceEvent.hpp>


using namespace RpT::Core;

using OptionalUidsSet = std::optional<std::unordered_set<std::uint64_t>>; // Shortcut for actor UIDs set instanciation


BOOST_TEST_DONT_PRINT_LOG_VALUE(std::unordered_set<std::uint64_t>); // Unable to implement operator<< for an std object

BOOST_AUTO_TEST_SUITE(ServiceEventTests)


/*
 * operator== unit tests
 */
BOOST_AUTO_TEST_SUITE(EqualityOperator)


BOOST_AUTO_TEST_CASE(SameCommandAndBothNoActorsList) {
    BOOST_CHECK(ServiceEvent { "A" } == ServiceEvent { "A" });
}

BOOST_AUTO_TEST_CASE(SameCommandAndSameActorsList) {
    BOOST_CHECK((ServiceEvent { "A", OptionalUidsSet { { 1, 2, 3 } } }
        == ServiceEvent { "A", OptionalUidsSet  { { 3, 2, 1 } } }));
}

BOOST_AUTO_TEST_CASE(SameCommandAndOnlyOneActorsList) {
    BOOST_CHECK((ServiceEvent { "A" } != ServiceEvent { "A", OptionalUidsSet { { 1, 2, 3 } } }));
}

BOOST_AUTO_TEST_CASE(SameCommandAndDifferentActorsList) {
    BOOST_CHECK((ServiceEvent { "A", OptionalUidsSet { { 1, 2 } } }
        != ServiceEvent { "A", OptionalUidsSet  { { 1 } } }));
}

BOOST_AUTO_TEST_CASE(DifferentCommandAndBothNoActorsList) {
    BOOST_CHECK(ServiceEvent { "A" } != ServiceEvent { "B" });
}

BOOST_AUTO_TEST_CASE(DifferentCommandAndSameActorsList) {
    BOOST_CHECK((ServiceEvent { "A", OptionalUidsSet { { 1, 2, 3 } } }
        != ServiceEvent { "B", OptionalUidsSet  { { 3, 2, 1 } } }));
}

BOOST_AUTO_TEST_CASE(DifferentCommandAndOnlyOneActorsList) {
    BOOST_CHECK((ServiceEvent { "A" } != ServiceEvent { "B", OptionalUidsSet { { 1, 2, 3 } } }));
}

BOOST_AUTO_TEST_CASE(DifferentCommandAndDifferentActorsList) {
    BOOST_CHECK((ServiceEvent { "A", OptionalUidsSet { { 1, 2 } } }
        != ServiceEvent { "B", OptionalUidsSet  { { 1 } } }));
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * prefixWith() unit tests
 */
BOOST_AUTO_TEST_SUITE(PrefixWith)


BOOST_AUTO_TEST_CASE(AnyUsage) {
    // Constructs a basic event Hello world! received by actors with UID which is 4, 5 or 6
    const ServiceEvent initial_event { "Hello world!", OptionalUidsSet { { 6, 5, 4 } } };
    // Prefixes event with a SERVICE EVENT sample command
    const ServiceEvent prefixed_event { initial_event.prefixWith("SERVICE EVENT ") };

    // Command event data should have been prefixed
    BOOST_CHECK_EQUAL(prefixed_event.command(), "SERVICE EVENT Hello world!");
    // Actor UIDs set should not have been modified
    BOOST_CHECK_EQUAL(prefixed_event.targets(), (std::unordered_set<std::uint64_t> { 6, 4, 5 }));
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * targetEveryone() unit tests
 */
BOOST_AUTO_TEST_SUITE(TargetEveryone)


BOOST_AUTO_TEST_CASE(TargetingEveryone) {
    BOOST_CHECK((ServiceEvent { "" }).targetEveryone()); // No UIDs set provided, targets everyone
}

BOOST_AUTO_TEST_CASE(TargetingNobody) {
    // Empty UIDs set provided, but still provided so doesn't target everyone
    BOOST_CHECK(!(ServiceEvent { "", OptionalUidsSet { std::initializer_list<std::uint64_t> {} } }).targetEveryone());
}

BOOST_AUTO_TEST_CASE(TargetingSpecifiedActors) {
    // UIDs set provided, doesn't target everyone
    BOOST_CHECK(!(ServiceEvent { "", OptionalUidsSet { { 3, 1, 2 } } }).targetEveryone());
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * targets() unit tests
 */
BOOST_AUTO_TEST_SUITE(Targets)


BOOST_AUTO_TEST_CASE(TargetingEveryone) {
    BOOST_CHECK_THROW((ServiceEvent { "" }).targets(), NoUidsList); // No UIDs to retrieve
}

BOOST_AUTO_TEST_CASE(TargetingSpecifiedActors) {
    // UIDs should be retrieved, no matter the order as a UIDs set is used
    BOOST_CHECK_EQUAL((ServiceEvent { "", OptionalUidsSet { { 5, 0, 2 } } }).targets(),
                      (std::unordered_set<std::uint64_t> { 2, 5, 0 }));
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
