#include "../include/server.hpp"

std::map<std::string, std::map<std::string, std::string>> g_chatRooms;

void Server::httpSession(tcp::socket socket) {
	try {
		// bufffer			: 서버의 메모리 공간을 의미한다. 클라이언트로부터 받은 데이터를 저장하는 공간이다.
		beast::flat_buffer buffer;
		// req				: 클라이언트로부터 받은 HTTP 요청을 저장하는 변수
		http::request<http::string_body> req;
		// read				: 클라이언트로부터 받은 HTTP 요청을 읽어오는 함수
		http::read(socket, buffer, req);
		// response			: 서버에서 클라이언트로 보낼 HTTP 응답을 저장하는 변수
		http::response<http::string_body> res;
		// ctx 				: 클라이언트로부터 받은 HTTP 요청을 처리하는데 필요한 정보를 저장하는 변수
		auto ctx = Context(req, res);

		// route() 함수는 클라이언트로부터 받은 HTTP 요청을 처리하는 함수이다.
		if (!router->route(ctx)) {
			res.result(http::status::not_found);
			res.set(http::field::content_type, "text/plain");
			res.body() = "The resource '" + std::string(req.target()) + "' was not found.";
		}

		// res.body().length() != 0 : 클라이언트로 보낼 데이터가 있다면 prepare_payload() 함수를 호출한다.
		// prepare_payload() 함수는 HTTP 응답을 보낼 준비를 하는 함수이다.
		if (res.body().length() != 0) 
			res.prepare_payload();

		// write() 함수는 클라이언트로 HTTP 응답을 보내는 함수이다.
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