#include "../include/server.hpp"

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

uint32_t calculate_checksum(const std::vector<char>& data) {
    return crc32(0, reinterpret_cast<const unsigned char*>(data.data()), data.size());
}

void Server::chatSession(tcp::socket socket) {
    try {
        auto client_socket = std::make_shared<tcp::socket>(std::move(socket));
        {
            std::cout << "client connected" << std::endl;
            std::cout << "Client IP: " << client_socket->remote_endpoint().address().to_string() << std::endl;
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.push_back(client_socket);
        }

        while (true) {
            std::array<char, 1> hcv;
            std::array<char, 4> checksum;
            std::array<char, 4> size;
            std::array<char, 1> tcv;

            // 메시지 읽기
            boost::asio::read(*client_socket, boost::asio::buffer(hcv));
            boost::asio::read(*client_socket, boost::asio::buffer(checksum));
            boost::asio::read(*client_socket, boost::asio::buffer(size));

            uint32_t payload_size = *reinterpret_cast<uint32_t*>(size.data());
            std::vector<char> payload(payload_size);

            boost::asio::read(*client_socket, boost::asio::buffer(payload));
            boost::asio::read(*client_socket, boost::asio::buffer(tcv));

            std::cout << "received message: " << std::string(payload.begin(), payload.end()) << std::endl;

            // 원본 페이로드만 브로드캐스트
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
    uint32_t checksum = calculate_checksum(original_payload);
    std::array<char, 4> checksum_array = *reinterpret_cast<std::array<char, 4>*>(&checksum);
    uint32_t payload_size = original_payload.size();
    std::array<char, 4> size = *reinterpret_cast<std::array<char, 4>*>(&payload_size);
    std::array<char, 1> tcv = { expected_tcv };

    // 메시지 순서대로 전송
    for (auto& client : clients) {
        try {
            boost::asio::write(*client, boost::asio::buffer(hcv));
            boost::asio::write(*client, boost::asio::buffer(checksum_array));
            boost::asio::write(*client, boost::asio::buffer(size));
            boost::asio::write(*client, boost::asio::buffer(original_payload));
            boost::asio::write(*client, boost::asio::buffer(tcv));
        }
        catch (std::exception& e) {
            std::cerr << "Error sending to client: " << e.what() << std::endl;
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