#ifndef RPTOGETHER_SERVER_SERVICEEVENTREQUESTPROTOCOL_HPP
#define RPTOGETHER_SERVER_SERVICEEVENTREQUESTPROTOCOL_HPP

#include <functional>
#include <initializer_list>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>


/**
 * @file ServiceEventRequestProtocol.hpp
 */


namespace RpT::Core {


/**
 * @brief Thrown if event is pushed on a pipeline which isn't referring to valid SER Protocol events queue
 *
 * It is typically thrown if `EventsPipeline` push event into null_ptr destination.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class PipelineNotBound : std::logic_error {
public:
    PipelineNotBound() : std::logic_error { "Pipeline isn't bound to any valid destination events queue" } {}
};

/**
 * @brief Pipeline for events emitted by services to SER Protocol
 *
 * Instances has reference to events queue for current SER Protocol instance (see `ServiceEventRequestProtocol`), so
 * events can be pushed from this class without giving public access to SER Protocol private events queue.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class EventsPipeline {
private:
    std::queue<std::string>* destination_events_queue_; // Must be copyable, so pointer is used instead of reference

public:
    EventsPipeline();

    /**
     * @brief Constructs pipeline using given destination events queue
     *
     * @param active_protocol_events_queue Destination for pushed events
     */
    explicit EventsPipeline(std::queue<std::string>& active_protocol_events_queue);

    /**
     * @brief Push Service Event command into events queue
     *
     * @param se_command SE command to push
     *
     * @throws PipelineNotBound if default constructed (which means that pipeline destination isn't valid)
     */
    void pushServiceEvent(std::string se_command);
};

/**
 * @brief Service ran by `ServiceEventRequestProtocol`
 *
 * Service requirement is being able to handle SR commands.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class Service {
protected:
    EventsPipeline events_queue_;

public:
    /**
     * @brief Set SER Protocol which is dispatching emitted events
     *
     * @param events_queue_pipeline Events pipeline to SER Protocol events queue that must dispatch emitted events
     */
    void emitEventsFor(EventsPipeline events_queue_pipeline);

    /**
     * @brief Get service name for registration
     *
     * @returns Service name
     */
    virtual std::string_view name() const = 0;

    /**
     * @brief Tries to handle a given command executed by a given actor
     *
     * @param actor Actor who's trying to execute the given SR command
     * @param sr_command_arguments Service Request command arguments (or command data words)
     *
     * @returns Boolean which indicates if SR command was successfully handled
     */
    virtual bool handleRequestCommand(std::string_view actor,
                                      const std::vector<std::string_view>& sr_command_arguments) = 0;
};

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
 * @brief Thrown by `ServiceEventRequestProtocol::handleServiceRequest()` if trying to execute SR command for
 * unregistered service
 *
 * @see ServiceEventRequestProtocol
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class ServiceNotFound : public std::logic_error {
public:
    /**
     * @brief Constructs exception with error message including unregistered service name
     *
     * @param name Name used for trying to access service
     */
    explicit ServiceNotFound(const std::string_view name) :
            std::logic_error { "Service with name \"" + std::string { name } + "\" not found" } {}
};

/**
 * @brief Thrown by `ServiceEventRequestProtocol::handleServiceRequest()` if trying to executor SR command using bad
 * format
 *
 * @see ServiceEventRequestProtocol
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class BadServiceRequest : public std::logic_error {
public:
    /**
     * @brief Constructs exception with error message including ill formed SR command and bad format reason
     *
     * @param sr_command SR command that wasn't executed
     * @param reason Reason why SR command is ill formed
     */
    explicit BadServiceRequest(const std::string_view sr_command, const std::string& reason) :
            std::logic_error { "SR command \"" + std::string { sr_command } + "\" ill formed: " + reason } {}
};

/**
 * @brief Communication protocol for Event/Request based services
 *
 * Runs a list of named services, learn more about services at `Service` class doc.
 *
 * Each service can receives request from actors and emits event to actors. Service Request (SR) and Service Event
 * (SE) both contain command data which describe what action is performed by service.
 *
 * A request is an action that any actor want to perform with a service. The service request command is handled
 * into protocol and the service tries ot handle it. If it was successfully handled, service sends response
 * `OK` to actor and the request must be dispatched across all other actors, else, it sends response `KO` to actor.
 *
 * Each sent request must come with a request UID (or RUID) to identify itself among other requests when replying to
 * it. RUID must 64bits hexadecimal represented value to avoid possible clashes.
 *
 * An event is an action performed by a service itself that must be dispatched across all the actors.
 *
 * SER Protocol:
 *
 * - Service Request command (SR) : `REQUEST <RUID> <SERVICE_NAME> <command_data>`
 * - Service Request Response (SRR) : `RESPONSE <RUID> <OK>` or `RESPONSE <RUID> <KO> <ERR_MSG>`
 * - Service Request Dispatch (SRD) : `REQUEST_FROM <actor> <SERVICE_NAME> <command_data>`
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
     * @brief Get list of words inside given Service Request command, separated by char ' '
     *
     * @param sr_command Service Request command to parse
     *
     * @returns String views to each separated word contained inside `sr_command`
     */
    static std::vector<std::string_view> getWordsFor(std::string_view sr_command);

    std::unordered_map<std::string_view, std::reference_wrapper<Service>> running_services_;
    std::queue<std::string> services_events_queue_;

public:
    /**
     * @brief Initialize SER Protocol with given services to run
     *
     * Each service will be named from its `Service::name()` returned value.
     *
     * @throws ServiceNameAlreadyRegistered if a service name appears twice into services list
     *
     * @param services References to services
     */
    ServiceEventRequestProtocol(const std::initializer_list<std::reference_wrapper<Service>>& services);

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
     * @param actor Actor who's trying to execute that SR command
     * @param sr_command Service Request command to handle
     *
     * @throws BadServiceRequest if service_request is ill formed
     * @throws ServiceNotFound if service into service_request isn't registered
     *
     * @returns Optional value, initialized with error message if not handled successfully, uninitialized else
     */
    bool handleServiceRequest(std::string_view actor, std::string_view service_request);

    /**
     * @brief Poll next Service Event command in queue, do nothing if queue is empty
     *
     * @returns Optional value, initialized to next SE command if it exists, uninitialized else
     */
    std::optional<std::string> pollServiceEvent();
};


}

#endif // RPTOGETHER_SERVER_SERVICEEVENTREQUESTPROTOCOL_HPP
