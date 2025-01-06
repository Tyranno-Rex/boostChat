

#include "../include/server.hpp"
#include "../include/router.hpp"
#include "../include/services/userService.hpp"
#include "../include/controllers/userController.hpp"
#include "../include/services/chatService.hpp"
#include "../include/controllers/chatController.hpp"
#include <pqxx/pqxx>


int main(void) {
    try {

        auto router_http = std::make_shared<Router>();
        auto router_chat = std::make_shared<Router>();
        
        Server httpServer(3571, router_http);
        Server chatServer(3572, router_chat);

        std::string db_conn_str = "dbname=fee user=postgres password=postgres hostaddr=";
        auto userService = std::make_shared<UserService>(db_conn_str);
        auto userController = std::make_shared<UserController>(userService);
        auto chatService = std::make_shared<ChatService>(db_conn_str);
		auto chatController = std::make_shared<ChatController>(chatService);
        pqxx::connection conn = pqxx::connection("dbname=fee user=postgres password=1234 hostaddr=");

        router_http->setPrefix("/api/v1");
        // userController를 캡처 리스트에 추가
        router_http->addRoute(GET, "/users/{id}", [userController](Context& ctx) {
            std::cout << "GET /users/{id}" << std::endl;
            userController->getUserById(ctx);
            });

        router_http->addRoute(POST, "/users", [&conn, userController](Context& ctx) {
            conn.prepare("insert_user", "INSERT INTO users (username, email, password, created_at, updated_at) VALUES ($1, $2, $3, NOW(), NOW())");
            userController->addUser(conn, ctx);
            });

        router_http->addRoute(DELETE, "/users/{id}", [userController](Context& ctx) {
            userController->deleteUserById(ctx);
            });
        
        router_http->addRoute(GET, "/room", [](Context& ctx) {
			//chatController->getChatRooms(ctx);
			ctx.setResponseResult(http::status::ok, "Room");
			});

        router_http->addRoute(POST, "/room/create", [chatController](Context& ctx) {
            chatController->createChatRoom(ctx);
            });

        // HTTP 서버와 채팅 서버를 각각 별도의 스레드에서 실행
        std::thread httpThread([&httpServer]() {
            httpServer.run();
            });

		std::thread chatThread([&chatServer]() {
			chatServer.chatRun();
            });

        httpThread.join();
        chatThread.join();
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}