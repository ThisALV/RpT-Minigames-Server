#include <RpT-Testing/TestingUtils.hpp>

#include <optional>
#include <RpT-Core/ServiceEventRequestProtocol.hpp>


using namespace RpT::Core;


/*
 * Minimal services for unit testing purpose
 *
 * They set `lastCommandActor()` return value to given request command actor and then returns successfully if non-empty
 * command is handled or "Empty" error message otherwise. This will allows to check if handler was successfully ran.
 *
 * When command is handled, an event is emitted, command is the actor name.
 */

class MinimalService : public Service {
private:
    std::optional<std::uint64_t> last_command_actor_;

public:
    explicit MinimalService(ServiceContext& run_context) : Service { run_context } {}

    std::uint64_t lastCommandActor() const {
        assert(last_command_actor_.has_value()); // Field must be initialized (at least one call to handleRequestCommand())
        return *last_command_actor_;
    }

    RpT::Utils::HandlingResult handleRequestCommand(const std::uint64_t actor,
                                                    const std::string_view sr_command_data) override {
        emitEvent(std::to_string(actor));
        last_command_actor_ = actor;

        return !sr_command_data.empty()
                ? RpT::Utils::HandlingResult {}
                : RpT::Utils::HandlingResult { "Empty" };
    }
};

class ServiceA : public MinimalService {
public:
    explicit ServiceA(ServiceContext& run_context) : MinimalService { run_context } {}

    std::string_view name() const override {
        return "ServiceA";
    }
};

class ServiceB : public MinimalService {
public:
    explicit ServiceB(ServiceContext& run_context) : MinimalService { run_context } {}

    std::string_view name() const override {
        return "ServiceB";
    }
};

class ServiceC : public MinimalService {
public:
    explicit ServiceC(ServiceContext& run_context) : MinimalService { run_context } {}

    std::string_view name() const override {
        return "ServiceC";
    }
};


/**
 * @brief Provides `LoggingContext` with disabled logging
 */
class DisabledLoggingFixture {
public:
    RpT::Utils::LoggingContext logging_context;

    DisabledLoggingFixture() {
        logging_context.disable();
    }
};


/**
 * @brief Provides 3 basic services implementations for testing purpose, and extends `DisabledLoggingFixture`
 *
 * Default state: Just initialized `ServiceContext`, used to construct `ServiceA`, `ServiceB` and `ServiceC`
 */
class MinimalServiceImplementationsFixture : public DisabledLoggingFixture {
public:
    ServiceContext context;

    ServiceA svc_a;
    ServiceB svc_b;
    ServiceC svc_c;

    MinimalServiceImplementationsFixture() :
            DisabledLoggingFixture {},
            context {}, svc_a { context }, svc_b { context }, svc_c { context } {}
};


/**
 * @brief Extends `SerProtocolTests` global fixture adding SER Protocol instance with `svc_a`, `svc_b` and `svc_c`
 * registered
 *
 * SER Protocol logging is disabled by context
 */
class SerProtocolWithMinimalServiceFixture :
        public MinimalServiceImplementationsFixture {

public:
    ServiceEventRequestProtocol ser_protocol;

    SerProtocolWithMinimalServiceFixture() :
            MinimalServiceImplementationsFixture {},
            ser_protocol { { svc_a, svc_b, svc_c }, logging_context } {}
};


BOOST_FIXTURE_TEST_SUITE(SerProtocolTests, MinimalServiceImplementationsFixture)

/*
 * Constructor
 */

BOOST_AUTO_TEST_SUITE(Constructor)

BOOST_AUTO_TEST_CASE(NoServices) {
    const ServiceEventRequestProtocol ser_protocol { {}, logging_context };

    // Not any service should be registered
    BOOST_CHECK(!ser_protocol.isRegistered("NonexistentService"));
}

BOOST_AUTO_TEST_CASE(SomeServices) {
    const ServiceEventRequestProtocol ser_protocol { { svc_a, svc_b, svc_c }, logging_context };

    // Checks if service is registered
    BOOST_CHECK(ser_protocol.isRegistered("ServiceA"));
    BOOST_CHECK(ser_protocol.isRegistered("ServiceB"));
    BOOST_CHECK(ser_protocol.isRegistered("ServiceC"));
    // Check if unknown service is unregistered
    BOOST_CHECK(!ser_protocol.isRegistered("NonexistentService"));
}

