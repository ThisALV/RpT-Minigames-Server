#include <Minigames-Services/ChatService.hpp>


namespace MinigamesServices {


std::string trim(const std::string_view chat_message) {
    auto msg_begin { chat_message.cbegin() };
    auto msg_end { chat_message.cend() };

    // While end of string isn't reached and current char isn't a whitespace, go to next char
    while (msg_begin != msg_end && std::isspace(*msg_begin))
        msg_begin++;

    // If end of string isn't reach, then msg_end - 1 is a valid iterator and there is at least one non-whitespace char
    if (msg_begin != msg_end) {
        // End iterator begins after last char, so -1 accesses currently checked char
        while (std::isspace(*(msg_end - 1)))
            msg_end--;
    }

    return { msg_begin, msg_end }; // Copy-elision for substring containing trimmed chat message
}


ChatService::ChatService(RpT::Core::ServiceContext& run_context, const std::size_t cooldown_ms)
: RpT::Core::Service { run_context, { cooldown_ } },
  cooldown_msg_ { "Last message when sent less than " + std::to_string(cooldown_ms) + " ms ago" },
  cooldown_ { run_context, cooldown_ms } {}

std::string_view ChatService::name() const {
    return "Chat";
}

RpT::Utils::HandlingResult ChatService::handleRequestCommand(const std::uint64_t actor,
                                                             const std::string_view sr_command_data) {

    // Copies message data to trim then send it if it is not "invisible"
    const std::string chat_message { trim(sr_command_data) };

    if (chat_message.empty()) // Checks for chat message to not be "invisible" (<=> empty after trim)
        return RpT::Utils::HandlingResult { "Message cannot be empty" };

    if (cooldown_.hasTriggered()) // If previous cooldown timed out, a new message can be sent, disable countdown
        cooldown_.clear();

    if (!cooldown_.isFree()) // If cooldown timer is still running, then message cannot be sent
        return RpT::Utils::HandlingResult { cooldown_msg_ };

    // Sends message to actors
    emitEvent("MESSAGE_FROM " + std::to_string(actor) + ' ' + chat_message);
    // Starts messages cooldown, next message not before given milliseconds delay
    cooldown_.requestCountdown();

    return {}; // Request successfully handled
}


}
