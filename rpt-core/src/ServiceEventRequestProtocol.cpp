#include <RpT-Core/ServiceEventRequestProtocol.hpp>

#include <algorithm>
#include <cassert>


namespace RpT::Core {


EventsPipeline::EventsPipeline() : destination_events_queue_ { nullptr } {}

EventsPipeline::EventsPipeline(std::queue<std::string>& active_protocol_events_queue) :
destination_events_queue_ { &active_protocol_events_queue } {}

void EventsPipeline::pushServiceEvent(std::string se_command) {
    // Pipeline must refers to valid destination events queue, not nullptr
    if (!destination_events_queue_)
        throw PipelineNotBound {};

    destination_events_queue_->push(std::move(se_command)); // Given SE command is moved to queue
}

void Service::emitEventsFor(EventsPipeline events_queue_pipeline) {
    events_queue_ = events_queue_pipeline;
}

std::vector<std::string_view> ServiceEventRequestProtocol::getWordsFor(std::string_view sr_command) {
    std::vector<std::string_view> parsed_words;

    // Begin and end constant iterators for SR command string
    const auto cmd_begin { sr_command.cbegin() };
    const auto cmd_end { sr_command.cend() };

    // Starts without any word parsed yet
    std::vector<std::string_view> words;

    // Starts string parsing from the beginning
    auto word_begin { cmd_begin };
    auto word_end { cmd_begin };

    // Until end of string is reached
    while (word_end != cmd_end) {
        word_end = std::find(word_begin + 1, cmd_end, ' '); // Find next ' ' char into unparsed command string

        // Word begin and end index calculations
        const long word_begin_i { word_begin - cmd_begin };
        const long word_end_i { word_end - cmd_end };

        // Indexes must be positives, else it means either word_begin or word_end is out of range
        assert(word_begin_i >= 0 && word_end_i >= 0);

        // Adds parsed word from its first char to next space excluded
        words.push_back(sr_command.substr(word_begin_i, word_end_i));

        word_begin = word_end ; // Next search should begin after current word
    }

    return words;
}

ServiceEventRequestProtocol::ServiceEventRequestProtocol(
        const std::initializer_list<std::reference_wrapper<Service>>& services) {

    // Pipeline will be distributed among registered services
    const EventsPipeline instance_pipeline { services_events_queue_ };

    // Each given service reference must be registered as running service
    for (const auto service_ref : services) {
        const std::string_view service_name { service_ref.get().name() };

        if (isRegistered(service_name)) // Service name must be unique among running services
            throw ServiceNameAlreadyRegistered { service_ref.get().name() };

        const auto service_registration_result { running_services_.insert({ service_name, service_ref }) };
        // Must be sure that service has been successfully registered, this is why insertion result is saved
        assert(service_registration_result.second);

        // Service must be able to emit events
        service_ref.get().emitEventsFor(instance_pipeline);
    }
}

bool ServiceEventRequestProtocol::isRegistered(const std::string_view service) const {
    return running_services_.count(service) == 1; // Returns if service name is present among running services
}

bool ServiceEventRequestProtocol::handleServiceRequest(const std::string_view actor,
                                                       const std::string_view service_request) {

    // Get all parsed words into SR command
    const std::vector<std::string_view> sr_command_words { getWordsFor(service_request) };

    // At least two words must be present for SER command prefix and service of request
    if (sr_command_words.size() < 2)
        throw BadServiceRequest { service_request, "SER command prefix and intended service must be given" };

    // First word must corresponds to SR command prefix
    if (sr_command_words.at(0) != REQUEST_PREFIX)
        throw BadServiceRequest { service_request, "SER command prefix must be REQUEST" };

    const std::string_view intended_service_name { sr_command_words.at(1) }; // Retrieve service name from 2nd word

    // Service must be registered
    if (!isRegistered(intended_service_name))
        throw ServiceNotFound { intended_service_name };

    Service& intended_service { running_services_.at(intended_service_name).get() };

    // Execute given command, and returns service execution result
    return intended_service.handleRequestCommand(actor, service_request);
}


std::optional<std::string> ServiceEventRequestProtocol::pollServiceEvent() {
    // Event to poll is uninitialized
    std::optional<std::string> next_event;

    if (!services_events_queue_.empty()) { // If there is at least one event to poll
        next_event = std::move(services_events_queue_.front()); // Then event is moved to polled event
        services_events_queue_.pop(); // And moved event is popped from queue
    }

    return next_event;
}


}