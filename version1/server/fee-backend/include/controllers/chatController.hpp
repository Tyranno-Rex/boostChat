#pragma once

#include "../context.hpp"
#include "../services/chatService.hpp"
#include "../../include/server.hpp"
#include <string>
#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <boost/bind.hpp>
#include <memory>
#include <mutex>
#include <pqxx/pqxx>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class ChatController {
private:
	std::shared_ptr<ChatService> chatService;
	std::mutex g_chatRoomsMutex;

public:
	ChatController(std::shared_ptr<ChatService> service) : chatService(service) {};
	void createChatRoom(Context& ctx);
};