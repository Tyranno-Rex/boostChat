#ifndef USERCONTROLLER_HPP
#define USERCONTROLLER_HPP

#pragma once

#include "../context.hpp"
#include "../serializers/userDTO.hpp"
#include "../services/userService.hpp"
#include <string>
#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <boost/bind.hpp>
#include <memory>
#include <pqxx/pqxx>


namespace beast = boost::beast; 
namespace http = beast::http;   
namespace net = boost::asio;    
using tcp = net::ip::tcp;       

class UserController {
private:
	std::shared_ptr<IUserService> UserService;

public:
    UserController(std::shared_ptr<IUserService> service): UserService(service) {};

    void getUserById(Context& ctx);
	void addUser(pqxx::connection& conn, Context& ctx);
    void deleteUserById(Context& ctx);
};

#endif