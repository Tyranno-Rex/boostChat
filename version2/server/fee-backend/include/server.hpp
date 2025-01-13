#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>
#include <zlib.h>
#include <mutex>
#include "packet.hpp"

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

#define GET (http::verb::get)
#define POST (http::verb::post)
#define PUT (http::verb::put)
#define PATCH (http::verb::patch)
#define DELETE (http::verb::delete_)

#define expected_hcv 0x02
#define expected_tcv 0x03
#define expected_checksum 0x04
#define expected_size 0x08

extern std::map<std::string, std::map<std::string, std::string>> g_chatRooms;

class Server {
private:
    short port;
    void handleRead(const boost::system::error_code& error, size_t bytes_transferred,
        std::shared_ptr<tcp::socket> client_socket,
        std::shared_ptr<std::array<char, 1024>> temp_buffer,
        std::shared_ptr<PacketBuffer> packet_buffer);

    boost::asio::io_context io_context;
    std::vector<std::shared_ptr<tcp::socket>> clients;
    std::mutex clients_mutex;

public:
    Server(short port)
        : port(port) {
    }
	void handleAccept(std::shared_ptr<tcp::socket> client_socket, tcp::acceptor& acceptor);
    void chatSession(std::shared_ptr<tcp::socket> client_socket);
    void chatRun();
	short getPort() { return port; }
};