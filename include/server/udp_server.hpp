#pragma once

#include <unordered_map>
#include <queue>
#include "shared/udp.hpp"
#include "shared/buffer.hpp"

class udp_server: public std::enable_shared_from_this<udp_server> {
public:

    typedef std::function<void(std::unique_ptr<Buffer>& data, const udp::endpoint&, const std::shared_ptr<udp_server>&)> handle_packet_t;

    udp_server(Ipv ipv, int port);
    void set_read_callback(handle_packet_t callback) {
        _read_callback = std::move(callback);
    }

    void send_to(Buffer &buffer, const udp::endpoint &endpoint);

    void start();
    void stop();
private:
    handle_packet_t _read_callback;
    boost::asio::io_context _io_context;
    udp::socket _socket;
    udp::endpoint _sender;
    std::array<char, 0x5FFF> _recv_buffer;
    std::array<char, 0x5FFF> _send_buffer;
    std::queue<std::pair<std::unique_ptr<Buffer>, udp::endpoint>> _to_send;

    void receive();
    void send();
};