#ifndef RPTOGETHER_SERVER_BEASTWEBSOCKETBACKENDBASE_INL
#define RPTOGETHER_SERVER_BEASTWEBSOCKETBACKENDBASE_INL

#include <string_view>
#include <boost/beast.hpp>
#include <RpT-Network/NetworkBackend.hpp>

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
    // Websocket stream using given TCP stream for each client token
    std::unordered_map<std::uint64_t, boost::beast::websocket::stream<TcpStream>> clients_stream_;
    // Provides running context for all async IO operations
    boost::asio::io_context async_io_context_;
    // Provides ready TCP connection to open WS stream from
    boost::asio::ip::tcp::acceptor tcp_acceptor_;

    /**
     * @brief Sends private RPTL message to given client stream
     *
     * @param client_token Token for used client stream
     * @param message RPTL message to be sent
     */
    void privateMessage(std::uint64_t client_token, std::string_view message);

    /**
     * @brief Sends broadcast RPTL message to all registered clients
     *
     * @param message RPTL message to be sent
     */
    void broadcastMessage(std::string_view message);

    /**
     * @brief Accepts next incoming TCP client connection, then wait for next client again
     */
    void waitNextClient();

    /**
     * @brief Receives and handles incoming message from given client
     *
     * @param client_token Token for client to be listening for
     */
    void listenMessageFrom(std::uint64_t client_token);

    /**
     * @brief If registered, closes pipeline with client actor, then closes Websocket and underlying streams
     *
     * @param client_token Token for stream to be closed and client to be unregistered (if registered)
     */
    void closeStream(std::uint64_t client_token);

protected:
    /**
     * @brief Must asynchronously open Websocket stream using `addClientStream()` from given established TCP connection
     *
     * @param new_client_connection TCP connection ready to handshake into upper protocols layer
     */
    virtual void openWebsocketStream(boost::asio::ip::tcp::socket& new_client_connection) = 0;

    /**
     * @brief Inserts new client using server-defined token and given Websocket stream
     *
     * @param new_client_stream
     */
    void addClientStream(boost::beast::websocket::stream<TcpStream>& new_client_stream);

public:
    /**
     * @brief Constructs IO interface listening for new TCP connections on given local endpoint
     *
     * @param local_endpoint Endpoint clients will connect to
     */
    explicit BeastWebsocketBackendBase(const boost::asio::ip::tcp::endpoint& local_endpoint)
    : tcp_acceptor_ { async_io_context_, local_endpoint } {}

    /**
     * @brief Starts listening for incoming client TCP connection on local endpoint
     */
    void start();

    /**
     * @brief Closes all opened Websocket streams stop handlind asynchronous IO operations, then mark IO interface as
     * closed
     */
    void close() final;
};


}


#endif //RPTOGETHER_SERVER_BEASTWEBSOCKETBACKENDBASE_INL
