

#include "../../include/services/chatService.hpp"

void ChatService::S_createChatRoom(std::map<std::string, std::map<std::string, std::string>>& g_chatRooms,
									std::string chatRoomId, std::string UserId) {
	g_chatRooms[chatRoomId] = { {"UserId", UserId} };
}

