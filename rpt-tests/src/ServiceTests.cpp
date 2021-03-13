#include <RpT-Testing/TestingUtils.hpp>

#include <RpT-Core/Service.hpp>


using namespace RpT::Core;


BOOST_AUTO_TEST_SUITE(ServiceContextTests)

BOOST_AUTO_TEST_CASE(DefaultConstructed) {
    ServiceContext context;

    BOOST_CHECK_EQUAL(context.newEventPushed(), 0);
    BOOST_CHECK_EQUAL(context.newEventPushed(), 1);
    BOOST_CHECK_EQUAL(context.newEventPushed(), 2);
}

BOOST_AUTO_TEST_SUITE_END()


/**
 * @brief Basic implementation to build Service class and test its defined methods
 *
 * `name()` returns empty string and `handleRequestCommand()` fires event with actor as event command, then returns
 * successfully.
 */
class TestingService : public Service {
public:
    explicit TestingService(ServiceContext& run_context) : Service { run_context } {}

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
 */
class ServiceTestFixture {
private:
    ServiceContext context_;

public:
    TestingService service;

    ServiceTestFixture() : context_ {}, service { context_ } {}
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

BOOST_AUTO_TEST_SUITE_END()
