#ifndef RPTOGETHER_SERVER_UNSAFEBEASTWEBSOCKETBACKEND_HPP
#define RPTOGETHER_SERVER_UNSAFEBEASTWEBSOCKETBACKEND_HPP

#include <RpT-Network/BeastWebsocketBackendBase.inl>

/**
 * @file UnsafeBeastWebsocketBackend.hpp
 */


namespace RpT::Network {


/**
 * @brief `BeastWebsocketBackendBase` implementation for unsecure HTTP using raw TCP underlying stream
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class UnsafeBeastWebsocketBackend : public BeastWebsocketBackendBase<boost::beast::tcp_stream> {
protected:
    /// Implementation takes base TCP socket to build TCP stream then uses it raw to build Websocket stream
    void openWebsocketStream(boost::asio::ip::tcp::socket new_client_connection) final;

public:
    /// Calls superclass constructor
    UnsafeBeastWebsocketBackend(const boost::asio::ip::tcp::endpoint& local_endpoint,
                                Utils::LoggingContext& logging_context);
};


}


#endif //RPTOGETHER_SERVER_UNSAFEBEASTWEBSOCKETBACKEND_HPP
