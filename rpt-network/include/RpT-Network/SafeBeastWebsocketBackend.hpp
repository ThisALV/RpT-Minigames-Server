#ifndef RPTOGETHER_SERVER_SAFEBEASTWEBSOCKETBACKEND_HPP
#define RPTOGETHER_SERVER_SAFEBEASTWEBSOCKETBACKEND_HPP

#include <boost/beast/ssl.hpp>
#include <RpT-Network/BeastWebsocketBackendBase.inl>

/**
 * @file SafeBeastWebsocketBackend.hpp
 */


namespace RpT::Network {


/**
 * @brief Implementation for secure HTTPS using SSL TCP underlying stream
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class SafeBeastWebsocketBackend : public BeastWebsocketBackendBase<boost::beast::ssl_stream<boost::beast::tcp_stream>> {
private:
    // Context providing crypto TLS features
    boost::asio::ssl::context tls_context_;

    /// Takes Websocket stream from `openWebsocketStream()` implementation to call `openSafeWebsocketLayer()` with open
    /// SSL layer
    void openSecureLayer(
            boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> new_client_stream);

    /// Takes Websocket stream from `openSecureLayer()` to call `addClientStream()` with open WSS layer
    void openSafeWebsocketLayer(
            boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> new_client_stream);

protected:
    /// Takes base TCP socket to build TCP stream, then build SSL stream using TLS features and uses it to open
    /// Websocket stream
    void openWebsocketStream(boost::asio::ip::tcp::socket new_client_connection) final;

public:
    /**
     * @brief Calls superclass constructor then initializes TLS features with given config files paths
     *
     * @param certificate_file Path to PEM certificate file
     * @param private_key_file Path to PEM private key file
     * @param local_endpoint Local server endpoint to be listening on
     * @param logging_context Context providing logging features
     *
     * @throws boost::system::system_error Error thrown by TLS features initialization
     */
    SafeBeastWebsocketBackend(const std::string& certificate_file, const std::string& private_key_file,
                              const boost::asio::ip::tcp::endpoint& local_endpoint,
                              Utils::LoggingContext& logging_context);
};


}


#endif //RPTOGETHER_SERVER_SAFEBEASTWEBSOCKETBACKEND_HPP
