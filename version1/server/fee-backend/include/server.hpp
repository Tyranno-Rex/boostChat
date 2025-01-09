#pragma once

#include "context.hpp"
#include "router.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>
#include <zlib.h>


#pragma once

#include "context.hpp"
#include "router.hpp"
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
    std::shared_ptr<Router> router;
    net::io_context io_context{ 1 };
    std::vector<std::shared_ptr<tcp::socket>> clients;
    std::mutex clients_mutex;

public:
    Server(short port, std::shared_ptr<Router> router)
        : port(port), router(std::move(router)) {
    }
    void httpSession(tcp::socket socket);
    void chatSession(tcp::socket socket);
    void run();
    void chatRun();
    short getPort();
    void broadcastMessage(const std::vector<char>& message);
};