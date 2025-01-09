#ifndef CHAT_HPP
#define CHAT_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include <libpq-fe.h>


class Chat {
public:
	int id;
	int user_id;
	std::string message;
	std::string created_at;
	Chat(int id, int user_id, const std::string& message, const std::string& created_at)
		: id(id), user_id(user_id), message(message), created_at(created_at) {
	}
};

#endif // CHAT_HPP