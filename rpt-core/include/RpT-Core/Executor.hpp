#ifndef RPTOGETHER_SERVER_EXECUTOR_HPP
#define RPTOGETHER_SERVER_EXECUTOR_HPP

#include <functional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <RpT-Core/InputOutputInterface.hpp>
#include <RpT-Core/Service.hpp>
#include <RpT-Core/ServiceEventRequestProtocol.hpp>
#include <RpT-Core/Timer.hpp>
#include <RpT-Utils/LoggerView.hpp>

/**
 * @file Executor.hpp
 */


/**
 * @brief Server engine and main loop related code
 */
namespace RpT::Core {


/**
 * @brief Thrown by `Executor` methods if called while instance run already finished once
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class BadExecutorMode : public std::logic_error {
public:
    /**
     * @brief Constructs error with predefined error message
     */
    BadExecutorMode() : std::logic_error { "Executor run already finished once" } {}
};


/**
 * @brief RpT main loop executor
 *
 * Runs main loop for RpT for each received input event from given `InputOutputInterface` implementation.
 *
 * When input event of a certain type is received, event type default handler is executed first, then user-provided
 * handler is called with input event as only argument.
 *
 * After event specific handlers have been called, %Executor will check for Timer instances in services which are
 * into Ready state (waiting for countdown). All of them will be registered with their token inside pending timers
 * registry, then `InputOutputInterface` implementation will do its job and begin waiting timers countdown.
 *
 * After required timers countdown have begun, routine handler, which is user-provided, is called to
 * make application progress with updated Services state.
 *
 * Finally, all events emitted by Services are sent to actors. Then, if IO interface is still open, next input event
 * is waited for.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class Executor {
private:
    /// Routine taking given input event type as only argument, default-constructible to ignore event
    template<typename InputEventT>
    class InputEventHandler {
    public: // Typedef is used all along class declaration, must be defined first
        using Routine = std::function<void(InputEventT)>;

    private:
        Routine handler_routine_;

    public:
        /// Pass execution, ignore input event
        InputEventHandler() : handler_routine_ { [](InputEventT) {} } {}

        /// Call given routine to handle input event
        InputEventHandler(Routine handler_routine) : handler_routine_ { std::move(handler_routine) } {} // NOLINT
        // (google-explicit-constructor)

        void operator()(InputEventT next_input_event) const {
            assert(handler_routine_); // Routine must be initialized <=> callable

            handler_routine_(std::move(next_input_event));
        }
    };

    /// Handles a received input event depending on its type, calling first Executor-provided handler, then
    /// user-provided handler
    class InputEventVisitor {
    private:
        // Required to access IO interface instance
        Executor& instance_;
        // Provided by Executor
        const Utils::LoggerView logger_;
        // Executor::run() scoped SER Protocol instance, not available during configuration
        ServiceEventRequestProtocol* ser_protocol_;

        /*
         * Default constructible routines
         */

        InputEventHandler<NoneEvent> userNoneHandler;
        InputEventHandler<ServiceRequestEvent> userServiceRequestHandler;
        InputEventHandler<TimerEvent> userTimerHandler;
        InputEventHandler<JoinedEvent> userJoinedHandler;
        InputEventHandler<LeftEvent> userLeftHandler;

    public:
        /// Constructs visitor in configuration mode (SER Protocol instance not initialized yet)
        explicit InputEventVisitor(Executor& running_instance);

        /// Checks if events visitor is ready for executor to run, or if it is still into configuration mode
        /// Defines if Executor current instance is running or not, as visitor must be marked as configured as soon
        /// as main loop start and SER Protocol instance is provided
        bool isConfigured() const;

        /// Checks for each available input event type and updates appropriate handler
        template<typename InputEventT>
        void configure(std::function<void(InputEventT)> event_handler) {
            InputEventHandler<InputEventT>* updatedHandler; // Appropriate handler will be selected by pointer

            // Checks for each supported type
            if constexpr (std::is_same_v<NoneEvent, InputEventT>)
                updatedHandler = &userNoneHandler;
            if constexpr (std::is_same_v<ServiceRequestEvent, InputEventT>)
                updatedHandler = &userServiceRequestHandler;
            if constexpr (std::is_same_v<TimerEvent, InputEventT>)
                updatedHandler = &userTimerHandler;
            if constexpr (std::is_same_v<JoinedEvent, InputEventT>)
                updatedHandler = &userJoinedHandler;
            if constexpr (std::is_same_v<LeftEvent, InputEventT>)
                updatedHandler = &userLeftHandler;
            else // Other are unsupported
                throw std::logic_error { "Unsupported InputEvenT" };

            *updatedHandler = std::move(event_handler);
        }

        /// Finish configuration mode, providing run() scoped SER Protocol instance
        void markReady(ServiceEventRequestProtocol& ser_protocol);

        /// No default behavior
        void operator()(NoneEvent event) const;
        /// Default behavior: handles SR command, and send back response to actor who emitted event
        void operator()(ServiceRequestEvent event) const;
        /// Default behavior: marks timer with given token as triggered ans removes it from pending timers registry
        void operator()(TimerEvent event) const;
        /// No default behavior
        void operator()(JoinedEvent event) const;
        /// No default behavior
        void operator()(LeftEvent event) const;
    };

    Utils::LoggingContext& logger_context_;
    const Utils::LoggerView logger_;
    InputOutputInterface& io_interface_;
    InputEventVisitor events_visitor_;
    std::function<void()> loop_routine_;
    std::unordered_map<std::uint64_t, std::reference_wrapper<Timer>> pending_timers_;

