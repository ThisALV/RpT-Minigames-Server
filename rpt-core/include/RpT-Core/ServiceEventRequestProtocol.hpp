#ifndef RPTOGETHER_SERVER_SERVICEEVENTREQUESTPROTOCOL_HPP
#define RPTOGETHER_SERVER_SERVICEEVENTREQUESTPROTOCOL_HPP

#include <functional>
#include <initializer_list>
#include <queue>
#include <string>
#include <string_view>
#include <unordered_map>


/**
 * @file ServiceEventRequestProtocol.hpp
 */


namespace RpT::Core {


/**
 * @brief Service ran by `ServiceEventRequestProtocol`
 *
 * Service requirement is being able to handle SR commands.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class Service {
public:
    /**
     * @brief Get service name for registration
     *
     * @returns Service name
     */
    virtual std::string_view name() const = 0;

    /**
     * @brief Tries to handle a given Service Request command executed for a given actor
     *
     * @param actor Actor who's trying to execute the given SR command
     * @param sr_command Service Request command to handle
     *
     * @returns Boolean which indicates if SR command was successfully handled
     */
    virtual bool handleRequestCommand(std::string_view actor, std::string_view sr_command) = 0;
};

/**
 * @brief Communication protocol for Event/Request based services
 *
 * Runs a list of named services, learn more about services at `Service` class doc.
 *
 * Each service can receives request from actors and emits event to actors.
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
 * - Service Request command (SR) : `REQUEST <RUID> <SERVICE_NAME> <data>`
 * - Service Request Response (SRR) : `RESPONSE <RUID> <OK>` or `RESPONSE <RUID> <KO> <ERR_MSG>`
 * - Service Event command (SE) : `EVENT <SERVICE_NAME> <data>`
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class ServiceEventRequestProtocol {
private:
    std::unordered_map<std::string_view, std::reference_wrapper<Service>> running_services_;
    std::queue<std::string> services_events_queue_;

public:
    /**
     * @brief Initialize SER Protocol with given services to run
     *
     * Each service will be named from its `Service::name()` returned value.
     *
     * @param services References to services
     */
    ServiceEventRequestProtocol(const std::initializer_list<std::reference_wrapper<Service>>& services);

    /**
     * @brief Try to treat the given Service Request command
     *
     * Find an appropriate service, and make it handle the given SR command with actor executor.
     *
     * @param actor Actor who's trying to execute that SR command
     * @param sr_command Service Request command to handle
     *
     * @returns Boolean indicates if service was found and if SR command was successfully handled
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
