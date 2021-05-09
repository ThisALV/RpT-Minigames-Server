#include <RpT-Testing/TestingUtils.hpp>

#include <RpT-Network/MessagesQueueView.hpp>


using namespace RpT::Network;


// Facility function, anonymous namespace to avoid name clashes
namespace {


std::shared_ptr<std::string> rptlMessage(std::string message) {
    return std::make_shared<std::string>(std::move(message)); // Avoid useless copy for smart pointer initialization
}


}


BOOST_AUTO_TEST_SUITE(MessagesQueueViewTests)


BOOST_AUTO_TEST_CASE(EmptyQueue) {
    std::queue<std::shared_ptr<std::string>> messages_queue;
    MessagesQueueView view { messages_queue };

    // Messages queue is empty
    BOOST_CHECK(!view.hasNext());
    BOOST_CHECK_THROW(view.next(), NoMoreMessage);
}

BOOST_AUTO_TEST_CASE(ManyRptlMessagesAfterCtor) {
    std::queue<std::shared_ptr<std::string>> messages_queue;
    MessagesQueueView view { messages_queue };

    const auto firstMessage { rptlMessage("A") };
    const auto secondMessage { rptlMessage("B") };

    messages_queue.push(firstMessage);
    messages_queue.push(secondMessage);

    // Messages queue is not empty AFTER view construction
    BOOST_CHECK(view.hasNext());
    BOOST_CHECK_EQUAL(view.next(), firstMessage); // Ptr identity check
    BOOST_CHECK_EQUAL(view.next(), secondMessage);
    BOOST_CHECK_THROW(view.next(), NoMoreMessage); // There were only 2 messages inside queue
}

BOOST_AUTO_TEST_CASE(ManyRptlMessagesBeforeCtor) {
    const auto firstMessage { rptlMessage("A") };
    const auto secondMessage { rptlMessage("B") };

    std::queue<std::shared_ptr<std::string>> messages_queue { // Constructed with non-empty underlying list (<=> deque)
        std::deque<std::shared_ptr<std::string>> { firstMessage, secondMessage }
    };
    MessagesQueueView view { messages_queue };

    // Messages queue is not empty BEFORE and AFTER view construction
    BOOST_CHECK(view.hasNext());
    BOOST_CHECK_EQUAL(view.next(), firstMessage); // Ptr identity check
    BOOST_CHECK_EQUAL(view.next(), secondMessage);
    BOOST_CHECK_THROW(view.next(), NoMoreMessage); // There were only 2 messages inside queue
}


BOOST_AUTO_TEST_SUITE_END()