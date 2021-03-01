#include <RpT-Core/Executor.hpp>

#include <type_traits>


namespace RpT::Core {

Executor::Executor(std::vector<boost::filesystem::path> game_resources_path, std::string game_name,
                   InputOutputInterface& io_interface, Utils::LoggingContext& logger_context) :
    logger_ { "Executor", logger_context },
    io_interface_ { io_interface } {

    logger_.debug("Game name: {}", game_name);

    for (const boost::filesystem::path& resource_path : game_resources_path)
        logger_.debug("Game resources path: {}", resource_path.string());
}

bool Executor::run() {
    logger_.info("Start.");

    try {
        bool running { true };
        while (running) {
            const AnyInputEvent input_event { io_interface_.waitForInput() };

            boost::apply_visitor([&](auto&& event) {
                using EventType = std::decay_t<decltype(event)>;

                if constexpr (std::is_same_v<EventType, StopEvent>) {
                    logger_.info("Stopping server: caught signal {}", event.caughtSignal());
                    running = false;
                } else if constexpr (std::is_same_v<EventType, ServiceRequestEvent>) {
                    logger_.trace("Received SR command: {}", event.serviceRequest());
                } else {
                    throw std::runtime_error { "Unexpected input event" };
                }
            }, input_event);
        }

        logger_.info("Stopped.");

        return true;
    } catch (const std::exception& err) {
        logger_.error("Runtime error: {}", err.what());

        return false;
    }
}

}
