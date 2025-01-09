#include "../../include/controllers/chatController.hpp"

void ChatController::createChatRoom(Context& ctx) {
	// body에 있는 id와 user_id를 읽어옴
	auto body = ctx.getRequestBody();
	auto json = boost::json::parse(body);
	std::string chatRoomId = json.at("id").as_string().c_str();
	std::string UserId = json.at("user_id").as_string().c_str();

	if (chatRoomId.empty() || UserId.empty()) {
		std::cout << "chatRoomId: " << chatRoomId << std::endl;
		std::cout << "UserId: " << UserId << std::endl;
		ctx.setResponseResult(http::status::bad_request, "Invalid input data.");
		return;
	}

	if (chatService->S_doesChatRoomExist(chatRoomId)) {
		ctx.setResponseResult(http::status::conflict, "Chat room already exists.");
	}
	else {
		chatService->S_createChatRoom(chatRoomId, UserId);
		ctx.setResponseResult(http::status::created, "Chat room created.");
	}

	for (auto& room : chatService->getChatRooms()) {
		std::cout << "Chat Room: " << room.first << std::endl;
	}
}