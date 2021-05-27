#include <RpT-Core/Executor.hpp>

#include <RpT-Core/ServiceEventRequestProtocol.hpp>


namespace RpT::Core {


Executor::InputEventVisitor::InputEventVisitor(Executor& running_instance)
: instance_ { running_instance }, logger_ { instance_.logger_ }, ser_protocol_ { nullptr } {}

bool Executor::InputEventVisitor::isConfigured() const {
    return static_cast<bool>(ser_protocol_);
}

void Executor::InputEventVisitor::markReady(ServiceEventRequestProtocol& ser_protocol) {
    ser_protocol_ = &ser_protocol;
}

void Executor::InputEventVisitor::operator()(NoneEvent event) const {
    logger_.trace("Null event");

    // Input event will not be used anymore, can be moved to user callback
    userNoneHandler(std::move(event));
}

void Executor::InputEventVisitor::operator()(ServiceRequestEvent event) const {
    logger_.debug("SR command received from player {}", event.actor());

    const std::uint64_t actor_uid { event.actor() };
    try { // Tries to parse SR command
        // Give SR command to parse and execute by SER Protocol
        const std::string sr_command_response {
                ser_protocol_->handleServiceRequest(event.actor(), event.serviceRequest())
        };

        // Replies to actor with command handling result
        instance_.io_interface_.replyTo(actor_uid, sr_command_response);
    } catch (const BadServiceRequest& err) { // If command cannot be parsed, SRR cannot be sent, pipeline broken
        // It is no longer possible to sync SR with actor as RUID might be wrong, closing pipeline with thrown
        // exception message
        instance_.io_interface_.closePipelineWith(
                actor_uid, Utils::HandlingResult { err.what() });

        logger_.error("SER Protocol broken for actor {}: {}. Closing pipeline...", actor_uid, err.what());
    }

    userServiceRequestHandler(std::move(event));
}

void Executor::InputEventVisitor::operator()(TimerEvent event) const {
    const std::uint64_t timer_token { event.token() }; // Token for timer which timed out
    logger_.trace("Triggering timer {}", timer_token);

    const auto timer_to_trigger { instance_.pending_timers_.find(timer_token) }; // Retrieves timer by its token
    assert(timer_to_trigger != instance_.pending_timers_.cend()); // Must be sure timer actually exists

    timer_to_trigger->second.get().trigger(); // Trigger timed out timer
    instance_.pending_timers_.erase(timer_to_trigger); // Timer is no longer pending, removes it from registry

    userTimerHandler(std::move(event));
}

void Executor::InputEventVisitor::operator()(JoinedEvent event) const {
    logger_.info("Player \"{}\" joined server as actor {}", event.playerName(), event.actor());

    userJoinedHandler(std::move(event));
}

void Executor::InputEventVisitor::operator()(LeftEvent event) const {
    logger_.info("Actor {} left server", event.actor());

    userLeftHandler(std::move(event));
}


Executor::Executor(InputOutputInterface& io_interface, Utils::LoggingContext& logger_context) :
    logger_context_ { logger_context },
    logger_ { "Executor", logger_context_ },
    io_interface_ { io_interface },
    events_visitor_ { *this },
    loop_routine_ { []() {} } /* default behavior of loop's routine is to do nothing */ {}

void Executor::make(std::function<void()> loop_routine) {
    if (events_visitor_.isConfigured()) // Configured underlying visitor means run() has been called once
        throw BadExecutorMode {};

    loop_routine_ = std::move(loop_routine);
}

bool Executor::run(std::initializer_list<std::reference_wrapper<Service>> services) {
    // Protocol initialization with created services
    ServiceEventRequestProtocol ser_protocol { services, logger_context_ };
    // run() called, ends configuration mode
    events_visitor_.markReady(ser_protocol);

    logger_.info("Starts main loop.");

    try { // Any errors occurring during main loop execution will
        while (!io_interface_.closed()) { // Main loop must run as long as inputs and outputs with players can occur
            // Blocking until receiving external event to handle (timer, data packet, etc.)
            const AnyInputEvent input_event { io_interface_.waitForInput() };

            boost::apply_visitor(events_visitor_, input_event);

            // Handlers on services might have been called, checks for waiting timers
            for (auto service : services) {
                for (auto waiting_timer : service.get().getWaitingTimers()) { // For each Ready timer in each service
                    const std::size_t timer_token { waiting_timer.get().token() };
                    const auto insert_result { pending_timers_.insert({ timer_token, waiting_timer }) };

                    assert(insert_result.second); // Checks for token and timer ref insertion into registry

                    waiting_timer.get().onNextClear([this, timer_token]() {
                        // If timer is set to Disabled without having been triggered, then executor didn't remove its
                        // entry from pending timers dictionary, it must be done otherwise it cannot be enabled used

                        pending_timers_.erase(timer_token); // Will erase if not removed by begin triggered before
                    });
                    io_interface_.beginTimer(waiting_timer.get()); // Will be set to pending by implementation
                }
            }

            // Calls routine for operations which must be performed or checked for iteration no matter which input
            // event were emitted

            logger_.trace("Entering loop routine...");
            loop_routine_();
            logger_.trace("Loop routine done.");

            // After all required handlers and operations have been done, events emitted by services should also be
            // handled in the order they appeared so clients can be synced with server services state

            logger_.debug("Polling service events...");

            std::optional<std::string> next_svc_event { ser_protocol.pollServiceEvent() }; // Read first event
            while (next_svc_event) { // Then while next event actually exists, handles it
                logger_.debug("Output event: {}", *next_svc_event);
                io_interface_.outputEvent(*next_svc_event); // Sent across actors

                next_svc_event = ser_protocol.pollServiceEvent(); // Read next event
            }

            logger_.debug("Events polled.");
        }

        logger_.info("Stopped.");

        return true;
    } catch (const std::exception& err) {
        logger_.error("Runtime error: {}", err.what());

        return false;
    }
}

}
