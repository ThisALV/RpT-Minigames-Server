#ifndef RPTOGETHER_SERVER_BEASTWEBSOCKETBACKENDBASE_INL
#define RPTOGETHER_SERVER_BEASTWEBSOCKETBACKENDBASE_INL

#include <chrono>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <utility>
#include <boost/asio/signal_set.hpp>
#include <boost/beast.hpp>
#include <RpT-Config/Config.hpp>
#include <RpT-Network/NetworkBackend.hpp>
#include <RpT-Utils/LoggerView.hpp>

/**
 * @file BeastWebsocketBackendBase.inl
 */


/**
 * @brief Contains implementations for `RpT::Core::InputOutputInterface`
 */
namespace RpT::Network {


/**
 * @brief IO interface implementation using websockets protocol over user-defined TCP stream
 *
 * This `NetworkBackend` implementation provides client and messaging features by connecting streams. Each Websocket
 * stream is owned by one and only one client token. That stream will be used for asynchronous message sending and
 * message receiving, so clients and server state will be synced.
 *
 * As all IO operations complete asynchronously, any error result in WS stream to be closed next time `waitForEvent()
 * ` is called.
 *
 * `BeastWebsocketBackendBase` subclass responsibility is to establish any valid Websocket stream using given
 * `TcpStream` as underlying stream from incoming established TCP connection (using TCP socket).
 *
 * @tparam TcpStream Underlying tcp stream type
 *
 * @author ThisALV, https://github.com/ThisALV
 */
template<typename TcpStream>
class BeastWebsocketBackendBase : public NetworkBackend {
protected: // Must be defined early so it can be used all along the class definitions
    /// Top-level WS stream used to communicate with clients with RPTL protocol, depends on underlying `TcpStream`
    using WebsocketStream = boost::beast::websocket::stream<TcpStream>;

private:
    /// Handles message sending result to given client token
    class SentMessageHandler {
    private:
        BeastWebsocketBackendBase& protocol_instance_;
        const std::uint64_t client_token_;

    public:
        /**
         * @brief Constructs handler for message sent to given actor from given RPTL protocol backend instance
         *
         * @param protocol_instance Instance which sent message to client
         * @param client_token Client who must receive instance message
         */
        SentMessageHandler(BeastWebsocketBackendBase& protocol_instance, const std::uint64_t client_token)
        : protocol_instance_ { protocol_instance }, client_token_ { client_token } {}

        /// Makes handler callable object
        void operator()(const boost::system::error_code& err, std::size_t) {
            // No matter if async_write succeeded or not, RPTL message should not be handled twice, so pop it from queue
            protocol_instance_.merged_remaining_messages_.pop();

            // Handles error with connection closure, as specified by RPTL protocol
            // An error for one RPTL message must NOT crash other client connections, so error is fatal for client only
            if (err && err != boost::asio::error::operation_aborted) { // Ignores if server stopped
                std::string error_message { err.message() }; // Retrieves Asio error code associated message

                protocol_instance_.logger_.error("Unable to send message to client {}: {}", client_token_, error_message);

                // As RPTL protocol requires, connection if closed if any error occurred, using specific error message
                // Retrieved error message can be moved inside message sent handler result
                protocol_instance_.killClient(client_token_, Utils::HandlingResult { error_message });
            }

            // If no error occurred, checks for messages queue and send next message recursively if any
            if (!protocol_instance_.merged_remaining_messages_.empty())
                protocol_instance_.sendRemainingMessages();
        }
    };

    /// Visits given input event triggered by client RPTL message, handlings depends on input event type
    class TriggeredInputEventVisitor {
    private:
        BeastWebsocketBackendBase& protocol_instance_;
        const std::uint64_t sender_token_;

    public:
        /**
         * @brief Constructs input event visitor acting on given backend instance triggered by message sent from
         * given client
         *
         * @param protocol_instance Backend the triggered event will be acting on
         * @param sender_token Token for client which sent message that triggered visited event
         */
        TriggeredInputEventVisitor(BeastWebsocketBackendBase& protocol_instance, const std::uint64_t sender_token)
        : protocol_instance_ { protocol_instance }, sender_token_ { sender_token } {}

