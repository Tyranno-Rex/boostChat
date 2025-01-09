#pragma once

#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <map>

namespace http = boost::beast::http;

class Context {
private:
    const http::request<http::string_body>& request;
    http::response<http::string_body>& response;
    std::map<std::string, std::string> params;
	std::map<std::string, std::string> form_params;
	http::status status;
	std::string client_ip;

public:
    Context(const http::request<http::string_body>& req,
        http::response<http::string_body>& res);

    const http::request<http::string_body>& getRequest();
    http::response<http::string_body>& getResponse();
	std::string getRequestBody();

    void setResponseResult(http::status status, const std::string& body);
    std::string getParam(const std::string& key) const;
    void setParam(const std::string& key, const std::string& value);
	std::string pathParam(const std::string& key) const;
	std::string formParam(const std::string& key) const;
	void setClientIP(const std::string& ip) {client_ip = ip;}   
	std::string getClientIP() { return client_ip; }
};