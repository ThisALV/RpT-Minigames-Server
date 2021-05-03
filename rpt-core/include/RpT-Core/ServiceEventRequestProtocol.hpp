#ifndef RPTOGETHER_SERVER_SERVICEEVENTREQUESTPROTOCOL_HPP
#define RPTOGETHER_SERVER_SERVICEEVENTREQUESTPROTOCOL_HPP

#include <deque>
#include <functional>
#include <initializer_list>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <RpT-Core/Service.hpp>
#include <RpT-Utils/HandlingResult.hpp>
#include <RpT-Utils/LoggerView.hpp>
#include <RpT-Utils/TextProtocolParser.hpp>


/**
 * @file ServiceEventRequestProtocol.hpp
 */


namespace RpT::Core {


/**
 * @brief Thrown by `ServiceEventRequestProtocol` constructor if trying to register already registered service name
 *
 * @see ServiceEventRequestProtocol
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class ServiceNameAlreadyRegistered : public std::logic_error {
public:
    /**
     * @brief Constructs exception with error message including already registered name
     *
     * @param name Name used for trying to register service
     */
    explicit ServiceNameAlreadyRegistered(const std::string_view name) :
        std::logic_error { "Service with name \"" + std::string { name } + "\" is already registered" } {}
};

/**
 * @brief Base class for errors about ill-formed Service Request command.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class BadServiceRequest : public std::logic_error {
public:
    /**
     * @brief Constructs error with custom message
     *
     * @param reason Custom error message
     */
    explicit BadServiceRequest(const std::string& reason) : std::logic_error { reason } {}
};

/**
 * @brief Thrown by `ServiceEventRequestProtocol::handleServiceRequest()` if trying to execute SR command for
 * unregistered service
 *
 * @see ServiceEventRequestProtocol
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class ServiceNotFound : public BadServiceRequest {
public:
    /**
     * @brief Constructs exception with error message including unregistered service name
     *
     * @param name Name used for trying to access service
     */
    explicit ServiceNotFound(const std::string_view name) :
            BadServiceRequest { "Service with name \"" + std::string { name } + "\" not found" } {}
};

/**
 * @brief Thrown by `ServiceEventRequestProtocol::handleServiceRequest()` if trying to executor SR command using bad
 * format
 *
 * @see ServiceEventRequestProtocol
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class InvalidRequestFormat : public BadServiceRequest {
public:
    /**
     * @brief Constructs exception with error message including ill formed SR command and bad format reason
     *
     * @param sr_command SR command that wasn't executed
     * @param reason Reason why SR command is ill formed
     */
    explicit InvalidRequestFormat(const std::string_view sr_command, const std::string& reason) :
            BadServiceRequest { "SR command \"" + std::string { sr_command } + "\" ill formed: " + reason } {}
};


/**
 * @brief Communication protocol for Event/Request based services
 *
 * Runs a list of named services, learn more about services at `Service` class doc.
 *
 * Each service can receives request from actors and emits event to actors.
 *
 * Service Requests (SR) commands are received from actors, containing Request UID (RUID), intended service name and
 * command handled by this named service. When SR command is handled by SER Protocol instance, it's contained
 * service command will be handled by targeted named service.
 * A service may returns successfully or with an error message in case of errors during command handling. Service
 * Request Response (SRR) is sent to SR command actor so it can know about its requests result. This SRR will contain
 * SR command RUID so it can be properly identified.
 * SR commands are used by actors to interact with server features.
 *
 * RUID format is unsigned integer of 64 bits.
 *
 * Service Events (SR) commands are sent to actors by services in the same order they were emitted by them. SE
 * commands are used by services to notify state changes which could be caused by an actor request or not.
 *
 * SER Protocol:
 *
 * - Service Request command (SR) : `REQUEST <RUID> <SERVICE_NAME> <command_data>`
 * - Service Request Response (SRR) : `RESPONSE <RUID> OK` or `RESPONSE <RUID> KO <ERR_MSG>`
 * - Service Event command (SE) : `EVENT <SERVICE_NAME> <command_data>`
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class ServiceEventRequestProtocol {
private:
    // Prefix for Service Request (SR) commands
    static constexpr std::string_view REQUEST_PREFIX { "REQUEST" };
    // Prefix for Service Request Response (SRR) commands
    static constexpr std::string_view RESPONSE_PREFIX { "RESPONSE" };
    // Prefix for Service Event (SE) commands
    static constexpr std::string_view EVENT_PREFIX { "EVENT" };

    /**
     * @brief Parses given SR command prefix, request UID and intended service's name
     *
     * @author ThisALV, https://github.com/ThisALV
     */
    class ServiceRequestCommandParser : public Utils::TextProtocolParser {
    private:
        std::uint64_t parsed_ruid_;

    public:
        /// Constructs parser for given command, will parse exactly 3 words for prefix, RUID conversion and service name
        explicit ServiceRequestCommandParser(std::string_view sr_command);

        /// Checks if SR command begins with correct prefix
        bool isValidRequest() const;

        /// Retrieves RUID
        std::uint64_t ruid() const;

        /// Retrieves intended service name
        std::string_view intendedServiceName() const;

        /// Retrieves command that will be passed to Service handler
        std::string_view commandData() const;
    };

    Utils::LoggerView logger_;
    std::unordered_map<std::string_view, std::reference_wrapper<Service>> running_services_;

public:
    /**
     * @brief Initialize SER Protocol with given services to run
     *
     * Each service will be named from its `Service::name()` returned value.
     *
     * @throws ServiceNameAlreadyRegistered if a service name appears twice into services list
     *
     * @param services References to services
     * @param logging_context for SER Protocol logging
     */
    ServiceEventRequestProtocol(const std::initializer_list<std::reference_wrapper<Service>>& services,
                                Utils::LoggingContext& logging_context);

    /**
     * @brief Get if given service is already registered
     *
     * @param service Name of service to check for
     *
     * @returns If registered services contains a service with given name
     */
    bool isRegistered(std::string_view service) const;

    /**
     * @brief Try to treat the given Service Request command
     *
     * Find appropriate service, and make it handle the given SR command with actor executor.
     *
     * @param actor UID for actor who's trying to execute that SR command
     * @param service_request Service Request command to handle
     *
     * @returns Service Request Response (SRR) which has to sent to SR actor
     *
     * @throws BadServiceRequest if SR command is ill-formed
     */
    std::string handleServiceRequest(std::uint64_t actor, std::string_view service_request);

    /**
     * @brief Poll next Service Event command in services queue, do nothing if queue is empty
     *
     * @returns Optional value, initialized to next SE command if it exists, uninitialized otherwise
     */
    std::optional<std::string> pollServiceEvent();
};


}

#endif // RPTOGETHER_SERVER_SERVICEEVENTREQUESTPROTOCOL_HPP
