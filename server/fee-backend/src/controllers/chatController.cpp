#include "../../include/controllers/chatController.hpp"

void ChatController::createChatRoom(Context& ctx) {
    std::string chatRoomId = ctx.formParam("id");
    std::string UserId = ctx.formParam("UserId");

    if (chatRoomId.empty() || UserId.empty()) {
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
