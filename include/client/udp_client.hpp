#pragma once

#include <queue>
#include "shared/udp.hpp"
#include "shared/buffer.hpp"

class udp_client: public std::enable_shared_from_this<udp_client> {
public:

    typedef std::function<void(std::unique_ptr<Buffer>& data, const udp::endpoint&, const std::shared_ptr<udp_client>&)> handle_packet_t;

    udp_client(const std::string& host, const std::string& port, Ipv ipv);
    void set_read_callback(handle_packet_t callback) {
        _read_callback = std::move(callback);
    }

    void send(Buffer& buffer);

    void start();
    void stop();
    bool alive();
private:
    handle_packet_t _read_callback;
    boost::asio::io_context _io_context;
    udp::socket _socket;
    udp::endpoint _server;
    udp::endpoint _sender;
    std::array<char, 0x5FFF> _recv_buffer;
    std::array<char, 0x5FFF> _send_buffer;
    std::queue<std::shared_ptr<Buffer>> _to_send;

    void receive();
    void send();
};
