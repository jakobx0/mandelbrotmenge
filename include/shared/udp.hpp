#pragma once
#include <boost/asio.hpp>
#include "buffer.hpp"

using udp = boost::asio::ip::udp;

enum class Ipv {
    V4,
    V6
};
