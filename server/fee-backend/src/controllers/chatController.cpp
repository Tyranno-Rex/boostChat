#include "../../include/controllers/chatController.hpp"


void ChatController::createChatRoom(Context& ctx) {
	std::string chatRoomId = ctx.formParam("id");
	std::string UserId = ctx.formParam("UserId");
	auto chatRoom = g_chatRooms.find(chatRoomId);
	if (chatRoom != g_chatRooms.end()) {
		http::status status = http::status::conflict;
		ctx.setResponseResult(status, "Chat room already exists.");
	}
	else {
		chatService->S_createChatRoom(g_chatRooms, chatRoomId, UserId);
		http::status status = http::status::created;
		for (auto& chatRoom : g_chatRooms) {
			std::cout << chatRoom.first << "\n";
			for (auto& user : chatRoom.second) {
				std::cout << user.first << " : " << user.second << "\n";
			}
		}
		ctx.setResponseResult(status, "Chat room created.");
	}
}