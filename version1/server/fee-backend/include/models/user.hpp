#ifndef USER_MODEL_HPP
#define USER_MODEL_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include <libpq-fe.h>

class User {
public:
    int id;
    std::string username;
    std::string email;
	std::string password;
    std::string created_at;
    std::string updated_at;

	User(int id, const std::string& username, const std::string& email, const std::string& password, const std::string& created_at, const std::string& updated_at)
		: id(id), username(username), email(email), password(password), created_at(created_at), updated_at(updated_at) {
    }
};
#endif // USER_MODEL_HPP
