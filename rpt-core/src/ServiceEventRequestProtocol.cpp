#include <RpT-Core/ServiceEventRequestProtocol.hpp>

#include <algorithm>
#include <cassert>


namespace RpT::Core {


std::vector<std::string_view> ServiceEventRequestProtocol::getWordsFor(std::string_view sr_command) {
    // Begin and end constant iterators for SR command string
    const auto cmd_begin { sr_command.cbegin() };
    const auto cmd_end { sr_command.cend() };

    // Starts without any word parsed yet
    std::vector<std::string_view> parsed_words;

    // Starts string parsing from the first word
    auto word_begin { cmd_begin };
    auto word_end { std::find(cmd_begin, cmd_end, ' ') };

    // While entire string hasn't be parsed (while word begin isn't at string end)
    while (word_begin != word_end) {
        // Calculate index for word begin, and next word size
        const long word_begin_i { word_begin - cmd_begin };
        const long word_length { (word_end - word_begin) };

        parsed_words.push_back(sr_command.substr(word_begin_i, word_length )); // Add currently parsed word to words

        word_begin = word_end; // Is the string end reached ?

        if (word_end != cmd_end) { // Is the string end reached ?
            word_begin++; // Then, move word begin after previously found space
            word_end = std::find(word_begin, cmd_end, ' '); // And find next word end starting from next word begin
        }
    }

    return parsed_words;
}

ServiceEventRequestProtocol::ServiceEventRequestProtocol(
        const std::initializer_list<std::reference_wrapper<Service>>& services,
        Utils::LoggingContext& logging_context) :

        logger_ { "SER-Protocol", logging_context } {

    // Each given service reference must be registered as running service
    for (const auto service_ref : services) {
        const std::string_view service_name { service_ref.get().name() };

        if (isRegistered(service_name)) // Service name must be unique among running services
            throw ServiceNameAlreadyRegistered { service_ref.get().name() };

        const auto service_registration_result { running_services_.insert({ service_name, service_ref }) };
        // Must be sure that service has been successfully registered, this is why insertion result is saved
        assert(service_registration_result.second);

        logger_.debug("Registered service {}.", service_name);
    }
}

bool ServiceEventRequestProtocol::isRegistered(const std::string_view service) const {
    return running_services_.count(service) == 1; // Returns if service name is present among running services
}

bool ServiceEventRequestProtocol::handleServiceRequest(const std::string_view actor,
                                                       const std::string_view service_request) {

    logger_.trace("Handling SR command from \"{}\": {}", actor, service_request);

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

    // Execute given command contained in command data arguments, and returns service execution result
    const std::vector<std::string_view> command_data_arguments { // Extract SR command words which are command arguments
        sr_command_words.cbegin() + 2, sr_command_words.cend()
    };

    logger_.trace("SR command successfully parsed, handled by service: {}", intended_service_name);

    return intended_service.handleRequestCommand(actor, command_data_arguments);
}


std::optional<std::string> ServiceEventRequestProtocol::pollServiceEvent() {
    std::optional<std::string> next_event; // Event to poll is first uninitialized

    Service* newest_event_emitter { nullptr }; // Service with event which has the lowest ID
    std::size_t lowest_event_id;
    for (auto registered_service : running_services_) {
        Service& service { registered_service.second.get() };

        const std::optional<std::size_t> next_event_id { service.checkEvent() };

        if (next_event_id.has_value()) { // Skip service if has an empty events queue
            logger_.trace("Service {} last event ID: {}", service.name(), *next_event_id);

            // If there isn't any event to poll or current was triggered first...
            if (!newest_event_emitter || next_event_id < lowest_event_id) {
                // ...then set new event emitter, and update lowest ID
                newest_event_emitter = &service;
                lowest_event_id = *next_event_id;
            }
        } else {
            logger_.trace("Service {} hasn't any event.", service.name());
        }
    }

    if (newest_event_emitter) { // If there is any emitted event, format it and move it into polled event
        const std::string service_name_copy { newest_event_emitter->name() }; // Required for concatenation

        next_event = std::string { EVENT_PREFIX } + ' ' + service_name_copy + ' ' + newest_event_emitter->pollEvent();

        logger_.trace("Polled event from service {}: {}", newest_event_emitter->name(), *next_event);
    } else {
        logger_.trace("No event to retrieve.");
    }

    return next_event;
}


}