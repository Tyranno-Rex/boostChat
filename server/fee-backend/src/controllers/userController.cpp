#include "../../include/controllers/userController.hpp"


//UserController::UserController(std::shared_ptr<IUserService> service)
//	: UserService(service) {
//}

void UserController::getUserById(Context& ctx) {
	int id = std::stoi(ctx.pathParam("id"));
	auto user = UserService->getUserById(id);
	if (user.has_value()) {
		http::status status = http::status::ok;
		ctx.setResponseResult(status, "User found: " + user->username + ", " + user->email);
	}
	else {
		http::status status = http::status::not_found;
		ctx.setResponseResult(status, "User not found.");
	}
}

void UserController::addUser(pqxx::connection& conn, Context& ctx) {
	std::string username = ctx.formParam("username");
	std::string email = ctx.formParam("email");
	std::string password = ctx.formParam("password");
	UserService->addUser(conn, username, email, password);
	http::status status = http::status::created;
	ctx.setResponseResult(status, "User added.");
}

void UserController::deleteUserById(Context& ctx) {
	int id = std::stoi(ctx.pathParam("id"));
	UserService->deleteUserById(id);
	http::status status = http::status::accepted;
	ctx.setResponseResult(status, "User deleted.");
}