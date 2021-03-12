#include <RpT-Testing/TestingUtils.hpp>

#include <optional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <RpT-Network/NetworkBackend.hpp>


using namespace RpT::Network;


// << overloads must be defined inside this namespace, so ADL can be performed to find them when boost use <<
// operator on RpT::Core types
namespace RpT::Core {
// DUPLICATED FROM InputEventTests.cpp, putting it to TestingUtils.hpp would mean includes RpT-Core/InputEvent.hpp
// for all tests files including TestingUtils.hpp, not relevant


/**
 * @brief Prints stringified representation for `LeftEvent::Reason` value
 *
 * @param out Output stream for value
 * @param dc_reason Value to stringify and print
 *
 * @returns Stream which is printing `LeftEvent::Reason` value
 */
std::ostream& operator<<(std::ostream& out, const LeftEvent::Reason dc_reason) {
    switch (dc_reason) {
    case LeftEvent::Reason::Clean:
        return out << "Clean";
    case LeftEvent::Reason::Crash:
        return out << "Crash";
    }
}


}


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


/**
 * @brief Basic `NetworkBackend` implementation which stores actors as a simple dictionary UID -> name and take
 * strings handled as command with given client actor UID.
 *
 * Used for `NetworkBackend` mocking.
 */
class SimpleNetworkBackend : public NetworkBackend {
private:
    std::unordered_map<std::uint64_t, std::string> actors_registry_;

protected:
    /// Retrieves `NoneEvent` triggered by actor with UID == 0 when queue is empty
    RpT::Core::AnyInputEvent waitForEvent() override {
        return RpT::Core::NoneEvent { 0 };
    }

    /// Checks if actor UID is key in registry dictionary
    bool isRegistered(const std::uint64_t actor_uid) const override {
        return actors_registry_.count(actor_uid);
    }

    /// Insert UID and name pair into dictionary
    void registerActor(const std::uint64_t uid, const std::string_view name) override {
        const auto insertion_result { actors_registry_.insert({ uid, std::string { name } }) };

        assert(insertion_result.second); // Checks for insertion to be successfully done
    }

    /// Erase UID from dictionary
    void unregisterActor(std::uint64_t uid, const RpT::Utils::HandlingResult&) override {
        const std::size_t removed_count { actors_registry_.erase(uid) };

        assert(removed_count == 1); // Checks for deletion to be successfully done
    }

public:
    /// Initializes backend with client actor 0 for `waitForEvent()` return value
    SimpleNetworkBackend() : actors_registry_ { { 0, "Console" } } {}

    /// Handles given command as handshake from new client and push emitted event into queue
    void clientConnection(const std::string& handshake_command) {
        pushInputEvent(handleHandshake(handshake_command));
    }

    /// Handles given command as message from given client and push emitted event into queue
    void clientCommand(const std::uint64_t client_actor_uid, const std::string& command) {
        pushInputEvent(handleMessage(client_actor_uid, command));
    }

    /// Push given event directly into queue, trivial access to pushInputEvent() for testing purpose
    void trigger(RpT::Core::AnyInputEvent event) {
        pushInputEvent(std::move(event));
    }

    /// Checks if given actor UID is registered or not, trivial access to isRegister() for testing purpose
    bool registered(const std::uint64_t actor_uid) {
        return isRegistered(actor_uid);
    }

    /*
     * Not NetworkBackend responsibility, no need to be implemented
     */

    void replyTo(const std::uint64_t, const std::string&) override {
        throw std::runtime_error { "replyTo() not implemented for mocking" };
    }

    void outputEvent(const std::string&) override {
        throw std::runtime_error { "outputEvent() not implemented for mocking" };
    }
};


BOOST_AUTO_TEST_SUITE(NetworkBackendTests)

/*
 * waitForInput() unit tests
 */

BOOST_AUTO_TEST_SUITE(WaitForInput)

BOOST_AUTO_TEST_CASE(EmptyQueue) {
    SimpleNetworkBackend io_interface;

    // Gets input event with empty queue, expected call to waitForEvent() returning NoneEvent triggered by actor 0
    const auto event { requireEventType<RpT::Core::NoneEvent>(io_interface.waitForInput()) };
    
    BOOST_CHECK_EQUAL(event.actor(), 0);
}