        /// Makes callable object
        /// Template method used instead of one method for each type as only few event types are actually checked
        template<typename InputEventT>
        void operator()(InputEventT&&) {
            using SimplifiedInputEventT = std::decay_t<InputEventT>;

            // If logout message was sent, client is actor is unregistered and must be removed
            // If handshaking was done, a new actor was registered and clients should be synced about that
            if constexpr (std::is_same_v<Core::LeftEvent, SimplifiedInputEventT>) {
                // As LeftEvent was triggered by logout command, we know no error occurred
                protocol_instance_.killClient(sender_token_);
            }
        }
    };

    static std::vector<int> getCaughtSignals() {
        std::vector<int> caught_signals { SIGTERM }; // SIGTERM is always caught and always exist
        caught_signals.reserve(3); // At least 3 caught Posix signals: SIGINT, SIGTERM and SIGHUP

#ifdef NDEBUG
        caught_signals.push_back(SIGINT); // SIGINT used by GDB for debugging
#endif

#if RPT_RUNTIME_PLATFORM == RPT_RUNTIME_UNIX
        caught_signals.push_back(SIGHUP); // SIGHUP only available for Unix runtime platform
#endif

        return caught_signals;
    }

    /// RPTL message with client destination token so it can be saved inside global protocol queue
    struct QueuedMessage {
        std::uint64_t client;
        std::shared_ptr<std::string> rptlMessage;
    };

    // Provides logging features
    Utils::LoggerView logger_;

    // Websocket stream using given TCP stream for each client token
    std::unordered_map<std::uint64_t, WebsocketStream> clients_stream_;
    // Provides running context for all async IO operations
    boost::asio::io_context async_io_context_;
    // Posix signals handling to stop server
    boost::asio::signal_set stop_signals_handling_;
    // Provides ready TCP connection to open WS stream from
    boost::asio::ip::tcp::acceptor tcp_acceptor_;
    // Keep total clients count so an unique token can be given to each new client
    std::uint64_t tokens_count_;
    // As only one async_write operation at once is allowed, every queue should be synced by merging them into one
    // single queue
    std::queue<QueuedMessage> merged_remaining_messages_;

    /**
     * @brief Starts listening for incoming client TCP connection on local endpoint
     */
    void start() {
        logger_.info("Open IO interface on local port {}.", tcp_acceptor_.local_endpoint().port());

        waitNextClient();
    }

    /**
     * @brief Sends next queued message for given client, async handler will recursively send next message when
     * operation will complete
     *
     * @param client_token Token for queue to send messages from
     * @param remaining_messages Pop-only queue for PRTL messages to send once next message have been sent successfully
     */
    void sendRemainingMessages() {
        // Using shared_ptr stored inside queue, data will be valid during async handler execution
        const QueuedMessage message_owner { merged_remaining_messages_.front() };
        // Buffer read by Asio to send message, data must be valid until handler call finished, so const buffer data
        // is owned by RPTL message shared pointer, alive until queued message is popped from queue
        const boost::asio::const_buffer message_buffer {
            message_owner.rptlMessage->data(), message_owner.rptlMessage->size()
        };

        clients_stream_.at(message_owner.client).async_write(
                message_buffer, SentMessageHandler { *this, message_owner.client });
    }

    /// Accepts next incoming TCP client connection, then wait for next client again
    void waitNextClient() {
        logger_.trace("Waiting for new TCP connection...");

        tcp_acceptor_.async_accept([this](
                const boost::system::error_code& err, boost::asio::ip::tcp::socket new_client_connection) {

            if (err == boost::asio::error::operation_aborted) // Ignores if server execution stopped
                return;

            if (err) {
                logger_.error("Unable to accept TCP from {}: {}", endpointFor(new_client_connection), err.message());
            } else {
                logger_.debug("Accepted TCP connection from {}", endpointFor(new_client_connection));

                // Tries to asynchronously open WS stream with TCP connection established from new client
                openWebsocketStream(std::move(new_client_connection));
            }

            waitNextClient(); // In any case, server must be waiting again for the next TCP connection
        });
    }

