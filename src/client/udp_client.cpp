#include "client/udp_client.hpp"
#include <stdio.h>
#include <boost/asio/error.hpp>
#include <stdexcept>

udp_client::udp_client(const std::string& host, const std::string& port, Ipv ipv)
: _socket(_io_context, udp::endpoint(ipv == Ipv::V4 ? udp::v4() : udp::v6(), 0))
{
    udp::resolver resolver(_io_context);
    auto results = resolver.resolve(ipv == Ipv::V4 ? udp::v4() : udp::v6(), host, port);
    auto endpoint_it = results.begin();
    if (endpoint_it == results.end()) {
        throw std::runtime_error("could not resolve server endpoint");
    }
    _server = endpoint_it->endpoint();
}

void udp_client::receive() {
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

void udp_client::send() {
    if (_to_send.empty()) {
        boost::asio::post(_io_context, [this] {
            this->send();
        });
        return;
    }
    auto buffer = this->_to_send.front(); this->_to_send.pop();
    auto size = buffer->remaining();
    assert(size <= _send_buffer.size());
    buffer->read_buffer(_send_buffer.data(), size);
    _socket.async_send_to(boost::asio::buffer(_send_buffer, size), _server, [this](const boost::system::error_code& error, size_t bytes_sent) {
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

void udp_client::send(Buffer &buffer) {
    auto to_write_buffer = std::make_shared<Buffer>();
    to_write_buffer->write_buffer(buffer, buffer.remaining());
    _to_send.push(std::move(to_write_buffer));
}

void udp_client::start() {
    this->receive();
    this->send();
    _io_context.run();
}

void udp_client::stop() {
    _io_context.stop();
}

bool udp_client::alive() {
    return !_io_context.stopped();
}
