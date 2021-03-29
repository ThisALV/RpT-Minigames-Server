#ifndef RPTOGETHER_SERVER_BEASTWEBSOCKETBACKENDBASE_INL
#define RPTOGETHER_SERVER_BEASTWEBSOCKETBACKENDBASE_INL

#include <sstream>
#include <string_view>
#include <type_traits>
#include <boost/beast.hpp>
#include <RpT-Network/NetworkBackend.hpp>
#include <RpT-Utils/LoggerView.hpp>

/**
 * @file BeastWebsocketBackendBase.hpp
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
private:
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

    /// Handles message sending result to given client token
    class SentMessageHandler {
    private:
        BeastWebsocketBackendBase& protocol_instance_;
        const std::uint64_t client_token_;
        const std::shared_ptr<std::string> message_data_owner_;

    public:
        /**
         * @brief Constructs handler for message sent to given actor from given RPTL protocol backend instance
         *
         * @param protocol_instance Instance which sent message to client
         * @param client_token Client who must receive instance message
         * @param message_data_owner Shared pointer owning memory range used by message buffer
         */
        SentMessageHandler(BeastWebsocketBackendBase& protocol_instance, const std::uint64_t client_token,
                           const std::shared_ptr<std::string> message_data_owner)
        : protocol_instance_ { protocol_instance }, client_token_ { client_token },
        message_data_owner_ { message_data_owner } {}

        /// Makes handler callable object
        void operator()(const boost::system::error_code& err, std::size_t) {
            // Nothing to handle if no error occurred or server stopped
            if (!err || err == boost::asio::error::operation_aborted)
                return;

            std::string error_message { err.message() }; // Retrieves Asio error code associated message

            protocol_instance_.logger_.error("Unable to send message to client {}: {}", client_token_, error_message);

            // As RPTL protocol requires, connection if closed if any error occurred, using specific error message
            // Retrieved error message can be moved inside message sent handler result
            protocol_instance_.killClient(client_token_, Utils::HandlingResult { error_message });
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
        void operator()(InputEventT&& input_event) {
            using SimplifiedInputEventT = std::decay_t<InputEventT>;

            // If logout message was sent, client is actor is unregistered and must be removed
            // If handshaking was done, a new actor was registered and clients should be synced about that
            if constexpr (std::is_same_v<Core::LeftEvent, SimplifiedInputEventT>) {
                // As LeftEvent was triggered by logout command, we know no error occurred
                protocol_instance_.killClient(sender_token_);
            } else if constexpr (std::is_same_v<Core::JoinedEvent, SimplifiedInputEventT>) {
                // Handshaking done, actor has been registered for given client, sync new actor owner
                protocol_instance_.privateMessage(sender_token_, protocol_instance_.formatRegistrationMessage());

                // Then sync all registered clients, including newly registered one
                std::string logged_in_message { // Formats Logged In RPTL command message
                        LOGGED_IN_COMMAND + (' ' + std::to_string(input_event.actor())) + ' ' + input_event.playerName()
                };

                protocol_instance_.broadcastMessage(std::move(logged_in_message));
            }
        }
    };

    // Provides logging features
    Utils::LoggerView logger_;
    // Websocket stream using given TCP stream for each client token
    std::unordered_map<std::uint64_t, boost::beast::websocket::stream<TcpStream>> clients_stream_;
    // Provides running context for all async IO operations
    boost::asio::io_context async_io_context_;
    // Provides ready TCP connection to open WS stream from
    boost::asio::ip::tcp::acceptor tcp_acceptor_;
    // Keep total clients count so an unique token can be given to each new client
    std::uint64_t tokens_count_;

    /**
     * @brief Sends private RPTL message to given client stream
     *
     * @param client_token Token for used client stream
     * @param message RPTL message to be sent
     */
    void privateMessage(const std::uint64_t client_token, std::string message) {
        // Moves message argument into pointer shared between method and async_write callback so argument string data
        // is owned until async_write handler call finished
        const auto message_owner { std::make_shared<std::string>(std::move(message)) };
        // Buffer read by Asio to send message, data must be valid until handler call finished
        const boost::asio::const_buffer message_buffer { message_owner->data(), message_owner->size() };

        clients_stream_.at(client_token).async_write(
                message_buffer, SentMessageHandler { *this, client_token, message_owner });
    }

    /**
     * @brief Sends broadcast RPTL message to all registered clients
     *
     * @param message RPTL message to be sent
     */
    void broadcastMessage(std::string message) {
        // Moves message argument into pointer shared between method and async_write callback so argument string data
        // is owned until async_write handler call finished
        const auto message_owner { std::make_shared<std::string>(std::move(message)) };
        // Buffer read by Asio to send message, data must be valid until handler call finished
        const boost::asio::const_buffer message_buffer { message_owner->data(), message_owner->size() };

        for (auto& [client_token, stream] : clients_stream_) // For each registered client, send message with buffer
            if (isClientRegistered(client_token))
                stream.async_write(message_buffer, SentMessageHandler { *this, client_token, message_owner });
    }

    /**
     * @brief Accepts next incoming TCP client connection, then wait for next client again
     */
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
                client_triggered_event.apply_visitor(TriggeredInputEventVisitor { *this, client_token });

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
        removeClient(client_token);

        // Retrieves stream closure reason to determine Websocket close frame close code
        const Utils::HandlingResult& disconnection_reason { disconnectionReason(client_token) };
        // Formatting interrupt command message to inform client that it will be disconnected and why
        std::string interrupt_message { INTERRUPT_COMMAND };

        // By default, set to normal close code
        boost::beast::websocket::close_reason websocket_close_reason { boost::beast::websocket::normal };

        // If any error occurred for disconnection to happen, then set error code with custom message, for close
        // frame and interrupt command message
        if (!disconnection_reason) {
            const std::string& error_message { disconnection_reason.errorMessage() };

            interrupt_message += ' ' + error_message;

            websocket_close_reason.code = boost::beast::websocket::close_code::abnormal;
            websocket_close_reason.reason = error_message;
        }

        privateMessage(client_token, std::move(interrupt_message));

        clients_stream_.at(client_token).async_close(websocket_close_reason, [this, client_token](
                const boost::system::error_code& err) {

            // If for any reason clean Websocket closure failed, TCP connection will be closed anyways, but a warning
            // message must be logged
            if (err)
                logger_.warn("Unclean disconnection with client {}: {}", client_token, err.message());

            // In any case, client stream must now be removed as it is finally closed
            const std::size_t removed_streams_count { clients_stream_.erase(client_token) };

            assert(removed_streams_count == 1); // Must be sure that only one stream was properly removed

            // Formats logout command message to sync clients with server
            std::string logout_message { LOGGED_OUT_COMMAND };
            logout_message += ' ' + std::to_string(client_token);
            // Then we can broadcast and confirm client is logged out and sync clients with server
            broadcastMessage(std::move(logout_message));
        });
    }

