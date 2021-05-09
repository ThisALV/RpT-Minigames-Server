#ifndef RPT_MINIGAMES_SERVER_MESSAGESQUEUEVIEW_HPP
#define RPT_MINIGAMES_SERVER_MESSAGESQUEUEVIEW_HPP

#include <functional>
#include <memory>
#include <queue>
#include <stdexcept>

/**
 * @file MessagesQueueView.hpp
 */


namespace RpT::Network {


/**
 * @brief Thrown by `MessagesQueueView::next()` if underlying messages queue is empty
 *
 * @author ThisALV, https://github.com/ThisALV/
 */
class NoMoreMessage : public std::logic_error {
public:
    /**
     * @brief Constructs error with basic error message
     */
    NoMoreMessage(): std::logic_error { "No more RPTL messages to send" } {}
};


/**
 * Provides access to a RPTL messages queue consumation forbidding insertion/add operations.
 *
 * @author ThisALV, https://github.com/ThisALV/
 */
class MessagesQueueView {
private:
    std::reference_wrapper<std::queue<std::shared_ptr<std::string>>> messages_queue_;

public:
    /**
     * @brief Constructs view for given messages queue
     *
     * @param messages_queue RPTL messages queue to provide access
     */
    explicit MessagesQueueView(std::queue<std::shared_ptr<std::string>>& messages_queue);

    /**
     * @brief Checks if every message has been consumed or not.
     *
     * @returns `true` if queue isn't empty, `false` if it is
     */
    bool hasNext() const;

    /**
     * @brief Retrieves next RPTL message and removes it from queue
     *
     * @returns Next RPTL message
     *
     *
     */
    std::shared_ptr<std::string> next();
};


}


#endif //RPT_MINIGAMES_SERVER_MESSAGESQUEUEVIEW_HPP
