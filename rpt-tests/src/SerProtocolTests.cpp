#include <RpT-Testing/TestingUtils.hpp>

#include <RpT-Core/ServiceEventRequestProtocol.hpp>


using namespace RpT::Core;


/*
 * Minimal services for unit testing purpose
 *
 * They set `lastCommandActor()` return value to given request command actor and then return `true` if non-empty
 * command is handled or `false` otherwise. This will allows to check if handler was successfully ran.
 *
 * When command is handled, an event is emitted, command is the actor name.
 */

class MinimalService : public Service {
private:
    std::string_view last_command_actor_;

public:
    explicit MinimalService(ServiceContext& run_context) : Service { run_context } {}

    std::string_view lastCommandActor() const {
        assert(last_command_actor_.data()); // Field must be initialized (at least one call to handleRequestCommand())
        return last_command_actor_;
    }

    bool handleRequestCommand(const std::string_view actor,
                              const std::vector<std::string_view>& sr_command_arguments) override {
        emitEvent(std::string { actor });
        last_command_actor_ = actor;

        return !sr_command_arguments.empty();
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
    BOOST_CHECK_THROW(ser_protocol.handleServiceRequest("", ""), BadServiceRequest);
}

BOOST_AUTO_TEST_CASE(OneWordServiceRequest) {
    // A request must contains at least two words
    BOOST_CHECK_THROW(ser_protocol.handleServiceRequest("", "RANDOM_WORD"), BadServiceRequest);
}

BOOST_AUTO_TEST_CASE(BadPrefixAndServiceName) {
    // Service Request must begins with "REQUEST" prefix
    BOOST_CHECK_THROW(ser_protocol.handleServiceRequest("", "BAD_PREFIX ServiceA"), BadServiceRequest);
}

BOOST_AUTO_TEST_CASE(RightPrefixAndUnknownServiceName) {
    // Service must be registered
    BOOST_CHECK_THROW(ser_protocol.handleServiceRequest("", "REQUEST NonexistentService"), ServiceNotFound);
}

BOOST_AUTO_TEST_CASE(RightPrefixServiceBEmptyCommand) {
    // SR command is well formed but empty, so handling should return false
    BOOST_CHECK(!ser_protocol.handleServiceRequest("Actor1", "REQUEST ServiceB"));
    // But service last command actor property should have been updated anyway
    BOOST_CHECK_EQUAL(svc_b.lastCommandActor(), "Actor1");
}

BOOST_AUTO_TEST_CASE(RightPrefixServiceBNonemptyCommand) {
    // SR command is well formed and contains arguments, so handling should be done successfully
    BOOST_CHECK(ser_protocol.handleServiceRequest("Actor1", "REQUEST ServiceB Some random arguments"));
    // And last command actor should have been updated
    BOOST_CHECK_EQUAL(svc_b.lastCommandActor(), "Actor1");
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
    svc_a.handleRequestCommand("Actor 1", {});
    svc_b.handleRequestCommand("Actor 2", {});
    svc_a.handleRequestCommand("Actor 3", {});
    svc_b.handleRequestCommand("Actor 4", {});

    // Checks for events polling order, FIFO queue, so should be the same than insertion (or events emission) order
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceA Actor 1" });
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceB Actor 2" });
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceA Actor 3" });
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceB Actor 4" });
}

BOOST_AUTO_TEST_CASE(ManyEventsInEveryQueues) {
    // Emits many events
    svc_a.handleRequestCommand("Actor 1", {});
    svc_b.handleRequestCommand("Actor 2", {});
    svc_c.handleRequestCommand("Actor 3", {});
    svc_a.handleRequestCommand("Actor 4", {});
    svc_b.handleRequestCommand("Actor 5", {});
    svc_c.handleRequestCommand("Actor 6", {});

    // Checks for events polling order, FIFO queue, so should be the same than insertion (or events emission) order
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceA Actor 1" });
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceB Actor 2" });
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceC Actor 3" });
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceA Actor 4" });
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceB Actor 5" });
    RpT::Testing::boostCheckOptionalsEqual(ser_protocol.pollServiceEvent(),
                                           std::optional<std::string> { "EVENT ServiceC Actor 6" });
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
