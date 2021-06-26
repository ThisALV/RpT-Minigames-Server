#ifndef RPT_MINIGAMES_SERVER_SERVICEEVENT_HPP
#define RPT_MINIGAMES_SERVER_SERVICEEVENT_HPP

#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>

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
 * @brief Represents a Service Event (SE) command with a set of actors which must receive that Event.
 *
 * Passing through `ServiceEventRequestProtocol` instance and other higher level protocols, a new SE instance
 * command will be prefixed, inserting the given command at the beginning of the Event data.
 *
 * For example, if polled with `ServiceEventRequestProtocol::pollServiceEvent()`, the new instance`command()` return
 * will be prefixed with `EVENT <service_name> <event_data>`.
 *
 * @author ThisALV, https://github.com/ThisALV/
 */
class ServiceEvent {
private:
    /// Optional set of actors which must receive that Event
    std::optional<std::unordered_set<std::uint64_t>> targets_;
    /// Command providing Event data
    std::string command_;

public:
    /**
     * @brief Constructs a Service Event represented with given command
     *
     * @param command SE data representation
     * @param actor_uids Set of actor UIDs which must receive that SE, uninitialized if all actors must receive it
     */
    explicit ServiceEvent(std::string command, std::optional<std::unordered_set<std::uint64_t>> actor_uids = {});

    /**
     * @brief Checks if two SE are the same event
     *
     * @param rhs Event to compare with this instance
     *
     * @returns `true` if both command and optional actor UIDs list are equal from `*this` to `rhs`, `false` otherwise
     */
    bool operator==(const ServiceEvent& rhs) const;

    /// Returns the opposite of `operator==`
    bool operator!=(const ServiceEvent& rhs) const;

    /**
     * @brief Inserts given protocol command at the SE command data beginning of a new instance
     *
     * @param higher_protocol_prefix Protocol command to insert
     *
     * @returns A new instance with SE event data prefixed using given higher protocol command
     */
    ServiceEvent prefixWith(const std::string& higher_protocol_prefix) const;

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
    const std::unordered_set<std::uint64_t>& targets() const;
};


}


#endif //RPT_MINIGAMES_SERVER_SERVICEEVENT_HPP
