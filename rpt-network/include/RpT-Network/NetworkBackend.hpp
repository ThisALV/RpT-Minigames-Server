#ifndef RPTOGETHER_SERVER_NETWORKBACKEND_HPP
#define RPTOGETHER_SERVER_NETWORKBACKEND_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <RpT-Core/InputOutputInterface.hpp>

/**
 * @file NetworkBackend.hpp
 */


namespace RpT::Network {


/**
 * @brief Thrown by `NetworkBackend::handleClientMessage()` if received client RPTL message is ill-formed.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class BadClientMessage : public std::logic_error {
public:
    /**
     * @brief Constructs exception with custom error message
     *
     * @param reason Custom error message describing why client RPTL message is ill-formed
     */
    explicit BadClientMessage(const std::string& reason) : std::logic_error { reason } {}
};


/**
 * @brief Base class for `RpT::Core::InputOutputInterface` implementations based on networking protocol.
 *
 * Implements a common protocol (RPTL, or RpT Login Protocol) which is managing who is connected or disconnected, and
 * which is associating player informations (like name) with corresponding actor UID.
 *
 * RPTL Protocol (after connection began):
 * - Log in: `LOGIN <name>`
 * - Log out (clean way): `LOGOUT`
 * - Send Service Request command: if message doesn't begin with `LOGIN` or `LOGOUT`, it's parsed as SR command and
 * will be given to `RpT::Core::ServiceEventRequestProtocol` using `RpT::Core::ServiceRequestEvent`.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class NetworkBackend : public Core::InputOutputInterface {
private:
    std::uint64_t uid_count_;
    std::unordered_map<std::uint64_t, std::string> logged_in_actors_;

    /*
     * Prefixes for RPTL protocol commands invoked by clients
     */

    static constexpr std::string_view LOGIN_COMMAND { "LOGIN" };
    static constexpr std::string_view LOGOUT_COMMAND { "LOGOUT" };

protected:
    /**
     * @brief Handles given received RPTL message from client with associated actor UID and retrieves triggered input
     * event
     *
     * @param client_actor UID for actor representing this client
     * @param client_message Received client message (received network data) to handle
     *
     * @returns Event triggered by message, must be `Core::JoinedEvent`, `Core::LeftEvent` or
     * `Core::ServiceRequestEvent`, as only these events can be triggered by client
     *
     * @throws BadClientMessage if given client message is ill-formed (missing args)
     */
    Core::AnyInputEvent handleClientMessage(std::uint64_t client_actor, const std::string& client_message);
};


}


#endif //RPTOGETHER_SERVER_NETWORKBACKEND_HPP
