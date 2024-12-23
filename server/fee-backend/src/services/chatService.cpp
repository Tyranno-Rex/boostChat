

#include "../../include/services/chatService.hpp"
#include "../../include/server.hpp"

std::mutex g_chatRoomsMutex;

void ChatService::S_createChatRoom(std::string chatRoomId, std::string UserId) {
    std::lock_guard<std::mutex> lock(chatRoomsMutex);
    chatRooms[chatRoomId] = { {"UserId", UserId} };
}

bool ChatService::S_doesChatRoomExist(const std::string& chatRoomId) {
    std::lock_guard<std::mutex> lock(chatRoomsMutex);
    return chatRooms.find(chatRoomId) != chatRooms.end();
}