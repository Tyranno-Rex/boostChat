#include "../include/server.hpp"

std::map<std::string, std::map<std::string, std::string>> g_chatRooms;

void Server::httpSession(tcp::socket socket) {
	try {
		// bufffer			: ������ �޸� ������ �ǹ��Ѵ�. Ŭ���̾�Ʈ�κ��� ���� �����͸� �����ϴ� �����̴�.
		beast::flat_buffer buffer;
		// req				: Ŭ���̾�Ʈ�κ��� ���� HTTP ��û�� �����ϴ� ����
		http::request<http::string_body> req;
		// read				: Ŭ���̾�Ʈ�κ��� ���� HTTP ��û�� �о���� �Լ�
		http::read(socket, buffer, req);
		// response			: �������� Ŭ���̾�Ʈ�� ���� HTTP ������ �����ϴ� ����
		http::response<http::string_body> res;
		// ctx 				: Ŭ���̾�Ʈ�κ��� ���� HTTP ��û�� ó���ϴµ� �ʿ��� ������ �����ϴ� ����
		auto ctx = Context(req, res);

		// route() �Լ��� Ŭ���̾�Ʈ�κ��� ���� HTTP ��û�� ó���ϴ� �Լ��̴�.
		if (!router->route(ctx)) {
			res.result(http::status::not_found);
			res.set(http::field::content_type, "text/plain");
			res.body() = "The resource '" + std::string(req.target()) + "' was not found.";
		}

		// res.body().length() != 0 : Ŭ���̾�Ʈ�� ���� �����Ͱ� �ִٸ� prepare_payload() �Լ��� ȣ���Ѵ�.
		// prepare_payload() �Լ��� HTTP ������ ���� �غ� �ϴ� �Լ��̴�.
		if (res.body().length() != 0) 
			res.prepare_payload();

		// write() �Լ��� Ŭ���̾�Ʈ�� HTTP ������ ������ �Լ��̴�.
		http::write(socket, res);
	} catch (std::exception& e) {
		std::cerr << "Exception in thread: " << e.what() << std::endl;
	}
}

uint32_t calculate_checksum(std::vector<char>& data) {
	return crc32(0, reinterpret_cast<const unsigned char*>(data.data()), data.size());
}

void Server::chatSession(tcp::socket socket) {
    try {
        while (true) {
            std::array<char, 1> hcv;
            std::array<char, 16> checksum;
            std::array<char, 4> size;
            std::array<char, 1> tcv;

            // Read HCV
            boost::asio::read(socket, boost::asio::buffer(hcv));
            if (hcv[0] != expected_hcv) {
                std::cerr << "Invalid HCV" << std::endl;
                return;
            }

            // Read CheckSum
            boost::asio::read(socket, boost::asio::buffer(checksum));

            // Read Size
            boost::asio::read(socket, boost::asio::buffer(size));
            uint32_t payload_size = *reinterpret_cast<uint32_t*>(size.data());

            // Read Payload
            std::vector<char> payload(payload_size);
            boost::asio::read(socket, boost::asio::buffer(payload));

            // Read TCV
            boost::asio::read(socket, boost::asio::buffer(tcv));
            if (tcv[0] != expected_tcv) {
                std::cerr << "Invalid TCV" << std::endl;
                return;
            }

			uint32_t received_checksum = *reinterpret_cast<uint32_t*>(checksum.data());
			uint32_t calculated_checksum = calculate_checksum(payload);
			if (received_checksum != calculated_checksum) {
				std::cerr << "Invalid Checksum" << std::endl;
				return;
			}

            // Echo the message back to the client
            //boost::asio::write(socket, boost::asio::buffer(hcv));
            //boost::asio::write(socket, boost::asio::buffer(checksum));
            //boost::asio::write(socket, boost::asio::buffer(size));
            //boost::asio::write(socket, boost::asio::buffer(payload));
            //boost::asio::write(socket, boost::asio::buffer(tcv));
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << std::endl;
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