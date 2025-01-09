#pragma once

#include "../models/user.hpp"
#include <vector>
#include <optional>
#include <libpq-fe.h>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <string>
#include <memory>
#include <pqxx/pqxx>

class IUserService {

public:
	virtual void addUser(
		pqxx::connection& conn,
		const std::string& username, 
		const std::string& email,
		const std::string& password) = 0;
	virtual std::optional<User> getUserById(unsigned int id) = 0;
	virtual bool deleteUserById(unsigned int id) = 0;
};

class UserService : public IUserService {
private:
	std::string db_conn_str_;

public:
	UserService(const std::string& db_conn_str)
		: db_conn_str_(db_conn_str) {
	};
	void addUser(pqxx::connection& conn, const std::string& username, const std::string& email, const std::string& password) override;
	std::optional<User> getUserById(unsigned int id) override;
	bool deleteUserById(unsigned int id) override;
};
