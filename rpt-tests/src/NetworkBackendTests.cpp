#include <RpT-Testing/TestingUtils.hpp>

#include <optional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <RpT-Network/NetworkBackend.hpp>


using namespace RpT::Network;


/// Retrieves index for given input event type inside `RpT::Core::AnyInputEvent` variant types, or -1 if not listed
template<typename InputType>
constexpr int inputIndexFor() {
    if constexpr (std::is_same_v<InputType, RpT::Core::NoneEvent>)
        return 0;
    else if constexpr (std::is_same_v<InputType, RpT::Core::StopEvent>)
        return 1;
    else if constexpr (std::is_same_v<InputType, RpT::Core::ServiceRequestEvent>)
        return 2;
    else if constexpr (std::is_same_v<InputType, RpT::Core::TimerEvent>)
        return 3;
    else if constexpr (std::is_same_v<InputType, RpT::Core::JoinedEvent>)
        return 4;
    else if constexpr (std::is_same_v<InputType, RpT::Core::LeftEvent>)
        return 5;
    else
        return -1;
}

/// Checks for given event variant to be of correct input event type and returns statically typed event
template<typename ExpectedEventT>
ExpectedEventT requireEventType(RpT::Core::AnyInputEvent event_variant) {
    // Checks for type ID before of given variant
    BOOST_REQUIRE_EQUAL(event_variant.which(), inputIndexFor<ExpectedEventT>());

    // Move constructs typed event from event variant which will no longer be used
    return boost::get<ExpectedEventT>(std::move(event_variant));
}


/*
 * Default client data
 */

static constexpr std::uint64_t CONSOLE_CLIENT { 0 };
static constexpr std::uint64_t CONSOLE_ACTOR { 0 };
static constexpr std::string_view CONSOLE_NAME { "Console" };

/*
 * Test client data
 */

static constexpr std::uint64_t TEST_CLIENT { 1 };
static constexpr std::uint64_t TEST_ACTOR { 1 };


/**
 * @brief Basic `NetworkBackend` implementation for mocking purpose
 */
class SimpleNetworkBackend : public NetworkBackend {
protected:
    /// Retrieves `NoneEvent` triggered by actor with UID == 0 when queue is empty
    RpT::Core::AnyInputEvent waitForEvent() override {
        return RpT::Core::NoneEvent { CONSOLE_ACTOR };
    }

    /*
     * Message sending isn't NetworkBackend responsibility
     */

    void handleServiceRequestResponse(std::uint64_t, std::string) override {
        throw std::logic_error { "Not implemented" };
    }

    void handleServiceEvent(std::string) override {
        throw std::logic_error { "Not implemented" };
    }

public:
    /// Initializes backend with client actor 0 for `waitForEvent()` return value and unregistered client 1 for
    /// testing purpose
    SimpleNetworkBackend() {
        // Default client uses token 0
        addClient(CONSOLE_CLIENT);
        // Registers client 0 as actor 0 named "Console"
        handleMessage(CONSOLE_CLIENT,
                      "LOGIN " + std::to_string(CONSOLE_ACTOR) + ' ' + std::string { CONSOLE_NAME });

        // Test client uses token 1
        addClient(TEST_CLIENT);
    }

    /// Handles given RPTL message and pushes event triggered by command handling
    void clientMessage(const std::uint64_t client_token, const std::string& client_message) {
        pushInputEvent(handleMessage(client_token, client_message));
    }

    /// Push given event directly into queue, trivial access to pushInputEvent() for testing purpose
    void trigger(RpT::Core::AnyInputEvent event) {
        pushInputEvent(std::move(event));
    }

    /// Trivial access to inputReady() for testing purpose
    bool ready() const {
        return inputReady();
    }

    /// Checks if given actor UID is registered or not, trivial access to isRegister() for testing purpose
    bool registered(const std::uint64_t actor_uid) const {
        return isRegistered(actor_uid);
    }

    /// Checks if given client is alive or not, trivial access to isAlive() for testing purpose
    bool alive(const std::uint64_t actor_uid) const {
        return isAlive(actor_uid);
    }

