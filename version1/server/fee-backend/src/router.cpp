#include "../include/router.hpp"
#include <iostream>

void Router::addRoute(http::verb method, const std::string& pathPattern,
		std::function<void(Context &)> handler) {
	routes.push_back({ method, pathPattern, handler });
}

bool Router::route(Context& ctx) {
	// ctx.getRequest() : 클라이언트로부터 받은 HTTP 요청을 반환한다.
	const auto& req = ctx.getRequest();
	// ctx.getResponse() : 서버에서 클라이언트로 보낼 HTTP 응답을 반환한다.
	std::string target = std::string(req.target());

	// prefix가 존재하면 target에서 prefix를 제거한다.
	if (!prefix.empty() && target.find(prefix) == 0) {
		target.erase(0, prefix.length());
	}

	// target이 비어있지 않고 target[0]이 '/'이면 '/'를 추가한다.
	if (!target.empty() && target[0] == '/') {
		target = '/' + target;
	}

	for (const auto& routeInfo : routes) {
		// req.method() != routeInfo.method : HTTP 요청의 메소드가 routeInfo.method와 다르다면 continue를 호출한다.
		if (req.method() != routeInfo.method)
			continue;
		
		// splitPath() 함수를 호출하여 target과 routeInfo.pathPattern을 '/'로 나눈다.
		// target : 클라이언트로부터 받은 HTTP 요청의 path
		// routeInfo.pathPattern : 라우터에 등록된 path
		std::vector<std::string> targetParts = splitPath(target);
		std::vector<std::string> patternParts = splitPath(routeInfo.pathPattern);
		

		// targetParts와 patternParts의 크기가 다르다면 continue를 호출한다.
		// Ex) targetParts = ["api", "v1", "users"], patternParts = ["api", "v1", "users"]
		if (targetParts.size() != patternParts.size())
			continue;

		// match : targetParts와 patternParts를 비교하여 일치하는지 확인하는 변수
		bool match = true;
		for (size_t i = 0; i < targetParts.size(); i++) {
			// patternParts[i]가 '{'로 시작하고 '}'로 끝나면 parameter이므로 continue를 호출한다.
			if (patternParts[i].front() == '{' && 
				patternParts[i].back() == '}') {
				// paramName : parameter의 이름
				std::string paramName = 
					patternParts[i].substr(1, patternParts[i].size() - 2);
				// setParam() 함수를 호출하여 parameter를 설정한다.
				ctx.setParam(paramName, targetParts[i]);
			}
			else if (targetParts[i] != patternParts[i]) {
				match = false;
				break;
			}
		}

		// match가 true이면 handler 함수를 호출한다.
		if (match) {
			routeInfo.handler(ctx);
			return true;
		}
	}
	return false;
}

// setPrefix() 함수는 prefix를 설정하는 함수이다.
void Router::setPrefix(const std::string& prefix) {
	this->prefix = prefix;

	// prefix가 '/'로 끝나지 않으면 '/'를 추가한다.
	if (!this->prefix.empty() && this->prefix.back() != '/') {
		this->prefix += '/';
	}
}

// splitPath() 함수는 path를 '/'로 나누어 vector<string> 형태로 반환한다.
// Ex) splitPath("/api/v1/users") -> ["api", "v1", "users"]
std::vector<std::string> Router::splitPath(const std::string& path) {
	// segments : path를 '/'로 나눈 결과를 저장하는 변수
	std::vector<std::string> segments;
	// start : path에서 '/'를 찾기 시작하는 위치
	size_t start = 0, end = 0;

	// path에서 '/'를 찾아 segments에 저장한다.
	while ((end = path.find('/', start)) != std::string::npos) {
		if (end != start) {
			segments.push_back(path.substr(start, end - start));
		}
		start = end + 1;
	}
	// start가 path.size()보다 작다면 나머지를 segments에 저장한다.
	if (start < path.size()) {
		segments.push_back(path.substr(start));
	}
	return segments;
}