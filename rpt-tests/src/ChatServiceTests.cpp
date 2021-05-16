#include <RpT-Testing/TestingUtils.hpp>
#include <Minigames-Services/ChatService.hpp>


using namespace MinigamesServices;


BOOST_AUTO_TEST_SUITE(ChatServiceTests)


/**
 * @brief Initializes a ChatService into a new context with a message cooldown of 2000 ms
 */
class ChatServiceFixture {
public:
    /// Actor used to test SR command handling
    static constexpr std::uint64_t CONSOLE_ACTOR { 0 };

    RpT::Core::ServiceContext context;
    ChatService service;

    ChatServiceFixture() : service { context, 2000 } {}
};


/*
 * trim() free function test
 */
BOOST_AUTO_TEST_SUITE(Trim)


BOOST_AUTO_TEST_CASE(ZeroLengthMessage) {
    BOOST_CHECK_EQUAL(trim(""), "");
}

BOOST_AUTO_TEST_CASE(OnlyWhitespaces) {
    BOOST_CHECK_EQUAL(trim("\n \t  \n\t "), "");
}

BOOST_AUTO_TEST_CASE(WhitespacesPrefix) {
    BOOST_CHECK_EQUAL(trim(" \n\t  Abcd"), "Abcd");
}

BOOST_AUTO_TEST_CASE(WhitespacesSuffix) {
    BOOST_CHECK_EQUAL(trim("Abcd \n\t"), "Abcd");
}

BOOST_AUTO_TEST_CASE(WhitespacesPrefixAndSuffix) {
    BOOST_CHECK_EQUAL(trim(" \n\n\t Abcd\n   \t"), "Abcd");
}

BOOST_AUTO_TEST_CASE(WhitespacesPrefixAndSuffixAndInside) {
    BOOST_CHECK_EQUAL(trim(" \n\n\t Ab\t \ncd\n   \t"), "Ab\t \ncd");
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * ChatService() constructor unit tests
 */
BOOST_FIXTURE_TEST_SUITE(Constructor, ChatServiceFixture)


BOOST_AUTO_TEST_CASE(NewContextAnd2sCooldown) {
    BOOST_CHECK_EQUAL(service.name(), "Chat");
    BOOST_CHECK(!service.checkEvent().has_value()); // No events emitted at initialization
    BOOST_CHECK(service.getWaitingTimers().empty()); // Cooldown hasn't been started since no message was sent

    BOOST_CHECK_EQUAL(context.newTimerCreated(), 1); // A timer with token 0 should have already been created
}


BOOST_AUTO_TEST_SUITE_END()


/*
 * handleRequestCommand() method unit tests
 */
BOOST_FIXTURE_TEST_SUITE(HandleRequestCommand, ChatServiceFixture)


BOOST_AUTO_TEST_CASE(WhitespacesOnlyMessage) {
    const RpT::Utils::HandlingResult was_sent {
        service.handleRequestCommand(CONSOLE_ACTOR, "\t\t\n   \n")
    };

    // Checks for message sending to have fail
    BOOST_CHECK(!was_sent);
    BOOST_CHECK_EQUAL(was_sent.errorMessage(), "Message cannot be empty");
    BOOST_CHECK(!service.checkEvent().has_value());
    // Checks for cooldown to not be started at message wasn't sent
    BOOST_CHECK(service.getWaitingTimers().empty());
}

BOOST_AUTO_TEST_CASE(RawNonTrimmedMessage) {
    const RpT::Utils::HandlingResult was_sent {
            service.handleRequestCommand(CONSOLE_ACTOR, "\t\t\n Hello world!  \n")
    };

    // Checks for message sending to be done
    BOOST_CHECK(was_sent);
    BOOST_CHECK_EQUAL(service.pollEvent(), "MESSAGE_FROM 0 Hello world!"); // Trimmed message should have been sent

    // Checks for cooldown to have been started has message is sent
    const std::vector<std::reference_wrapper<RpT::Core::Timer>> waiting_timers { service.getWaitingTimers() };
    BOOST_CHECK_EQUAL(waiting_timers.size(), 1); // One timer, the cooldown, should have been started
    BOOST_CHECK_EQUAL(waiting_timers.at(0).get().token(), 0); // This timer should be the cooldown timer
}

BOOST_AUTO_TEST_CASE(NormalMessageWithCooldownNotFree) {
    service.handleRequestCommand(CONSOLE_ACTOR, "Hello world!"); // Sends a first message, cooldown starts

    const RpT::Utils::HandlingResult second_was_sent { // Even if it is another actor, cooldown block message sending
        service.handleRequestCommand(1, "Second message")
    };

    // Checks for second message to not have been sent
    BOOST_CHECK(!second_was_sent);
    BOOST_CHECK_EQUAL(second_was_sent.errorMessage(), "Last message when sent less than 2500 ms ago");
    // Only first message should have been sent
    BOOST_CHECK_EQUAL(service.pollEvent(), "MESSAGE_FROM 0 Hello world!");
    BOOST_CHECK(!service.checkEvent().has_value()); // No second message
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
