#include "../include/context.hpp"

const http::request<http::string_body>& Context::getRequest() {
    return request;
}

http::response<http::string_body>& Context::getResponse() {
    return response;
}

void Context::setResponseResult(http::status status, const std::string& body) {
    response.result(status);
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

// New method: Retrieve a path parameter
std::string Context::pathParam(const std::string& key) const {
    return getParam(key); // Reuse getParam to fetch path parameters
}

// New method: Retrieve a form parameter from the request body
std::string Context::formParam(const std::string& key) const {
    // Parse the request body for form data
    auto body = request.body();
    std::istringstream bodyStream(body);
    std::string pair;
    while (std::getline(bodyStream, pair, '&')) {
        auto delimiterPos = pair.find('=');
        if (delimiterPos != std::string::npos) {
            std::string paramKey = pair.substr(0, delimiterPos);
            std::string paramValue = pair.substr(delimiterPos + 1);

            if (paramKey == key) {
                return paramValue;
            }
        }
    }
    return ""; // Return empty if the key is not found
}