    /**
     * @brief Receives and handles incoming message from given client
     *
     * @param client_token Token for client to be listening for
     */
    void listenMessageFrom(const std::uint64_t client_token) {
        logger_.trace("Listening next message from {}...", client_token);

        // A new buffer has to be initialized for each received/sent message as multiple messages could be
        // sent/received before handlers call
        const auto read_buffer { std::make_shared<boost::beast::flat_buffer>() };
        // Buffer should lives for both async_read and callback handler operations, so shared pointer is used

        clients_stream_.at(client_token).async_read(*read_buffer, [this, read_buffer, client_token](
                const boost::system::error_code& err, const std::size_t) {

            if (err) {
                if (err == boost::asio::error::operation_aborted) // Ignores if server stopped
                    return;

                Utils::HandlingResult message_handling_result; // No error for now
                if (err == boost::beast::websocket::error::closed) { // Client sent a close frame
                    logger_.info("Websocket close frame from client {}", client_token);
                } else { // Ignores if server stopped
                    const std::string error_message { err.message() };

                    logger_.error("Failed to receive message from client {}: {}", client_token, error_message);
                    // Error occurred, sets correct message handling result with given error message
                    message_handling_result = Utils::HandlingResult { error_message };
                }

                // In any case, an error means that client must NOT be listened anymore
                killClient(client_token, message_handling_result);

                return;
            }

            // Const readable for received message
            const boost::asio::const_buffer readonly_buffer { read_buffer->cdata() };

            // Reinterpret generic void pointer to cstring with buffer-defined message length
            std::string rptl_message { reinterpret_cast<const char*>(readonly_buffer.data()), readonly_buffer.size() };

            try {
                Core::AnyInputEvent client_triggered_event { handleMessage(client_token, rptl_message) };

                // Visits triggered event checking for type
                boost::apply_visitor(TriggeredInputEventVisitor { *this, client_token }, client_triggered_event);

                pushInputEvent(std::move(client_triggered_event)); // Moves triggered event into queue
                listenMessageFrom(client_token); // Then listens next message from current client
            } catch (const std::exception& err) { // Any error in message handling results into client disconnection
                logger_.error("During {} message handling: {}", client_token, err.what());

                // Client will be disconnect for thrown error reason
                killClient(client_token, Utils::HandlingResult { err.what() });
            }
        });
    }

    /**
     * @brief Removes and closes Websocket and underlying streams for given killed client
     *
     * @param client_token Token for killed client to closes connection with
     */
    void closeStream(const std::uint64_t client_token) {
        // Retrieves stream closure reason to determine Websocket close frame close code
        const Utils::HandlingResult& disconnection_reason { disconnectionReason(client_token) };

        // By default, set to normal close code
        boost::beast::websocket::close_reason websocket_close_reason { boost::beast::websocket::normal };

        // If any error occurred for disconnection to happen, then set error code with custom message, for close
        // frame and interrupt command message
        if (!disconnection_reason) {
            const std::string& error_message { disconnection_reason.errorMessage() };

            websocket_close_reason.code = boost::beast::websocket::close_code::internal_error;
            websocket_close_reason.reason = error_message;
        }

        removeClient(client_token); // Once disconnection reason has been sent to client, it can be removed

        // Moves client stream entry as it will be closed and no more operation should be performed on
        // Use of shared_ptr because stream must not be destroyed before Websocket closure was handled
        const auto dead_client_stream {
            std::make_shared<WebsocketStream>(std::move(clients_stream_.at(client_token)))
        };

        const std::size_t removed_streams_count { clients_stream_.erase(client_token) };
        // Must be sure that exactly ony client stream and messages queue entry have been removed
        assert(removed_streams_count == 1);

        // Does and handles Websocket closure for dead client
        dead_client_stream->async_close(websocket_close_reason, [this, dead_client_stream, client_token](
                const boost::system::error_code& err) {

            // If for any reason clean Websocket closure failed, TCP connection will be closed anyways, but a warning
            // message must be logged
            if (err)
                logger_.warn("Unclean disconnection with client {}: {}", client_token, err.message());
        });
    }

protected:
    /**
     * @brief Tries to get string representation for TCP socket endpoint, offering safe remote endpoints logging
     *
     * @param client_connection Client TCP to try go retrieve endpoint from
     *
     * @return Socket remote endpoint string if successful, `"UNKNOWN"` otherwise
     */
    static std::string endpointFor(const boost::asio::ip::tcp::socket& client_connection) {
        try {
            std::ostringstream endpoint_output;
            endpoint_output << client_connection.remote_endpoint(); // remote_endpoint() may fail for some reasons

            return endpoint_output.str();
        } catch (const boost::system::system_error&) { // If it fails, returns fallback string representation
            return "UNKNOWN";
        }
    }

