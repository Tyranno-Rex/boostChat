#include "../../include/services/userService.hpp"

void UserService::addUser(pqxx::connection& conn, const std::string& username,
	const std::string& email, const std::string& password) {
	try {
		pqxx::work txn(conn);
		txn.exec_prepared("insert_user", username, email, password);
		txn.commit();
	}
	catch (const std::exception& e) {
		std::cerr << "Error adding user: " << e.what() << std::endl;
		throw;
	}
}

std::optional<User> UserService::getUserById(unsigned int id) {
	try {
		pqxx::connection conn(db_conn_str_);
		pqxx::work txn(conn);
		conn.prepare("select_user", "SELECT id, username, email, password, created_at, updated_at FROM users WHERE id = $1");
		pqxx::result result = txn.exec_prepared("select_user", id);
		if (result.empty()) {
			return std::nullopt;
		}
		const auto& row = result[0];
		return User(row["id"].as<int>(), row["username"].as<std::string>(), row["email"].as<std::string>(), row["password"].as<std::string>(), row["created_at"].as<std::string>(), row["updated_at"].as<std::string>());
	}
	catch (const std::exception& e) {
		std::cerr << "Error fetching user: " << e.what() << std::endl;
		return std::nullopt;
	}
}

bool UserService::deleteUserById(unsigned int id) {
	try {
		pqxx::connection conn(db_conn_str_);
		pqxx::work txn(conn);
		conn.prepare("delete_user", "DELETE FROM users WHERE id = $1");
		txn.exec_prepared("delete_user", id);
		txn.commit();
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "Error deleting user: " << e.what() << std::endl;
		return false;
	}
}

