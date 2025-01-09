#ifndef CHAT_SERVICE_HPP
#define CHAT_SERVICE_HPP

#include <vector>
#include <optional>
#include <libpq-fe.h>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <string>
#include <memory>
#include <pqxx/pqxx>

class ChatService {
private:
    std::string db_conn_str_;
    std::map<std::string, std::map<std::string, std::string>> chatRooms;
    std::mutex chatRoomsMutex;

public:
    ChatService(const std::string& db_conn_str)
        : db_conn_str_(db_conn_str) {
    };
    void S_createChatRoom(std::string chatRoomId, std::string UserId);
	bool S_doesChatRoomExist(const std::string& chatRoomId);
	std::map<std::string, std::map<std::string, std::string>> getChatRooms() { return chatRooms; }
};

#endif