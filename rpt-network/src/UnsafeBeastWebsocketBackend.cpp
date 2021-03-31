#include <RpT-Network/UnsafeBeastWebsocketBackend.hpp>

namespace RpT::Network {


UnsafeBeastWebsocketBackend::UnsafeBeastWebsocketBackend(
        const boost::asio::ip::tcp::endpoint& local_endpoint, Utils::LoggingContext& logging_context)
        : BeastWebsocketBackendBase<boost::beast::tcp_stream> { local_endpoint, logging_context } {}

void UnsafeBeastWebsocketBackend::openWebsocketStream(boost::asio::ip::tcp::socket new_client_connection) {
    // Stream ownership is not inside connected clients registry yet, ownership need to be preserved by async IO
    // handler
    const auto new_client_stream {
            std::make_shared<boost::beast::websocket::stream<boost::beast::tcp_stream>>(
                    std::move(new_client_connection))
    };

    new_client_stream->async_accept([this, new_client_stream](const boost::system::error_code& err) {
        boost::asio::ip::tcp::socket& underlying_socket { new_client_stream->next_layer().socket() };

        if (err) {
            if (err != boost::asio::error::operation_aborted) // Silent if server was stopped
                getLogger().error("Websocket handshaking with {}: {}",
                                  endpointFor(underlying_socket), err.message());

            return; // In any case, failed Websocket handshake means client should NOT be added to registry
        }

        // Successfully handshake Websocket, move stream into registry, providing underlying TCP socket
        addClientStream(underlying_socket, std::move(*new_client_stream));
    });
}


}