BOOST_AUTO_TEST_CASE(SomeServiceAndTwiceSameName) {
    ServiceA svc_a_bis { context };

    // Checks if exception is thrown as svc_a_bis and svc_a both have name "ServiceA"
    BOOST_CHECK_THROW((ServiceEventRequestProtocol { { svc_a, svc_b, svc_c, svc_a_bis }, logging_context }),
                      ServiceNameAlreadyRegistered);
}

BOOST_AUTO_TEST_CASE(SomeServiceAndTwiceSameInstance) {
    // Checks if exception is thrown as svc_a is registered twice under "ServiceA" name
    BOOST_CHECK_THROW((ServiceEventRequestProtocol { { svc_a, svc_b, svc_c, svc_a }, logging_context }),
                      ServiceNameAlreadyRegistered);
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * handleServiceRequest()
 */

BOOST_FIXTURE_TEST_SUITE(HandleServiceRequest, SerProtocolWithMinimalServiceFixture)

BOOST_AUTO_TEST_CASE(EmptyServiceRequest) {
    // An empty request is ill formed
    BOOST_CHECK_THROW(ser_protocol.handleServiceRequest(0, ""), InvalidRequestFormat);
}

BOOST_AUTO_TEST_CASE(OneWordServiceRequest) {
    // A request must contains at least two words
    BOOST_CHECK_THROW(ser_protocol.handleServiceRequest(0, "RANDOM_WORD"), InvalidRequestFormat);
}

BOOST_AUTO_TEST_CASE(BadPrefixAndServiceName) {
    // Service Request must begins with "REQUEST" prefix
    BOOST_CHECK_THROW(ser_protocol.handleServiceRequest(0, "BAD_PREFIX ServiceA"), InvalidRequestFormat);
}

BOOST_AUTO_TEST_CASE(RightPrefixAndUnknownServiceName) {
    // Service must be registered
    BOOST_CHECK_THROW(ser_protocol.handleServiceRequest(0, "REQUEST 2 NonexistentService"), ServiceNotFound);
}

BOOST_AUTO_TEST_CASE(RightPrefixServiceBEmptyCommand) {
    // SR command is well formed but empty, so handling should return KO response with "Empty" error message
    BOOST_CHECK_EQUAL(ser_protocol.handleServiceRequest(1, "REQUEST 1 ServiceB"), "RESPONSE 1 KO Empty");
    // But service last command actor property should have been updated anyway
    BOOST_CHECK_EQUAL(svc_b.lastCommandActor(), 1);
}

BOOST_AUTO_TEST_CASE(RightPrefixServiceBNonemptyCommand) {
    // SR command is well formed and contains arguments, so handling should be done successfully
    BOOST_CHECK_EQUAL(ser_protocol.handleServiceRequest(1, "REQUEST 0 ServiceB Some random arguments"),
                      "RESPONSE 0 OK");
    // And last command actor should have been updated
    BOOST_CHECK_EQUAL(svc_b.lastCommandActor(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * pollServiceEvent() unit tests
 */

BOOST_FIXTURE_TEST_SUITE(PollServiceEvent, SerProtocolWithMinimalServiceFixture)

BOOST_AUTO_TEST_CASE(NoEvents) {
    // All events queues should be empty as no request command were handled
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(), std::optional<std::string> {});
}

BOOST_AUTO_TEST_CASE(ManyEventsInSomeQueues) {
    // Emits many events
    svc_a.handleRequestCommand(1, {});
    svc_b.handleRequestCommand(2, {});
    svc_a.handleRequestCommand(3, {});
    svc_b.handleRequestCommand(4, {});

    // Checks for events polling order, FIFO queue, so should be the same than insertion (or events emission) order
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceA 1" });
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceB 2" });
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceA 3" });
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceB 4" });
}

BOOST_AUTO_TEST_CASE(ManyEventsInEveryQueues) {
    // Emits many events in a random miscellaneous order
    svc_a.handleRequestCommand(1, "");
    svc_a.handleRequestCommand(2, "");
    svc_c.handleRequestCommand(3, "");
    svc_b.handleRequestCommand(4, "");
    svc_b.handleRequestCommand(5, "");
    svc_b.handleRequestCommand(6, "");
    svc_c.handleRequestCommand(7, "");

    // Checks for events polling order, FIFO queue, so should be the same than insertion (or events emission) order
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceA 1" });
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceA 2" });
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceC 3" });
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceB 4" });
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceB 5" });
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceB 6" });
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceC 7" });
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
