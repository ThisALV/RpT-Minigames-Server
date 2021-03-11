#include <RpT-Testing/TestingUtils.hpp>

#include <optional>
#include <unordered_map>
#include <RpT-Network/NetworkBackend.hpp>


using namespace RpT::Network;


/**
 * @brief Basic `NetworkBackend` implementation which stores actors as a simple dictionary UID -> name and take
 * strings handled as command with given client actor UID.
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
    void unregisterActor(std::uint64_t uid, const RpT::Utils::HandlingResult& disconnection_reason) override {
        const std::size_t removed_count { actors_registry_.erase(uid) };

        assert(removed_count == 1); // Checks for deletion to be successfully done
    }

public:
    /// Initializes backend with client actor 0 for `waitForEvent()` returned value
    SimpleNetworkBackend() : actors_registry_ { { 0, "Console" } } {}

    /// Handles given command as handshake from new client
    void clientConnection(const std::string& handshake_command) {
        handleHandshake(handshake_command);
    }

    /// Handles given command as message from given client
    void clientCommand(const std::uint64_t client_actor_uid, const std::string& command) {
        handleMessage(client_actor_uid, command);
    }
};


BOOST_AUTO_TEST_SUITE(NetworkBackendTests)

BOOST_AUTO_TEST_CASE(Nothing) {}

BOOST_AUTO_TEST_SUITE_END()
