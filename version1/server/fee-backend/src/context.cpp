#include "../include/context.hpp"

Context::Context(const http::request<http::string_body>& req,
                        http::response<http::string_body>& res)
    : request(req), response(res) {
}

const http::request<http::string_body>& Context::getRequest() {
    return request;
}

http::response<http::string_body>& Context::getResponse() {
    return response;
}

std::string Context::getRequestBody() {
    return request.body();
}

void Context::setResponseResult(http::status status, const std::string& body) {
    response.result(status);
    response.set(http::field::content_type, "text/plain");
    response.body() = body;
    response.prepare_payload();
}

std::string Context::getParam(const std::string& key) const {
    auto it = params.find(key);
    if (it != params.end()) {
        return it->second;
    }
    return "";
}

void Context::setParam(const std::string& key, const std::string& value) {
    params[key] = value;
}

std::string Context::pathParam(const std::string& key) const {
    auto it = params.find(key);
    if (it != params.end()) {
        return it->second;
    }
    return "";
}

std::string Context::formParam(const std::string& key) const {
    auto it = form_params.find(key);
    if (it != form_params.end()) {
        return it->second;
    }
    return "";
}
