#ifndef RPTOGETHER_SERVER_SERVICE_HPP
#define RPTOGETHER_SERVER_SERVICE_HPP

#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <RpT-Utils/HandlingResult.hpp>

/**
 * @file Service.hpp
 */


namespace RpT::Core {


/**
 * @brief Provides a context for services to run, same instance expected for constructs all Service instances
 * registered in same SER Protocol.
 *
 * Instance is used for providing events ID.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class ServiceContext {
private:
    std::size_t events_count_;

public:
    /**
     * @brief Initialize events count at 0
     */
    ServiceContext();

    /**
     * @brief Increments events count and retrieve its previous value
     *
     * @note Called by `Service::emitEvent()` for retrieving triggered event ID, shouldn't be called by user.
     *
     * @return Previous value for events count
     */
    std::size_t newEventPushed();
};


/**
 * @brief Thrown if trying to poll event when queue is empty
 *
 * @see Service
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class EmptyEventsQueue : public std::logic_error {
public:
    /**
     * @brief Constructs error with basic message including queue service name
     *
     * @param service Name of service the queue belongs to
     */
    explicit EmptyEventsQueue(const std::string_view service) :
            std::logic_error { "No more events for \"" + std::string { service } + '"' } {}
};


/**
 * @brief Service ran by `ServiceEventRequestProtocol`
 *
 * Service requirement is being able to handle SR commands, implementations must define `handleRequestCommand()`
 * virtual method.
 *
 * Implementations will access protected method `emitEvent()` so they can trigger events later polled by any SER
 * Protocol instance.
 *
 * Each service possesses its own events queue, and each event contains a event ID provided by `ServiceContext`, which
 * allows knowing what event was triggered first (as ID is growing from low to high) and an event command,
 * corresponding to words after `EVENT` prefix and service name inside Service Event command.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class Service {
private:
    ServiceContext& run_context_;
    std::queue<std::pair<std::size_t, std::string>> events_queue_;

protected:
    /**
     * @brief Emits event command into service
     *
     * @param event_command Event command to emit (words coming after `EVENT` prefix and service name in SE command)
     */
    void emitEvent(std::string event_command);

public:
    /// Returned by `checkEvent()` when service events queue is empty
    static constexpr std::optional<std::size_t> EMPTY_QUEUE {};

    /**
     * @brief Constructs service with empty events queue and given run context
     *
     * @param run_context Context, should be same instance for services registered in same SER Protocol
     */
    explicit Service(ServiceContext& run_context);

    /**
     * @brief Get next event ID so check for newest event between services can be performed
     *
     * @note Called by `ServiceEventRequestProtocol` instance to perform described check, shouldn't be called by user.
     *
     * @returns ID for next queued event, or uninitialized if queue is empty
     */
    std::optional<std::size_t> checkEvent() const;

    /**
     * @brief Get next event command
     *
     * @note Called by `ServiceEventRequestProtocol` instance to dispatch across actors, shouldn't be called by user.
     *
     * @returns Command for next queued event
     *
     * @throws EmptyEventsQueue if queue is empty so event cannot be polled
     */
    std::string pollEvent();

    /**
     * @brief Get service name for registration
     *
     * @returns Service name
     */
    virtual std::string_view name() const = 0;

    /**
     * @brief Tries to handle a given command executed by a given actor
     *
     * @param actor UID for actor who's trying to execute the given SR command
     * @param sr_command_data Service Request command arguments (or command data words)
     *
     * @returns Result for command handling, evaluates to `true` is succeeded, else contains an error message
     */
    virtual Utils::HandlingResult handleRequestCommand(std::uint64_t actor, std::string_view sr_command_data) = 0;
};


}


#endif //RPTOGETHER_SERVER_SERVICE_HPP
