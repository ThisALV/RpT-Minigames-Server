#ifndef RPT_MINIGAMES_SERVICES_CHATSERVICE_HPP
#define RPT_MINIGAMES_SERVICES_CHATSERVICE_HPP

/**
 * @file ChatService.hpp
 */

#include <RpT-Core/Service.hpp>
#include <RpT-Core/Timer.hpp>


/// Contains RpT Minigames project relative services, like chat, Açores, Bermudes and Canaries
namespace MinigamesServices {


/**
 * @brief Copies trimmed string from given string view.
 *
 * @param chat_message Sent chat message copied and trimmed.
 *
 * @returns Copied chars which aren't beginning or trailing whitespaces
 */
std::string trim(std::string_view chat_message);


/**
 * @brief Basic messaging service with actors which implements a cooldown (minimal delay) between each sent message
 */
class ChatService : public RpT::Core::Service {
private:
    const std::string cooldown_msg_;
    RpT::Core::Timer cooldown_;

public:
    /**
     * @brief Initializes service inside given context with given delay between each message
     *
     * @param run_context Context containing server services, providing events & timers ID
     * @param cooldown_ms Delay to wait before sending next message when a message is sent
     */
    explicit ChatService(RpT::Core::ServiceContext& run_context, std::size_t cooldown_ms);

    /// Retrieves service name `Chat`
    std::string_view name() const override;

    /**
     * @brief Sends given Service Request command data as message if at least one of its chars isn't a whitespace and
     * if previous message was send since more than `ChatService` cooldown milliseconds
     *
     * @param actor UID for actor who's sending a message
     * @param sr_command_data Raw message to be send
     *
     * @returns Empty `RpT::Utils::HandlingResult` if message was sent successfully, `RpT::Utils::HandlingResult` with
     * an error message otherwise
     */
    RpT::Utils::HandlingResult handleRequestCommand(std::uint64_t actor, std::string_view sr_command_data) override;
};


}


#endif //RPT_MINIGAMES_SERVICES_CHATSERVICE_HPP