BOOST_AUTO_TEST_CASE(SingleQueuedEvent) {
    SimpleNetworkBackend io_interface;

    // A new player named NoName with UID 1 joined server, event triggered
    io_interface.trigger(RpT::Core::JoinedEvent { 1, "NoName" });

    // Gets input event with non-empty queue, first pushed element will be returned
    const auto event { requireEventType<RpT::Core::JoinedEvent>(io_interface.waitForInput()) };

    BOOST_CHECK_EQUAL(event.actor(), 1);
    BOOST_CHECK_EQUAL(event.playerName(), "NoName");
}

BOOST_AUTO_TEST_CASE(ManuQueuedEvents) {
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
 * closePipelineWith() unit tests
 */

BOOST_AUTO_TEST_SUITE(ClosePipelineWith)

BOOST_AUTO_TEST_CASE(Clean) {
    SimpleNetworkBackend io_interface;

    // Closes pseudo-connection with player 0 (registered at initialization) without errors
    io_interface.closePipelineWith(0, {});

    /*
     * Pipeline closure NetworkBackend implementation should have emitted a LeftEvent after actor was unregistered
     */

    BOOST_CHECK(!io_interface.registered(0));

    const auto left_event { requireEventType<RpT::Core::LeftEvent>(io_interface.waitForInput()) };
    BOOST_CHECK_EQUAL(left_event.actor(), 0);
    // Disconnection should be clean
    BOOST_CHECK_EQUAL(left_event.disconnectionReason(), RpT::Core::LeftEvent::Reason::Clean);
}

BOOST_AUTO_TEST_CASE(Crash) {
    SimpleNetworkBackend io_interface;

    // Closes pseudo-connection with player 0 (registered at initialization) with error message "ERROR"
    io_interface.closePipelineWith(0, RpT::Utils::HandlingResult { "ERROR" });

    /*
     * Pipeline closure NetworkBackend implementation should have emitted a LeftEvent after actor was unregistered
     */

    BOOST_CHECK(!io_interface.registered(0));

    const auto left_event { requireEventType<RpT::Core::LeftEvent>(io_interface.waitForInput()) };
    BOOST_CHECK_EQUAL(left_event.actor(), 0);
    // Disconnection should be caused by crash
    BOOST_CHECK_EQUAL(left_event.disconnectionReason(), RpT::Core::LeftEvent::Reason::Crash);
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * handleHandshake() unit tests
 */

BOOST_AUTO_TEST_SUITE(HandleHandshake)

BOOST_AUTO_TEST_CASE(Uid42NameAlvis) {
    SimpleNetworkBackend io_interface;

    io_interface.clientConnection("LOGIN 42 Alvis"); // Sends handshake RPTL command for registration

    // Actor UID 42 registration should have been called by NetworkBackend implementation
    BOOST_CHECK(io_interface.registered(42));

    // Player joined input event should have been emitted by NetworkBackend
    const auto joined_event { requireEventType<RpT::Core::JoinedEvent>(io_interface.waitForInput()) };
    BOOST_CHECK_EQUAL(joined_event.actor(), 42);
    BOOST_CHECK_EQUAL(joined_event.playerName(), "Alvis"); // Registered with name "Alvis" inside command
}

BOOST_AUTO_TEST_CASE(Uid1MissingName) {
    SimpleNetworkBackend io_interface;

    BOOST_CHECK_THROW(io_interface.clientConnection("LOGIN 1 "), BadClientMessage); // Name argument is missing
    BOOST_CHECK(!io_interface.registered(1)); // Ill-formed handshake, should not be registered
}

BOOST_AUTO_TEST_CASE(InvalidUid) {
    SimpleNetworkBackend io_interface;

    // UID argument isn't valid 64 bits unsigned integer
    BOOST_CHECK_THROW(io_interface.clientConnection("LOGIN abcd "), BadClientMessage);
}

BOOST_AUTO_TEST_CASE(ExtraArgs) {
    SimpleNetworkBackend io_interface;

    BOOST_CHECK_THROW(io_interface.clientConnection("LOGIN 42 Alvis a"), BadClientMessage); // Extra arg "a"
    BOOST_CHECK(!io_interface.registered(42)); // Ill-formed handshake, should not be registered
}

BOOST_AUTO_TEST_CASE(NotAHandshake) {
    SimpleNetworkBackend io_interface;

    // UNKNOWN command is not handshaking command
    BOOST_CHECK_THROW(io_interface.clientConnection("UNKNOWN 42 Alvis"), BadClientMessage);
}

BOOST_AUTO_TEST_CASE(UnavailableUid) {
    SimpleNetworkBackend io_interface;

    // Actor UID 0 isn't available, actor 0 at initialized, which is a server error
    BOOST_CHECK_THROW(io_interface.clientConnection("LOGIN 0 Alvis"), InternalError);
    BOOST_CHECK(io_interface.registered(0)); // Actual actor 0 should still be registered
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * handleMessage() unit tests
 */

BOOST_AUTO_TEST_SUITE(HandleMessage)

BOOST_AUTO_TEST_CASE(ServiceCommandAnyRequest) {
    SimpleNetworkBackend io_interface;

    // Send any Service Request event with SERVICE command, NetworkBackend doesn't care about SER Protocol validity
    io_interface.clientCommand(0, "SERVICE Any SR command");

    // Service Request event should have been emitted by NetworkBackend
    const auto event { requireEventType<RpT::Core::ServiceRequestEvent>(io_interface.waitForInput()) };
    BOOST_CHECK_EQUAL(event.actor(), 0); // Emitted by actor 0
    BOOST_CHECK_EQUAL(event.serviceRequest(), "Any SR command"); // With "Any SR command" args
}

BOOST_AUTO_TEST_CASE(ServiceCommandNoRequest) {
    SimpleNetworkBackend io_interface;

    // Send empty Service Request event with SERVICE command, NetworkBackend doesn't care about SER Protocol validity
    io_interface.clientCommand(0, "SERVICE");

    // Service Request event should have been emitted by NetworkBackend
    const auto event { requireEventType<RpT::Core::ServiceRequestEvent>(io_interface.waitForInput()) };
    BOOST_CHECK_EQUAL(event.actor(), 0); // Emitted by actor 0
    BOOST_CHECK_EQUAL(event.serviceRequest(), ""); // With "Any SR command" args
}

BOOST_AUTO_TEST_CASE(LogoutCommandNoArgs) {
    SimpleNetworkBackend io_interface;

    // Send LOGOUT command from actor 0 (so it will be unregistered) with well-formed message (no extra args)
    io_interface.clientCommand(0, "LOGOUT");

    // Actor 0 should no longer be registered
    BOOST_CHECK(!io_interface.registered(0));

    // Left event should have been emitted by NetworkBackend
    const auto event { requireEventType<RpT::Core::LeftEvent>(io_interface.waitForInput()) };
    BOOST_CHECK_EQUAL(event.actor(), 0);
    // LOGOUT command is the clean way for client disconnection
    BOOST_CHECK_EQUAL(event.disconnectionReason(), RpT::Core::LeftEvent::Reason::Clean);
}

BOOST_AUTO_TEST_CASE(LogoutCommandExtraArgs) {
    SimpleNetworkBackend io_interface;

    // Send LOGOUT command from actor 0 with ill-formed message (extra args) so it will not be properly parsed...
    BOOST_CHECK_THROW(io_interface.clientCommand(0, "LOGOUT many extra args"), BadClientMessage);
    // ...and actor will not be unregistered by NetworkBackend (but its rpt-network implementations will probably do it
    // as pipeline could be broken)
    BOOST_CHECK(io_interface.registered(0));

}

BOOST_AUTO_TEST_CASE(UnknownCommand) {
    SimpleNetworkBackend io_interface;

    // Send UNKNOWN_COMMAND which isn't a valid RPTL command, so message is ill-formed
    BOOST_CHECK_THROW(io_interface.clientCommand(0, "UNKNOWN_COMMAND some args"), BadClientMessage);
}

BOOST_AUTO_TEST_CASE(EmptyMessage) {
    SimpleNetworkBackend io_interface;

    // Send message not containing any RPTL command which isn't a valid syntax, so message is ill-formed
    BOOST_CHECK_THROW(io_interface.clientCommand(0, ""), BadClientMessage);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