    /// Trivial access to addClient() for testing purpose
    void newClient(const std::uint64_t new_token) {
        addClient(new_token);
    }

    /// Trivial access to removeClient() for testing purpose
    void deleteClient(const std::uint64_t old_token) {
        removeClient(old_token);
    }

    /// Trivial access to killClient() for testing purpose
    void kill(const std::uint64_t client_token, const RpT::Utils::HandlingResult& disconnnection_reason = {}) {
        killClient(client_token, disconnnection_reason);
    }

    /// Trivial access to disconnectionReason() for testing purpose
    const RpT::Utils::HandlingResult& killReason(const std::uint64_t client_token) {
        return disconnectionReason(client_token);
    }
};


BOOST_AUTO_TEST_SUITE(NetworkBackendTests)

/*
 * inputReady() unit tests
 */

BOOST_AUTO_TEST_SUITE(InputReady)

BOOST_AUTO_TEST_CASE(EmptyQueue) {
    SimpleNetworkBackend io_interface;

    // No events pushed into queue, shouldn't be ready
    BOOST_CHECK(!io_interface.ready());
}

BOOST_AUTO_TEST_CASE(AnyEventInsideQueue) {
    SimpleNetworkBackend io_interface;

    // Push any event into queue
    io_interface.trigger(RpT::Core::JoinedEvent { TEST_ACTOR, "NoName" });

    // Should now be ready for next input event
    BOOST_CHECK(io_interface.ready());
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * waitForInput() unit tests
 */

BOOST_AUTO_TEST_SUITE(WaitForInput)

BOOST_AUTO_TEST_CASE(EmptyQueue) {
    SimpleNetworkBackend io_interface;

    // Gets input event with empty queue, expected call to waitForEvent() returning NoneEvent triggered by actor 0
    const auto event { requireEventType<RpT::Core::NoneEvent>(io_interface.waitForInput()) };
    
    BOOST_CHECK_EQUAL(event.actor(), CONSOLE_ACTOR);
}

BOOST_AUTO_TEST_CASE(SingleQueuedEvent) {
    SimpleNetworkBackend io_interface;

    // A new player named NoName with UID 1 joined server, event triggered
    io_interface.trigger(RpT::Core::JoinedEvent { TEST_ACTOR, "NoName" });

    // Gets input event with non-empty queue, first pushed element will be returned
    const auto event { requireEventType<RpT::Core::JoinedEvent>(io_interface.waitForInput()) };

    BOOST_CHECK_EQUAL(event.actor(), TEST_ACTOR);
    BOOST_CHECK_EQUAL(event.playerName(), "NoName");
}

BOOST_AUTO_TEST_CASE(ManyQueuedEvents) {
    SimpleNetworkBackend io_interface;

    // 2 timers TO then and server stopped triggered
    io_interface.trigger(RpT::Core::TimerEvent { 1 }); // Triggered by actor 1
    io_interface.trigger(RpT::Core::TimerEvent { 2 }); // Triggered by actor 2
    io_interface.trigger(RpT::Core::StopEvent { 0, 2 }); // Triggered by actor 0 with signal 2

    // Checks for 3rd event pushed into queue
    const auto first_event { requireEventType<RpT::Core::TimerEvent>(io_interface.waitForInput()) };
    BOOST_CHECK_EQUAL(first_event.actor(), 1);

    // Checks for 2nd event pushed into queue
    const auto second_event { requireEventType<RpT::Core::TimerEvent>(io_interface.waitForInput()) };
    BOOST_CHECK_EQUAL(second_event.actor(), 2);

    // Checks for 3rd event pushed into queue
    const auto third_event { requireEventType<RpT::Core::StopEvent>(io_interface.waitForInput()) };
    BOOST_CHECK_EQUAL(third_event.actor(), 0);
    BOOST_CHECK_EQUAL(third_event.caughtSignal(), 2);
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * addClient() unit tests
 */

BOOST_AUTO_TEST_SUITE(AddClient)

BOOST_AUTO_TEST_CASE(AvailableToken) {
    SimpleNetworkBackend io_interface;

    // NetworkBackend mock should also have added TEST_CLIENT as connected token, just have to test for it jere
    BOOST_CHECK(io_interface.alive(TEST_CLIENT));
    // However, its actor should not have been registered yet
    BOOST_CHECK(!io_interface.registered(TEST_ACTOR));
}

BOOST_AUTO_TEST_CASE(UnavailableToken) {
    SimpleNetworkBackend io_interface;

    // CONSOLE_CLIENT token already used by default client at NetworkBackend mock construction, should throws error
    BOOST_CHECK_THROW(io_interface.newClient(CONSOLE_CLIENT), UnavailableClientToken);
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * killClient() unit tests
 */

BOOST_AUTO_TEST_SUITE(KillClient)

BOOST_AUTO_TEST_CASE(UnknownClient) {
    SimpleNetworkBackend io_interface;

    // No connected client with token 42, error expected
    BOOST_CHECK_THROW(io_interface.kill(42), UnknownClientToken);
}

BOOST_AUTO_TEST_CASE(RegisteredNormal) {
    SimpleNetworkBackend io_interface;

    // CONSOLE_CLIENT is already registered by default
    io_interface.kill(CONSOLE_CLIENT);

    // Its previous actor UID should no longer be registered
    BOOST_CHECK(!io_interface.registered(CONSOLE_ACTOR));
    // Client should no longer be alive
    BOOST_CHECK(!io_interface.alive(CONSOLE_CLIENT));
    // Disconnection should be clean
    BOOST_CHECK(io_interface.killReason(CONSOLE_CLIENT));

    /*
     * As client has a registered actor, a pipeline should have happened with appropriate LeftEvent
     */

    const auto left_event { requireEventType<RpT::Core::LeftEvent>(io_interface.waitForInput()) };
    BOOST_CHECK_EQUAL(left_event.actor(), CONSOLE_ACTOR);
    // Disconnection should be clean
    BOOST_CHECK(left_event.disconnectionReason());
}

BOOST_AUTO_TEST_CASE(RegisteredWithErrorMessage) {
    SimpleNetworkBackend io_interface;

    // CONSOLE_CLIENT is already registered by default
    io_interface.kill(CONSOLE_CLIENT, RpT::Utils::HandlingResult { "Error reason" });

    // Its previous actor UID should no longer be registered
    BOOST_CHECK(!io_interface.registered(CONSOLE_ACTOR));
    // Client should no longer be alive
    BOOST_CHECK(!io_interface.alive(CONSOLE_CLIENT));
    // Disconnection should be crash with an error message
    const RpT::Utils::HandlingResult status_error { io_interface.killReason(CONSOLE_CLIENT) };
    BOOST_CHECK(!status_error);
    BOOST_CHECK_EQUAL(status_error.errorMessage(), "Error reason");

    /*
     * As client has a registered actor, a pipeline should have happened with appropriate LeftEvent
     */

    const auto left_event { requireEventType<RpT::Core::LeftEvent>(io_interface.waitForInput()) };
    const RpT::Utils::HandlingResult disconnection_reason { left_event.disconnectionReason() };
    BOOST_CHECK_EQUAL(left_event.actor(), CONSOLE_ACTOR);
    // Disconnection should be caused by crash
    BOOST_CHECK(!disconnection_reason);
    BOOST_CHECK_EQUAL(disconnection_reason.errorMessage(), "Error reason");
}

BOOST_AUTO_TEST_CASE(UnregisteredNormal) {
    SimpleNetworkBackend io_interface;

    // TEST_CLIENT is already connected by default but not registered
    io_interface.kill(TEST_CLIENT);

    // TEST_CLIENT should no longer be alive
    BOOST_CHECK(!io_interface.alive(TEST_CLIENT));
    // Disconnection should be clean
    BOOST_CHECK(io_interface.killReason(TEST_CLIENT));
}

BOOST_AUTO_TEST_CASE(UnregisterWithErrorMessage) {
    SimpleNetworkBackend io_interface;

    // TEST_CLIENT is already connected by default but not registered
    io_interface.kill(TEST_CLIENT, RpT::Utils::HandlingResult { "Error reason" });

    // TEST_CLIENT should no longer be alive
    BOOST_CHECK(!io_interface.alive(TEST_CLIENT));
    // Disconnection should crash with an error message
    const RpT::Utils::HandlingResult status_error { io_interface.killReason(TEST_CLIENT) };
    BOOST_CHECK(!status_error);
    BOOST_CHECK_EQUAL(status_error.errorMessage(), "Error reason");
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * removeClient() unit tests
 */

BOOST_AUTO_TEST_SUITE(RemoveClient)

BOOST_AUTO_TEST_CASE(UnknownToken) {
    SimpleNetworkBackend io_interface;

    // Token 42 not used by any connected client
    BOOST_CHECK_THROW(io_interface.deleteClient(42), UnknownClientToken);
}

BOOST_AUTO_TEST_CASE(NormalDisconnection) {
    SimpleNetworkBackend io_interface;
    // TEST_CLIENT must first be set in dead mode
    io_interface.kill(TEST_CLIENT);

    // Disconnects default client which wasn't registered for no error reason
    io_interface.deleteClient(TEST_CLIENT);
    // Adding new client with CONSOLE_CLIENT token should works as it has been removed
    io_interface.newClient(TEST_CLIENT);
}

BOOST_AUTO_TEST_CASE(ErrorDisconnection) {
    SimpleNetworkBackend io_interface;
    // TEST_CLIENT must first be set in dead mode
    io_interface.kill(TEST_CLIENT);

    // Disconnects default client which wasn't registered for random error reason
    io_interface.deleteClient(TEST_CLIENT);
    // Adding new client with CONSOLE_CLIENT token should works as it has been removed
    io_interface.newClient(TEST_CLIENT);
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * closePipelineWith() unit tests
 */

BOOST_AUTO_TEST_SUITE(ClosePipelineWith)

BOOST_AUTO_TEST_CASE(Clean) {
    SimpleNetworkBackend io_interface;

    // Closes pseudo-connection with player 0 (registered at initialization) without errors
    io_interface.closePipelineWith(CONSOLE_ACTOR, {});

    /*
     * Pipeline closure NetworkBackend implementation should have emitted a LeftEvent after actor was unregistered
     */

    BOOST_CHECK(!io_interface.registered(CONSOLE_ACTOR));
    // Client Console should ne longer be alive
    BOOST_CHECK(!io_interface.alive(CONSOLE_CLIENT));
    // Disconnection should be clean
    BOOST_CHECK(io_interface.killReason(CONSOLE_CLIENT));

    const auto left_event { requireEventType<RpT::Core::LeftEvent>(io_interface.waitForInput()) };
    BOOST_CHECK_EQUAL(left_event.actor(), CONSOLE_ACTOR);
    // Disconnection should be clean
    BOOST_CHECK(left_event.disconnectionReason());
}

BOOST_AUTO_TEST_CASE(Crash) {
    SimpleNetworkBackend io_interface;

    // Closes pseudo-connection with player 0 (registered at initialization) with error message "ERROR"
    io_interface.closePipelineWith(CONSOLE_ACTOR, RpT::Utils::HandlingResult { "ERROR" });

    /*
     * Pipeline closure NetworkBackend implementation should have emitted a LeftEvent after actor was unregistered
     */

    BOOST_CHECK(!io_interface.registered(CONSOLE_ACTOR));
    // Client Console should ne longer be alive
    BOOST_CHECK(!io_interface.alive(CONSOLE_CLIENT));
    // Disconnection should be caused by crash
    const RpT::Utils::HandlingResult status_error { io_interface.killReason(CONSOLE_CLIENT) };
    BOOST_CHECK(!status_error);
    BOOST_CHECK_EQUAL(status_error.errorMessage(), "ERROR");

    const auto left_event { requireEventType<RpT::Core::LeftEvent>(io_interface.waitForInput()) };
    const RpT::Utils::HandlingResult disconnection_reason { left_event.disconnectionReason() };
    BOOST_CHECK_EQUAL(left_event.actor(), CONSOLE_ACTOR);
    // Disconnection should be caused by crash
    BOOST_CHECK(!disconnection_reason);
    BOOST_CHECK_EQUAL(disconnection_reason.errorMessage(), "ERROR");
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * handleHandshake() unit tests
 */

BOOST_AUTO_TEST_SUITE(HandleMessage)

/*
 * Client connection mode: unregistered
 */

BOOST_AUTO_TEST_SUITE(HandleHandshake)

BOOST_AUTO_TEST_CASE(Uid42NameAlvis) {
    SimpleNetworkBackend io_interface;

    // Sends handshake RPTL command for registration, from client token 1 using actor UID 42
    io_interface.clientMessage(TEST_CLIENT, "LOGIN 42 Alvis");

    // Actor UID 42 registration should have been called by NetworkBackend implementation
    BOOST_CHECK(io_interface.registered(42));
    // Client should still be alive
    BOOST_CHECK(io_interface.alive(TEST_CLIENT));

    // Player joined input event should have been emitted by NetworkBackend
    const auto joined_event { requireEventType<RpT::Core::JoinedEvent>(io_interface.waitForInput()) };
    BOOST_CHECK_EQUAL(joined_event.actor(), 42);
    BOOST_CHECK_EQUAL(joined_event.playerName(), "Alvis"); // Registered with name "Alvis" inside command
}

BOOST_AUTO_TEST_CASE(Uid1MissingName) {
    SimpleNetworkBackend io_interface;

    // Name argument is missing
    BOOST_CHECK_THROW(io_interface.clientMessage(TEST_CLIENT, "LOGIN 2 "), BadClientMessage);
    // Ill-formed handshake, should not be registered
    BOOST_CHECK(!io_interface.registered(2));
    // Client should still be alive, error handling is for NetworkBackend implementation
    BOOST_CHECK(io_interface.alive(TEST_CLIENT));
}

BOOST_AUTO_TEST_CASE(InvalidUid) {
    SimpleNetworkBackend io_interface;

    // UID argument isn't valid 64 bits unsigned integer
    BOOST_CHECK_THROW(io_interface.clientMessage(TEST_CLIENT, "LOGIN abcd "), BadClientMessage);
    // Client should still be alive, error handling is for NetworkBackend implementation
    BOOST_CHECK(io_interface.alive(TEST_CLIENT));
}

BOOST_AUTO_TEST_CASE(ExtraArgs) {
    SimpleNetworkBackend io_interface;

    // Extra arg "a"
    BOOST_CHECK_THROW(io_interface.clientMessage(TEST_CLIENT, "LOGIN 42 Alvis a"), BadClientMessage);
    // Ill-formed handshake, should not be registered
    BOOST_CHECK(!io_interface.registered(42));
    BOOST_CHECK(io_interface.alive(TEST_CLIENT));
}

BOOST_AUTO_TEST_CASE(NotAHandshake) {
    SimpleNetworkBackend io_interface;

    // UNKNOWN command is not handshaking command
    BOOST_CHECK_THROW(io_interface.clientMessage(TEST_CLIENT, "UNKNOWN 42 Alvis"), BadClientMessage);
    BOOST_CHECK(io_interface.alive(TEST_CLIENT));
}

BOOST_AUTO_TEST_CASE(UnavailableUid) {
    SimpleNetworkBackend io_interface;

    // Actor UID 0 isn't available, actor 0 at initialized, which is a server error
    BOOST_CHECK_THROW(io_interface.clientMessage(TEST_CLIENT, "LOGIN 0 Alvis"), InternalError);
    BOOST_CHECK(io_interface.registered(CONSOLE_ACTOR)); // Actual actor 0 should still be registered
    BOOST_CHECK(io_interface.alive(TEST_CLIENT));
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * Client connection mode: registered
 */

BOOST_AUTO_TEST_SUITE(HandleRegular)

BOOST_AUTO_TEST_CASE(ServiceCommandAnyRequest) {
    SimpleNetworkBackend io_interface;

    // Send any Service Request event with SERVICE command, NetworkBackend doesn't care about SER Protocol validity
    io_interface.clientMessage(CONSOLE_CLIENT, "SERVICE Any SR command");

    // Service Request event should have been emitted by NetworkBackend
    const auto event { requireEventType<RpT::Core::ServiceRequestEvent>(io_interface.waitForInput()) };
    BOOST_CHECK_EQUAL(event.actor(), CONSOLE_ACTOR); // Emitted by actor 0
    BOOST_CHECK_EQUAL(event.serviceRequest(), "Any SR command"); // With "Any SR command" args
}

BOOST_AUTO_TEST_CASE(ServiceCommandNoRequest) {
    SimpleNetworkBackend io_interface;

    // Send empty Service Request event with SERVICE command, NetworkBackend doesn't care about SER Protocol validity
    io_interface.clientMessage(CONSOLE_CLIENT, "SERVICE");

    // Service Request event should have been emitted by NetworkBackend
    const auto event { requireEventType<RpT::Core::ServiceRequestEvent>(io_interface.waitForInput()) };
    BOOST_CHECK_EQUAL(event.actor(), CONSOLE_ACTOR); // Emitted by actor 0
    BOOST_CHECK_EQUAL(event.serviceRequest(), ""); // With "Any SR command" args
}

BOOST_AUTO_TEST_CASE(LogoutCommandNoArgs) {
    SimpleNetworkBackend io_interface;

    // Send LOGOUT command from actor 0 (so it will be unregistered) with well-formed message (no extra args)
    io_interface.clientMessage(CONSOLE_CLIENT, "LOGOUT");

    // Console actor should no longer be registered
    BOOST_CHECK(!io_interface.registered(CONSOLE_ACTOR));
    // Console client should still no longer be alive because logged out
    BOOST_CHECK(!io_interface.alive(CONSOLE_CLIENT));

    // Left event should have been emitted by NetworkBackend
    const auto event { requireEventType<RpT::Core::LeftEvent>(io_interface.waitForInput()) };
    BOOST_CHECK_EQUAL(event.actor(), CONSOLE_ACTOR);
    // LOGOUT command is the clean way for client disconnection
    BOOST_CHECK(event.disconnectionReason());
}

BOOST_AUTO_TEST_CASE(LogoutCommandExtraArgs) {
    SimpleNetworkBackend io_interface;

    // Send LOGOUT command from actor 0 with ill-formed message (extra args) so it will not be properly parsed...
    BOOST_CHECK_THROW(io_interface.clientMessage(CONSOLE_CLIENT, "LOGOUT many extra args"), BadClientMessage);
    // ...and actor will not be unregistered by NetworkBackend (but its rpt-network implementations will probably do it
    // as pipeline could be broken)
    BOOST_CHECK(io_interface.registered(CONSOLE_ACTOR));
    // Logout didn't occurre, client should still be alive
    BOOST_CHECK(io_interface.alive(CONSOLE_CLIENT));
}

BOOST_AUTO_TEST_CASE(UnknownCommand) {
    SimpleNetworkBackend io_interface;

    // Send UNKNOWN_COMMAND which isn't a valid RPTL command, so message is ill-formed
    BOOST_CHECK_THROW(io_interface.clientMessage(CONSOLE_CLIENT, "UNKNOWN_COMMAND some args"), BadClientMessage);
}

BOOST_AUTO_TEST_CASE(EmptyMessage) {
    SimpleNetworkBackend io_interface;

    // Send message not containing any RPTL command which isn't a valid syntax, so message is ill-formed
    BOOST_CHECK_THROW(io_interface.clientMessage(CONSOLE_CLIENT, ""), BadClientMessage);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
