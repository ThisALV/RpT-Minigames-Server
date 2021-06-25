#include <RpT-Core/ServiceEventRequestProtocol.hpp>

#include <algorithm>
#include <cassert>


namespace RpT::Core {


ServiceEventRequestProtocol::ServiceRequestCommandParser::ServiceRequestCommandParser(
        const std::string_view sr_command) : Utils::TextProtocolParser { sr_command, 3 } {

    try {
        const std::string ruid_copy { getParsedWord(1) }; // Required for conversion to unsigned integer

        // `unsigned long long` return type, which is ALWAYS 64 bits large
        // See: https://en.cppreference.com/w/cpp/language/types
        parsed_ruid_ = std::stoull(ruid_copy);
    } catch (const std::invalid_argument&) { // If parsed actor UID argument isn't a valid unsigned integer
        throw BadServiceRequest { "Request UID must be an unsigned integer of 64 bits" };
    }
}

bool ServiceEventRequestProtocol::ServiceRequestCommandParser::isValidRequest() const {
    return getParsedWord(0) == REQUEST_PREFIX;
}

std::uint64_t ServiceEventRequestProtocol::ServiceRequestCommandParser::ruid() const {
    return parsed_ruid_;
}

std::string_view ServiceEventRequestProtocol::ServiceRequestCommandParser::intendedServiceName() const {
    return getParsedWord(2);
}

std::string_view ServiceEventRequestProtocol::ServiceRequestCommandParser::commandData() const {
    return unparsedWords();
}


ServiceEventRequestProtocol::ServiceEventRequestProtocol(
        const std::initializer_list<std::reference_wrapper<Service>>& services,
        Utils::LoggingContext& logging_context) :

        logger_ { "SER-Protocol", logging_context } {

    // Each given service reference must be registered as running service
    for (const auto service_ref : services) {
        const std::string_view service_name { service_ref.get().name() };

        if (isRegistered(service_name)) // Service name must be unique among running services
            throw ServiceNameAlreadyRegistered { service_name };

        const auto service_registration_result { running_services_.insert({ service_name, service_ref }) };
        // Must be sure that service has been successfully registered, this is why insertion result is saved
        assert(service_registration_result.second);

        logger_.debug("Registered service {}.", service_name);
    }
}


bool ServiceEventRequestProtocol::isRegistered(const std::string_view service) const {
    return running_services_.count(service) == 1; // Returns if service name is present among running services
}

std::string ServiceEventRequestProtocol::handleServiceRequest(const std::uint64_t actor,
                                                              const std::string_view service_request) {

    logger_.trace("Handling SR command from \"{}\": {}", actor, service_request);

    std::uint64_t request_uid;
    std::string_view intended_service_name;
    std::string_view command_data;
    try { // Tries to parse received SR command
        const ServiceRequestCommandParser sr_command_parser { service_request }; // Parsing

        if (!sr_command_parser.isValidRequest()) // Checks for SER command prefix, must be REQUEST for a SR command
            throw InvalidRequestFormat { service_request, "Expected SER command prefix \"REQUEST\" for SR command" };

        // Set given parameters to corresponding parsed (or not) arguments
        intended_service_name = sr_command_parser.intendedServiceName();
        request_uid = sr_command_parser.ruid();
        command_data = sr_command_parser.commandData();
    } catch (const Utils::NotEnoughWords& err) { // Catches error if SER command format is invalid
        throw InvalidRequestFormat { service_request, "Expected SER command prefix and request service name" };
    }

    assert(!intended_service_name.empty()); // Service name must be initialized if try statement passed successfully

    // Checks for intended service registration
    if (!isRegistered(intended_service_name))
        throw ServiceNotFound { intended_service_name };

    Service& intended_service { running_services_.at(intended_service_name).get() };

    logger_.trace("SR command successfully parsed, handled by service: {}", intended_service_name);

    // SRR beginning is always `RESPONSE <RUID>`
    const std::string sr_response_prefix {
        std::string { RESPONSE_PREFIX } + ' ' + std::to_string(request_uid) + ' '
    };

    // Try to handle SR command, catching errors occurring inside handlers
    try {
        // Handles SR command and saves result
        const Utils::HandlingResult command_result { intended_service.handleRequestCommand(actor, command_data) };

        if (command_result) // If command was successfully handled, must retrieves OK Service Request Response
            return sr_response_prefix + "OK";
        else // Else, command failed and KO response must be retrieved
            return sr_response_prefix + "KO " + command_result.errorMessage();
    } catch (const std::exception& err) { // If exception is thrown by intended service
        logger_.error("Service \"{}\" failed to handle command: {}" , intended_service_name, err.what());

        // Retrieves error Service Request Response with given caught message `RESPONSE <RUID> KO <ERR_MSG>`
        return sr_response_prefix + "KO " + err.what();
    }
}

std::optional<ServiceEvent> ServiceEventRequestProtocol::pollServiceEvent() {
    std::optional<ServiceEvent> next_event; // Event to poll is first uninitialized

    // Will be set to Service which has the lowest ID if any of them emitted a an event
    Service* latest_event_emitter { nullptr };

    std::size_t lowest_event_id;
    for (auto registered_service : running_services_) {
        Service& service { registered_service.second.get() };

        const std::optional<std::size_t> next_event_id { service.checkEvent() };

        if (next_event_id.has_value()) { // Skip service if has an empty events queue
            logger_.trace("Service {} last event ID: {}", service.name(), *next_event_id);

            // If there isn't any event to poll or current was triggered first...
            if (!latest_event_emitter || next_event_id < lowest_event_id) {
                // ...then set new event emitter, and update lowest ID
                latest_event_emitter = &service;
                lowest_event_id = *next_event_id;
            }
        } else {
            logger_.trace("Service {} hasn't any event.", service.name());
        }
    }

    if (latest_event_emitter) { // If there is any emitted event, format it and move it into polled event
        const std::string service_name_copy { latest_event_emitter->name() }; // Required for concatenation
        // Latest event is popped from queue to be polled as next event
        *next_event = latest_event_emitter->pollEvent();
        // SE event data prefixed with SER command EVENT and Service name to format a full and valid SER command
        next_event->prefixWith(std::string { EVENT_PREFIX } + ' ' + service_name_copy + ' ');

        logger_.trace("Polled event from service {}: {}", latest_event_emitter->name(), next_event->command());
    } else {
        logger_.trace("No event to retrieve");
    }

    return next_event;
}

}