protected:
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
    void addClientStream(const boost::asio::ip::tcp::socket& new_client_connection,
                         boost::beast::websocket::stream<TcpStream> new_client_stream) {

        const std::string remote_endpoint { endpointFor(new_client_connection) };

        try {
            const std::uint64_t new_client_token { tokens_count_++ };

            logger_.debug("New token for {}: {}", remote_endpoint, new_client_token);

            // Add token into connected clients NetworkBackend registry
            addClient(new_client_token); // May throws if token insertion failed
            // Move produced stream into clients stream registry
            const auto insert_result { clients_stream_.insert({ new_client_token, std::move(new_client_stream) }) };

            assert(insert_result.second); // Checks if client stream insertion has been done

            listenMessageFrom(new_client_token); // Now client stream was added, it can be listened
        } catch (const std::exception& err) { // Any token insertion error must result in stream closure
            logger_.error("Unable to add client for {}: {}", remote_endpoint, err.what());

            // Directly closes Websocket stream as client hasn't been added yet
            new_client_stream.async_close(boost::beast::websocket::internal_error,
                                          [this, &err, &remote_endpoint](const boost::system::error_code& closure_err) {

                if (closure_err == boost::asio::error::operation_aborted) // Ignores if server execution stopped
                    return;

                if (closure_err)
                    logger_.error("Client {} websocket closure: {}", remote_endpoint, closure_err.message());
                else
                    logger_.debug("Client {} websocket closed prematurely: {}", remote_endpoint);
            });
        }
    }

    /// Broadcasts given RPTL Service Event command
    void handleServiceEvent(std::string se_command) final {
        broadcastMessage(std::move(se_command));
    }

    /// Sends private message to appropriate client containing RPTL Service Request Response
    void handleServiceRequestResponse(const std::uint64_t sr_actor_owner, std::string sr_response) final {
        privateMessage(sr_actor_owner, std::move(sr_response));
    }

public:
    /**
     * @brief Constructs IO interface listening for new TCP connections on given local endpoint
     *
     * @param local_endpoint Endpoint clients will connect to
     * @param logging_context Context for WS backend logging features
     */
    explicit BeastWebsocketBackendBase(const boost::asio::ip::tcp::endpoint& local_endpoint,
                                       Utils::LoggingContext& logging_context)
    : logger_ { "WS-Backend", logging_context },
    tcp_acceptor_ { async_io_context_, local_endpoint },
    tokens_count_ { 0 } {}

    /**
     * @brief Starts listening for incoming client TCP connection on local endpoint
     */
    void start() {
        waitNextClient();
    }

    /**
     * @brief Closes all opened Websocket streams stops handling asynchronous IO operations, then mark IO interface as
     * closed
     */
    void close() final {
        for (const auto& client : clients_stream_) // For each client token, closes associated stream
            closeStream(client.first);

        // Then IO interface can be considered closed
        InputOutputInterface::close();
    }

    /**
     * @brief Runs next Asio asynchronous operations handler until input events queue is no longer empty
     *
     * @returns
     */
    void waitForEvent() final {
        while (!inputReady()) { // While input events queue is empty
            // Wait for next asynchronous IO operation handler, it may triggers an input event
            async_io_context_.run_one();
        }
    }
};


}


#endif //RPTOGETHER_SERVER_BEASTWEBSOCKETBACKENDBASE_INL
