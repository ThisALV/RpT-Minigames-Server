#ifndef RPT_MINIGAMES_SERVER_SERVICEEVENT_HPP
#define RPT_MINIGAMES_SERVER_SERVICEEVENT_HPP

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @file ServiceEvent.hpp
 */


namespace RpT::Core {


/**
 * @brief Thrown by `ServiceEvent::targets()` if everyone must receive the Service Event
 */
class NoUidsList : public std::logic_error {
public:
    NoUidsList() : std::logic_error { "No UIDs provided, everyone must receive this SE" } {}
};


/**
 * @brief Represents a Service Event (SE) command with a list of actors which must receive that Event.
 *
 * @note Passing through `ServiceEventRequestProtocol` instance and other higher level protocols, new SE will be
 * constructed, prefixing the current SE data with a new protocol command.
 *
 * For example, if polled with `ServiceEventRequestProtocol::pollServiceEvent()`, `command()` return will be prefixed
 * with `EVENT <service_name> <event_data>`.
 *
 * @author ThisALV, https://github.com/ThisALV/
 */
class ServiceEvent {
private:
    /// Command providing Event data
    const std::string command_;
    /// Optional list of actors which must receive that Event
    const std::optional<std::vector<std::uint64_t>> targets_;

public:
    /**
     * @brief Constructs a Service Event represented with given command
     *
     * @param command SE data representation
     * @param actor_uids List of actor UIDs which must receive that SE, uninitialized if all actors must receive it
     */
    explicit ServiceEvent(std::string command, std::optional<std::initializer_list<std::uint64_t>> actor_uids = {});

    /**
     * @brief Retrieves a view on SE command
     *
     * @returns View on SE command data
     */
    std::string_view command() const;

    /**
     * @brief Checks if this Service Event must be sent to every registered actor
     *
     * @returns `true` if every registered actor must receive this, `false` otherwise
     */
    bool targetEveryone() const;

    /**
     * @brief Lists every actor which must receive this, if it is not sent to everyone
     *
     * @returns List of UIDs for actor which must receive this SE
     *
     * @throws NoUidsList if every registered actor must receive that SE <=> if `targetEveryone() == true`
     */
    const std::vector<std::uint64_t>& targets() const;
};


}


#endif //RPT_MINIGAMES_SERVER_SERVICEEVENT_HPP