    /**
     * @brief Provides class logging features
     *
     * @returns A copy for member logger view
     */
    Utils::LoggerView getLogger() const {
        return logger_;
    }

    /**
     * @brief Must asynchronously open Websocket stream using `addClientStream()` from given established TCP connection
     *
     * @param new_client_connection TCP connection ready to handshake into upper protocols layer
     */
    virtual void openWebsocketStream(boost::asio::ip::tcp::socket new_client_connection) = 0;


    /**
     * @brief Inserts new client using server-defined token and given Websocket stream, should be called by
     * `openWebsocketStream()` implementation
     *
     * If any error occurres during client token insertion, stream will be closed
     *
     * @param new_client_connection Underlying TCP socket, required for debugging informations
     * @param new_client_stream Produced Websocket stream from TCP connection
     */
    void addClientStream(const boost::asio::ip::tcp::socket& new_client_connection, WebsocketStream new_client_stream) {
        const std::string remote_endpoint { endpointFor(new_client_connection) };

        try {
            const std::uint64_t new_client_token { tokens_count_++ };

            logger_.debug("New token for {}: {}", remote_endpoint, new_client_token);

            // Add token into connected clients NetworkBackend registry
            addClient(new_client_token); // May throws if token insertion failed
            // Move produced stream into clients stream registry
            const auto insert_stream_result {
                clients_stream_.insert({ new_client_token, std::move(new_client_stream) })
            };

            // Checks if client stream and messages queue insertions has been done
            assert(insert_stream_result.second);

            listenMessageFrom(new_client_token); // Now client stream was added, it can be listened
        } catch (const std::exception& err) { // Any token insertion error must result in stream closure
            logger_.error("Unable to add client for {}: {}", remote_endpoint, err.what());

            // Directly closes Websocket stream as client hasn't been added yet
            new_client_stream.async_close(boost::beast::websocket::internal_error,
                                          [this, &remote_endpoint](const boost::system::error_code& closure_err) {

                if (closure_err == boost::asio::error::operation_aborted) // Ignores if server execution stopped
                    return;

                if (closure_err)
                    logger_.error("Client {} websocket closure: {}",
                                  remote_endpoint, closure_err.message());
                else
                    logger_.debug("Client {} websocket closed prematurely: {}",
                                  remote_endpoint, closure_err.message());
            });
        }
    }

    /// Syncs client state with server state by sending recursively each flushed message to given client
    void syncClient(const std::uint64_t client_token, MessagesQueueView client_messages_queue) final {
        // If there is any message inside merged queue, then recursive async calls are still running
        const bool sending_messages { !merged_remaining_messages_.empty() };

        // Merges this client queue with global queue, saving client token destination
        while (client_messages_queue.hasNext())
            merged_remaining_messages_.push({ client_token, client_messages_queue.next() });

        // Initiates recursive calls if no recursive async calls are already sending RPTL messages inside queue and
        // there is new messages to send
        if (!sending_messages && !merged_remaining_messages_.empty())
            sendRemainingMessages();
    }

