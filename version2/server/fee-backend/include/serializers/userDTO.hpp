#pragma once

#include "../models/user.hpp"
#include <boost/asio.hpp>
#include <boost/json.hpp>

class UserDTO {
public:
	static boost::json::object toJson(const User& user);
	static User fromJson(const boost::json::object& json);
};