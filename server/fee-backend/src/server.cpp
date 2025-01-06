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
    // MD5��: 128��Ʈ ������ �ؽð��� �����ϴ� �ؽ� �Լ�
    // �ʿ��� ����: �������� ���Ἲ�� �����ϱ� ���� ���
    // MD5 �ؽð��� ������ �迭
    std::array<unsigned char, MD5_DIGEST_LENGTH> checksum;
    // MD5 �ؽ� ���
    // EVP_MD_CTX_new: EVP_MD_CTX ��ü ����
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    // EVP_MD_CTX_new ���� �� ���� ó��
    if (mdctx == nullptr) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    // MD5 �ؽ� �ʱ�ȭ
    // DigestInit_ex: �ؽ� �Լ� �ʱ�ȭ
    // EVP_md5: MD5 �ؽ� �Լ�
    // �ʱ�ȭ �۾� �ϴ� ���� : ������ ���� �ؽ� �Լ��� ���¸� �ʱ�ȭ�ϱ� ����
    if (EVP_DigestInit_ex(mdctx, EVP_md5(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to initialize digest");
    }

    // MD5 �ؽ� ������Ʈ
    // DigestUpdate: �����͸� �ؽ� �Լ��� ������Ʈ
    // data.data(): �������� ���� �ּ�
    // data.size(): �������� ����
    if (EVP_DigestUpdate(mdctx, data.data(), data.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to update digest");
    }

    // MD5 �ؽ� ����ȭ
    unsigned int length = 0;
    // DigestFinal_ex: �ؽ� �Լ��� �����ϰ� ����� ����
    // checksum.data(): �ؽ� ����� ������ �迭
    // length: �ؽ� ����� ����
    if (EVP_DigestFinal_ex(mdctx, checksum.data(), &length) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to finalize digest");
    }

    EVP_MD_CTX_free(mdctx);
    return checksum;
}

void Server::chatSession(tcp::socket socket) {
    try {
		// Ŭ���̾�Ʈ ������ ���� �����ͷ� ��ȯ
        auto client_socket = std::make_shared<tcp::socket>(std::move(socket));
        {
            std::cout << "client connected" << std::endl;
            std::cout << "Client IP: " << client_socket->remote_endpoint().address().to_string() << std::endl;
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.push_back(client_socket);
        }

        while (true) {
			// hcv, checksum, size, payload, tcv�� �б� ���� �迭 ����
            std::array<char, 1> hcv;
			std::array<char, MD5_DIGEST_LENGTH> checksum;
            std::array<char, 4> size;
            std::array<char, 1> tcv;

            // �޽��� �б�
            boost::asio::read(*client_socket, boost::asio::buffer(hcv));
            boost::asio::read(*client_socket, boost::asio::buffer(checksum));
            boost::asio::read(*client_socket, boost::asio::buffer(size));

			// payload: �޽����� ���� ������
			// payload_size: payload�� ũ��
			// reinterpret_cast: ������ ������ ��ȯ
            uint32_t payload_size = *reinterpret_cast<uint32_t*>(size.data());
            std::vector<char> payload(payload_size);

			// payload �б�
			// boost::asio::read: �񵿱�� �����͸� ����
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

    // ���ο� �޽��� ��Ŷ ����
    std::array<char, 1> hcv = { expected_hcv };
    std::array<char, 1> tcv = { expected_tcv };
    // checksum ���
    auto checksum = calculate_checksum(original_payload);
    // checksum�� �迭�� ��ȯ
    std::array<char, MD5_DIGEST_LENGTH> checksum_array = *reinterpret_cast<std::array<char, MD5_DIGEST_LENGTH>*>(&checksum);

    for (auto it = clients.begin(); it != clients.end(); ) {
        try {
            boost::asio::write(**it, boost::asio::buffer(hcv));
            boost::asio::write(**it, boost::asio::buffer(checksum_array));

            // � Ŭ���̾�Ʈ�� ���´��� �˱� ���� "client_ip : payload" ���·� ��ȯ
            std::string payload_str(original_payload.begin(), original_payload.end());
            std::string client_ip = (*it)->remote_endpoint().address().to_string();
            payload_str = client_ip + " : " + payload_str;
            std::vector<char> payload(payload_str.begin(), payload_str.end());

            // payload size�� 4����Ʈ �迭�� ��ȯ
            uint32_t payload_size = payload.size();
            std::array<char, 4> size = *reinterpret_cast<std::array<char, 4>*>(&payload_size);

            boost::asio::write(**it, boost::asio::buffer(size));
            boost::asio::write(**it, boost::asio::buffer(payload));
            boost::asio::write(**it, boost::asio::buffer(tcv));
            ++it;
        }
        catch (std::exception& e) {
            // Ŭ���̾�Ʈ ����
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
	// tcp::acceptor ��ü ����
	// acceptor: Ŭ���̾�Ʈ�� ������ �����ϴ� ��ü
	// io_context: �񵿱� I/O �۾��� ó���ϴ� ��ü
	// port: ������ ��Ʈ ��ȣ
	// v4(): IPv4 �ּ� ü�� ���
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