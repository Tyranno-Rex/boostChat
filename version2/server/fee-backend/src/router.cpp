#include "../include/router.hpp"
#include <iostream>

void Router::addRoute(http::verb method, const std::string& pathPattern,
		std::function<void(Context &)> handler) {
	routes.push_back({ method, pathPattern, handler });
}

bool Router::route(Context& ctx) {
	// ctx.getRequest() : Ŭ���̾�Ʈ�κ��� ���� HTTP ��û�� ��ȯ�Ѵ�.
	const auto& req = ctx.getRequest();
	// ctx.getResponse() : �������� Ŭ���̾�Ʈ�� ���� HTTP ������ ��ȯ�Ѵ�.
	std::string target = std::string(req.target());

	// prefix�� �����ϸ� target���� prefix�� �����Ѵ�.
	if (!prefix.empty() && target.find(prefix) == 0) {
		target.erase(0, prefix.length());
	}

	// target�� ������� �ʰ� target[0]�� '/'�̸� '/'�� �߰��Ѵ�.
	if (!target.empty() && target[0] == '/') {
		target = '/' + target;
	}

	for (const auto& routeInfo : routes) {
		// req.method() != routeInfo.method : HTTP ��û�� �޼ҵ尡 routeInfo.method�� �ٸ��ٸ� continue�� ȣ���Ѵ�.
		if (req.method() != routeInfo.method)
			continue;
		
		// splitPath() �Լ��� ȣ���Ͽ� target�� routeInfo.pathPattern�� '/'�� ������.
		// target : Ŭ���̾�Ʈ�κ��� ���� HTTP ��û�� path
		// routeInfo.pathPattern : ����Ϳ� ��ϵ� path
		std::vector<std::string> targetParts = splitPath(target);
		std::vector<std::string> patternParts = splitPath(routeInfo.pathPattern);
		

		// targetParts�� patternParts�� ũ�Ⱑ �ٸ��ٸ� continue�� ȣ���Ѵ�.
		// Ex) targetParts = ["api", "v1", "users"], patternParts = ["api", "v1", "users"]
		if (targetParts.size() != patternParts.size())
			continue;

		// match : targetParts�� patternParts�� ���Ͽ� ��ġ�ϴ��� Ȯ���ϴ� ����
		bool match = true;
		for (size_t i = 0; i < targetParts.size(); i++) {
			// patternParts[i]�� '{'�� �����ϰ� '}'�� ������ parameter�̹Ƿ� continue�� ȣ���Ѵ�.
			if (patternParts[i].front() == '{' && 
				patternParts[i].back() == '}') {
				// paramName : parameter�� �̸�
				std::string paramName = 
					patternParts[i].substr(1, patternParts[i].size() - 2);
				// setParam() �Լ��� ȣ���Ͽ� parameter�� �����Ѵ�.
				ctx.setParam(paramName, targetParts[i]);
			}
			else if (targetParts[i] != patternParts[i]) {
				match = false;
				break;
			}
		}

		// match�� true�̸� handler �Լ��� ȣ���Ѵ�.
		if (match) {
			routeInfo.handler(ctx);
			return true;
		}
	}
	return false;
}

// setPrefix() �Լ��� prefix�� �����ϴ� �Լ��̴�.
void Router::setPrefix(const std::string& prefix) {
	this->prefix = prefix;

	// prefix�� '/'�� ������ ������ '/'�� �߰��Ѵ�.
	if (!this->prefix.empty() && this->prefix.back() != '/') {
		this->prefix += '/';
	}
}

// splitPath() �Լ��� path�� '/'�� ������ vector<string> ���·� ��ȯ�Ѵ�.
// Ex) splitPath("/api/v1/users") -> ["api", "v1", "users"]
std::vector<std::string> Router::splitPath(const std::string& path) {
	// segments : path�� '/'�� ���� ����� �����ϴ� ����
	std::vector<std::string> segments;
	// start : path���� '/'�� ã�� �����ϴ� ��ġ
	size_t start = 0, end = 0;

	// path���� '/'�� ã�� segments�� �����Ѵ�.
	while ((end = path.find('/', start)) != std::string::npos) {
		if (end != start) {
			segments.push_back(path.substr(start, end - start));
		}
		start = end + 1;
	}
	// start�� path.size()���� �۴ٸ� �������� segments�� �����Ѵ�.
	if (start < path.size()) {
		segments.push_back(path.substr(start));
	}
	return segments;
}