    /**
     * @brief Runs next Asio asynchronous operations handler until input events queue is no longer empty
     */
    void waitForEvent() final {
        // As interaction with clients might occurres, they must be synced with server and game state
        synchronize();

        while (!inputReady()) { // While input events queue is empty
            std::vector<std::uint64_t> dead_clients;
            // Max count of dead clients is count of actual clients
            dead_clients.reserve(clients_stream_.size());

            // Checks for all dead clients
            for (const auto& client : clients_stream_) {
                const std::uint64_t token { client.first }; // Retrieves token for current entry

                if (!isAlive(token)) // If connection is dead, it must be closed
                    dead_clients.push_back(token);
            }

            // As clients_stream_ elements must not be erased during iteration, closeStream() calls are deferred
            for (const std::uint64_t dead_client_token : dead_clients)
                closeStream(dead_client_token);

            // Wait for next asynchronous IO operation handler, it may triggers an input event
            async_io_context_.run_one();
        }
    }

public:
    /**
     * @brief Constructs IO interface listening for new TCP connections on given local endpoint
     *
     * In addition to that, constructor will initialize an Asio signal set listening for `SIGINT` and `SIGTERM` to
     * close IO interface when required.
     *
     * @param local_endpoint Endpoint clients will connect to
     * @param logging_context Context for WS backend logging features
     * @param players_limit Maximum number of actors registered simultaneously
     */
    explicit BeastWebsocketBackendBase(const boost::asio::ip::tcp::endpoint& local_endpoint,
                                       Utils::LoggingContext& logging_context, const std::size_t players_limit = 2)
    : NetworkBackend { players_limit },
    logger_ { "WS-Backend", logging_context },
    stop_signals_handling_ { async_io_context_ },
    tcp_acceptor_ { async_io_context_, local_endpoint },
    tokens_count_ { 0 } {
        Utils::LoggerView logger { getLogger() }; // Avoid to create LoggerView for each added signal

        // For each Posix signal that must be caught
        for (const int posix_signal : getCaughtSignals()) {
            boost::system::error_code err;
            // Error might occurs when adding signal to add, but it must NOT be fatal
            stop_signals_handling_.add(posix_signal, err);

            if (err) // Displays warning if signal will not be caught as expected
                logger.warn("Posix signal {} will not be caught: {}", posix_signal, err.message());
        }

        // Listens for Posix signals
        stop_signals_handling_.async_wait([this](const boost::system::error_code& err, const int posix_signal) {
            if (err) { // Checks for error during signal handling
                getLogger().error("Failed to handle posix signal {}: {}", posix_signal, err.message());
            } else {
                getLogger().debug("Posix signal {}, stopping...", posix_signal);
                close(); // Close IO interface to stop server
            }
        });

        start(); // Required to start because there is no way to use polymorphism on template class
    }

    /**
     * @brief Set Ready timer state to Pending, then uses an Asio steady clock to asynchronously wait for timer
     * countdown to be done
     */
    void beginTimer(Core::Timer& ready_timer) final {
        // Retrieves token for timer which will be pending
        const std::uint64_t token { ready_timer.token() };
        // Retrieves duration to wait asynchronously and set state to Pending, into std::chrono compatible type
        const std::chrono::milliseconds countdown { ready_timer.beginCountdown() };

        // Must NOT on the stack as timer will be pending asynchronously, owning shared with Asio timer async handler
        const auto asio_timer { std::make_shared<boost::asio::steady_timer>(async_io_context_, countdown) };

        // Does timer countdown
        asio_timer->async_wait([this, token, asio_timer](const boost::system::error_code& err) {
            if (err == boost::asio::error::operation_aborted) // Ignores if server stopped
                return;

            if (err) { // Does not trigger timer if error occurred while it was pending
                logger_.error("Pending timer {} countdown: {}", token, err.message());
                return;
            }

            // Actor UID doesn't matter, timer token does
            pushInputEvent(Core::TimerEvent { 0, token });
        });
    }

    /**
     * @brief Closes all opened Websocket streams stops handling asynchronous IO operations, then mark IO interface as
     * closed
     */
    void close() final {
        std::vector<std::uint64_t> client_tokens;
        // One token for each client
        client_tokens.reserve(clients_stream_.size());

        // Each client must be disocnnected
        for (const auto& client : clients_stream_) {
            const std::uint64_t token { client.first }; // Retrieves token for current entry

            client_tokens.push_back(token);
            // Client must be unregistered before being removed
            killClient(token); // No error, server closed
        }

        synchronize(); // Sends interrupt messages to clients before disconnection

        // As clients_stream_ elements must not be erased during iteration, closeStream() calls are deferred
        for (const std::uint64_t dead_client_token : client_tokens)
            closeStream(dead_client_token);

        // A null event must be pushed so waitForEvent() can properly return
        // None event must not be handled by Executor so actor UID doesn't matter
        pushInputEvent(Core::NoneEvent { 0 });

        // As all players will be disconnected, don't care about syncing server state with LOGGED_OUT broadcast message
        // Handlers execution can be stopped right now
        async_io_context_.stop();

        // Then IO interface can be considered closed
        InputOutputInterface::close();
    }
};


}


#endif //RPTOGETHER_SERVER_BEASTWEBSOCKETBACKENDBASE_INL
