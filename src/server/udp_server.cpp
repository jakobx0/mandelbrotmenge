#include "server/udp_server.hpp"
#include <stdio.h>
#include <boost/thread.hpp>

udp_server::udp_server(Ipv ipv, int port) :
_socket(_io_context, udp::endpoint(ipv == Ipv::V4 ? udp::v4() : udp::v6(), port))
{
    printf("[info] UDP server started on port %d\n", port);
}

void udp_server::receive() {
    _socket.async_receive_from(boost::asio::buffer(_recv_buffer), _sender, [this](const boost::system::error_code& error, size_t bytes_received) {
        if (error) {
            printf("[error] Error receiving data: %s\n", error.message().c_str());
            _io_context.stop();
            return;
        }
        if (bytes_received == 0) {
            this->receive();
            return;
        }
        printf("[debug] Received %zu bytes\n", bytes_received);
        auto buffer = std::make_unique<Buffer>();
        buffer->write_buffer(_recv_buffer.data(), bytes_received);
        this->_read_callback(buffer, _sender, this->shared_from_this());
        this->receive();
    });
}


void udp_server::send() {
    if (_to_send.empty()) {
        boost::asio::post(_io_context, [this] {
            this->send();
        });
        return;
    }
    auto& buffer_endpoint_pair = _to_send.front();
    auto& buffer = buffer_endpoint_pair.first;
    auto& endpoint = buffer_endpoint_pair.second;
    auto size = buffer->remaining();
    assert(size <= _send_buffer.size());
    buffer->read_buffer(_send_buffer.data(), size);
    _to_send.pop();
    _socket.async_send_to(boost::asio::buffer(_send_buffer, size), endpoint, [this](const boost::system::error_code& error, size_t bytes_sent) {
        if (error) {
            printf("[error] Error sending data: %s\n", error.message().c_str());
            _io_context.stop();
            return;
        }
        if (bytes_sent > 0)
            printf("[debug] Sent %zu bytes\n", bytes_sent);
        this->send();
    });
}

void udp_server::send_to(Buffer &buffer, const udp::endpoint &endpoint) {
    auto to_write_buffer = std::make_unique<Buffer>();
    to_write_buffer->write_buffer(buffer, buffer.remaining());
    _to_send.emplace(std::move(to_write_buffer), endpoint);
}

void udp_server::start() {
    this->receive();
    this->send();
    _io_context.run();
}

void udp_server::stop() {
    _io_context.stop();
}
