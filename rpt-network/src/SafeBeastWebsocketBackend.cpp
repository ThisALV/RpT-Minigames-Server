#include <RpT-Network/SafeBeastWebsocketBackend.hpp>


namespace RpT::Network {


SafeBeastWebsocketBackend::SafeBeastWebsocketBackend(
        const std::string& certificate_file, const std::string& private_key_file,
        const boost::asio::ip::tcp::endpoint& local_endpoint, Utils::LoggingContext& logging_context)
        : BeastWebsocketBackendBase<boost::beast::ssl_stream<boost::beast::tcp_stream>> {
    local_endpoint,logging_context },
    tls_context_ { boost::asio::ssl::context::tls_server } {

    // Initializes path for both private key and certificate files
    tls_context_.use_certificate_file(certificate_file, boost::asio::ssl::context::pem);
    tls_context_.use_private_key_file(private_key_file, boost::asio::ssl::context::pem);
}

void SafeBeastWebsocketBackend::openSecureLayer(
        boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> new_client_stream) {

    // Websocket stream should be alive until TLS layer has been open
    const auto new_client_stream_owner {
        std::make_shared<boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>>(
                std::move(new_client_stream))
    };

    // Get TLS layer from shared Websocket stream
    boost::beast::ssl_stream<boost::beast::tcp_stream>& tls_layer { new_client_stream_owner->next_layer() };

    // Next layer after TCP stream should be TLS layer
    tls_layer.async_handshake(boost::asio::ssl::stream_base::server,
                              [this, new_client_stream_owner](const boost::system::error_code& err) {

        if (err) {
            if (err != boost::asio::error::operation_aborted) {
                const boost::asio::ip::tcp::socket& underlying_socket { // Get base connection socket for logging
                    new_client_stream_owner->next_layer().next_layer().socket()
                };

                getLogger().error("TLS handshaking with {}: {}", endpointFor(underlying_socket), err.message());
            }

            return; // In any case, failed TLS handshaking means client should NOT be added into registry
        }

        // Moves WSS stream to open WSS layer
        openSafeWebsocketLayer(std::move(*new_client_stream_owner));
    });
}

void SafeBeastWebsocketBackend::openSafeWebsocketLayer(
        boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> new_client_stream) {

    // Websocket stream should be alive until WSS layer has been open
    const auto new_client_stream_owner {
            std::make_shared<boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>>(
                    std::move(new_client_stream))
    };

    new_client_stream_owner->async_accept([this, new_client_stream_owner](const boost::system::error_code& err) {
        boost::asio::ip::tcp::socket& underlying_socket { // Get base TCP socket for logging purpose
                new_client_stream_owner->next_layer().next_layer().socket()
        };

        if (err) {
            if (err != boost::asio::error::operation_aborted) { // Ignores if server stopped
                getLogger().error("WSS accepting connection from {}: {}",
                                  endpointFor(underlying_socket), err.message());
            }

            return; // In any case, failed WSS handshaking/accepting means client should NOT be added into registry
        }

        // Moves WSS open stream into clients registry
        addClientStream(underlying_socket, std::move(*new_client_stream_owner));
    });
}

void SafeBeastWebsocketBackend::openWebsocketStream(boost::asio::ip::tcp::socket new_client_connection) {
    // Builds new websocket stream over security layer using TLS features
    boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> new_client_stream {
        std::move(new_client_connection), tls_context_
    };

    openSecureLayer(std::move(new_client_stream)); // Open TLS layer for given Websocket stream
}


}