public:
    /**
     * @brief Constructs executor with user-defined IO interface implementation, no user-provided input events
     * handler and no user-provided loop's routine
     *
     * @param io_interface Backend for input and output based main loop events handling
     * @param logger_context Whole server logging context
     */
    Executor(InputOutputInterface& io_interface, Utils::LoggingContext& logger_context);

    // Entity class semantic :

    Executor(const Executor&) = delete;
    Executor& operator=(const Executor&) = delete;

    // this pointer used during object lifetime, not movable
    Executor(Executor&&) = delete;
    Executor& operator=(Executor&&) = delete;

    bool operator==(const Executor&) const = delete;

    /**
     * @brief Setup loop's specific input event handler
     *
     * @note User-provided handler does NOT override default behavior, it rather is an unique additional handler for
     * given event type.
     *
     * @tparam InputEventT type to expect for given handler
     *
     * @param event_handler Called for each input event of type `InputEventT`
     *
     * @throws BadExecutorMode if `run()` has already been called
     */
    template<typename InputEventT>
    void handle(std::function<void(InputEventT)> event_handler) {
        static_assert(std::is_base_of_v<InputEvent, InputEventT>); // Checks for given type to be an input event type

        if (events_visitor_.isConfigured()) // Configured underlying visitor means run() has been called once
            throw BadExecutorMode {};

        events_visitor_.configure(std::move(event_handler)); // Configures visitor before running executor
    }

    /**
     * @brief Setup loop's routine
     *
     * @param loop_routine Routine to call at the end for each main loop iteration
     *
     * @throws BadExecutorMode if `run()` has already been called
     */
    void make(std::function<void()> loop_routine);

    /**
     * @brief Starts executor main loop
     *
     * Main loop will be ran IO interface implementation is closed. See IO interface implementation doc for closing
     * conditions.
     *
     * @param services Access to services which will be registered inside `run()` local `ServiceEventRequestProtocol`
     * instance.
     *
     * @returns `true` if properly shutdown, `false` if an error occurred
     */
    bool run(std::initializer_list<std::reference_wrapper<Service>> services);
};


}

#endif //RPTOGETHER_SERVER_EXECUTOR_HPP
