#include <RpT-Network/MessagesQueueView.hpp>


namespace RpT::Network {


MessagesQueueView::MessagesQueueView(std::queue<std::shared_ptr<std::string>>& messages_queue)
: messages_queue_ { messages_queue } {}

bool MessagesQueueView::hasNext() const {
    return !messages_queue_.get().empty();
}

std::shared_ptr<std::string> MessagesQueueView::next() {
    if (!hasNext()) // Checks for queue to have a RPTL message to pop from queue
        throw NoMoreMessage {};

    // Pops message from queue
    std::shared_ptr<std::string> popped_message { messages_queue_.get().front() }; // Saves message
    messages_queue_.get().pop(); // Removes it

    return popped_message;
}


}
