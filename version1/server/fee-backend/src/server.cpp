#include "../include/server.hpp"
#include <openssl/md5.h>
#include <openssl/evp.h>

std::map<std::string, std::map<std::string, std::string>> g_chatRooms;

void Server::httpSession(tcp::socket socket) {
    try {
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(socket, buffer, req);
        http::response<http::string_body> res;

        std::string client_ip = socket.remote_endpoint().address().to_string();
        std::cout << "Client IP: " << client_ip << std::endl;

        auto ctx = Context(req, res);
        ctx.setClientIP(client_ip);

        if (!router->route(ctx)) {
            res.result(http::status::not_found);
            res.set(http::field::content_type, "text/plain");
            res.body() = "The resource '" + std::string(req.target()) + "' was not found.";
        }

        if (res.body().length() != 0)
            res.prepare_payload();

        http::write(socket, res);
    }
    catch (std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << std::endl;
    }
}

std::array<unsigned char, MD5_DIGEST_LENGTH> calculate_checksum(const std::vector<char>& data) {
    // MD5란: 128비트 길이의 해시값을 생성하는 해시 함수
    // 필요한 이유: 데이터의 무결성을 보장하기 위해 사용
    // MD5 해시값을 저장할 배열
    std::array<unsigned char, MD5_DIGEST_LENGTH> checksum;
    // MD5 해시 계산
    // EVP_MD_CTX_new: EVP_MD_CTX 객체 생성
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    // EVP_MD_CTX_new 실패 시 예외 처리
    if (mdctx == nullptr) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    // MD5 해시 초기화
    // DigestInit_ex: 해시 함수 초기화
    // EVP_md5: MD5 해시 함수
    // 초기화 작업 하는 이유 : 이전에 사용된 해시 함수의 상태를 초기화하기 위해
    if (EVP_DigestInit_ex(mdctx, EVP_md5(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to initialize digest");
    }

    // MD5 해시 업데이트
    // DigestUpdate: 데이터를 해시 함수에 업데이트
    // data.data(): 데이터의 시작 주소
    // data.size(): 데이터의 길이
    if (EVP_DigestUpdate(mdctx, data.data(), data.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to update digest");
    }

    // MD5 해시 최종화
    unsigned int length = 0;
    // DigestFinal_ex: 해시 함수를 종료하고 결과를 저장
    // checksum.data(): 해시 결과를 저장할 배열
    // length: 해시 결과의 길이
    if (EVP_DigestFinal_ex(mdctx, checksum.data(), &length) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to finalize digest");
    }

    EVP_MD_CTX_free(mdctx);
    return checksum;
}

void Server::chatSession(tcp::socket socket) {
    try {
		// 클라이언트 소켓을 공유 포인터로 변환
        auto client_socket = std::make_shared<tcp::socket>(std::move(socket));
        {
            std::cout << "client connected" << std::endl;
            std::cout << "Client IP: " << client_socket->remote_endpoint().address().to_string() << std::endl;
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.push_back(client_socket);
        }

        while (true) {
			// hcv, checksum, size, payload, tcv를 읽기 위한 배열 생성
            std::array<char, 1> hcv;
			std::array<char, MD5_DIGEST_LENGTH> checksum;
            std::array<char, 4> size;
            std::array<char, 1> tcv;

            // 메시지 읽기
            boost::asio::read(*client_socket, boost::asio::buffer(hcv));
            boost::asio::read(*client_socket, boost::asio::buffer(checksum));
            boost::asio::read(*client_socket, boost::asio::buffer(size));

			// payload: 메시지의 실제 데이터
			// payload_size: payload의 크기
			// reinterpret_cast: 데이터 형식을 변환
            uint32_t payload_size = *reinterpret_cast<uint32_t*>(size.data());
            std::vector<char> payload(payload_size);

			// payload 읽기
			// boost::asio::read: 비동기로 데이터를 읽음
            boost::asio::read(*client_socket, boost::asio::buffer(payload));
            boost::asio::read(*client_socket, boost::asio::buffer(tcv));

            broadcastMessage(payload);
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << std::endl;
    }
}

void Server::broadcastMessage(const std::vector<char>& original_payload) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    std::cout << "broadcasting message" << std::endl;

    // 새로운 메시지 패킷 구성
    std::array<char, 1> hcv = { expected_hcv };
    std::array<char, 1> tcv = { expected_tcv };
    // checksum 계산
    auto checksum = calculate_checksum(original_payload);
    // checksum을 배열로 변환
    std::array<char, MD5_DIGEST_LENGTH> checksum_array = *reinterpret_cast<std::array<char, MD5_DIGEST_LENGTH>*>(&checksum);

    for (auto it = clients.begin(); it != clients.end(); ) {
        try {
            boost::asio::write(**it, boost::asio::buffer(hcv));
            boost::asio::write(**it, boost::asio::buffer(checksum_array));

            // 어떤 클라이언트가 보냈는지 알기 위해 "client_ip : payload" 형태로 변환
            std::string payload_str(original_payload.begin(), original_payload.end());
            std::string client_ip = (*it)->remote_endpoint().address().to_string();
            payload_str = client_ip + " : " + payload_str;
            std::vector<char> payload(payload_str.begin(), payload_str.end());

            // payload size를 4바이트 배열로 변환
            uint32_t payload_size = payload.size();
            std::array<char, 4> size = *reinterpret_cast<std::array<char, 4>*>(&payload_size);

            boost::asio::write(**it, boost::asio::buffer(size));
            boost::asio::write(**it, boost::asio::buffer(payload));
            boost::asio::write(**it, boost::asio::buffer(tcv));
            ++it;
        }
        catch (std::exception& e) {
            // 클라이언트 제거
            std::cerr << "Error sending to client: " << e.what() << std::endl;
            it = clients.erase(it);
        }
    }
}

void Server::run() {
    tcp::acceptor acceptor(
        io_context, { tcp::v4(), static_cast<boost::asio::ip::port_type>(port) });
    while (true) {
        tcp::socket socket{ io_context };
        acceptor.accept(socket);
        std::thread(&Server::httpSession, this, std::move(socket)).detach();
    }
}

void Server::chatRun() {
	// tcp::acceptor 객체 생성
	// acceptor: 클라이언트의 연결을 수락하는 객체
	// io_context: 비동기 I/O 작업을 처리하는 객체
	// port: 서버의 포트 번호
	// v4(): IPv4 주소 체계 사용
    tcp::acceptor acceptor(
        io_context, { tcp::v4(), static_cast<boost::asio::ip::port_type>(port) });
    while (true) {
        tcp::socket socket{ io_context };
        acceptor.accept(socket);
        std::thread(&Server::chatSession, this, std::move(socket)).detach();
    }
}

short Server::getPort() {
    return port;
}