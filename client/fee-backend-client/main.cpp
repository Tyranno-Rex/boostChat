#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <array>
#include <zlib.h> // CRC32 계산을 위해 zlib 라이브러리를 사용합니다.

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

const char expected_hcv = 0x02; // 예시 값, 실제 값으로 변경 필요
const char expected_tcv = 0x03; // 예시 값, 실제 값으로 변경 필요

uint32_t calculate_checksum(const std::vector<char>& data) {
    return crc32(0, reinterpret_cast<const unsigned char*>(data.data()), data.size());
}

void read_messages(tcp::socket& socket) {
    try {
        while (true) {
            std::array<char, 1> hcv;
            std::array<char, 4> checksum;
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

            // Process Payload
            std::string message(payload.begin(), payload.end());
            std::cout << "Received: " << message << std::endl;
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in read thread: " << e.what() << std::endl;
    }
}

void send_http_request(boost::asio::io_context& io_context, tcp::resolver::results_type& http_endpoints, const std::string& target, http::verb method, const std::string& body = "") {
    try {
        beast::tcp_stream stream(io_context);
        stream.connect(http_endpoints);

        http::request<http::string_body> req{ method, target, 11 };
        req.set(http::field::host, "localhost");
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.body() = body;
        req.prepare_payload();

        http::write(stream, req);

        // 응답 읽기
        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        std::cout << res << std::endl;

        // 연결 닫기
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        if (ec && ec != beast::errc::not_connected)
            throw beast::system_error{ ec };
    }
    catch (std::exception& e) {
        std::cerr << "Exception in HTTP request: " << e.what() << std::endl;
    }
}

void write_messages(tcp::socket& chat_socket, tcp::resolver::results_type& http_endpoints, boost::asio::io_context& io_context) {
    try {
        while (true) {
            std::string message;
            std::getline(std::cin, message);

            std::vector<char> payload(message.begin(), message.end());
            uint32_t payload_size = payload.size();
            uint32_t checksum = calculate_checksum(payload);

            std::array<char, 1> hcv = { expected_hcv };
            std::array<char, 4> checksum_array = *reinterpret_cast<std::array<char, 4>*>(&checksum);
            std::array<char, 4> size = *reinterpret_cast<std::array<char, 4>*>(&payload_size);
            std::array<char, 1> tcv = { expected_tcv };
               

            if (message.rfind("/", 0) == 0) { // 명령어는 '/'로 시작
                if (message == "/room") {
                    send_http_request(io_context, http_endpoints, "/api/v1/room", http::verb::get);
                }
				if (message == "/room/create") {
					send_http_request(io_context, http_endpoints, "/api/v1/room/create", http::verb::post);
				}
                else if (message.rfind("/delete", 0) == 0) {
                    send_http_request(io_context, http_endpoints, message, http::verb::delete_);
                }
                else {
                    send_http_request(io_context, http_endpoints, "/", http::verb::post, message);
                }
            }
            else { // 채팅 메시지
                boost::asio::write(chat_socket, boost::asio::buffer(hcv));
                boost::asio::write(chat_socket, boost::asio::buffer(checksum_array));
                boost::asio::write(chat_socket, boost::asio::buffer(size));
                boost::asio::write(chat_socket, boost::asio::buffer(payload));
                boost::asio::write(chat_socket, boost::asio::buffer(tcv));
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in write thread: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: client <host> <chat_port> <http_port>\n";
        return 1;
    }

    try {
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type chat_endpoints = resolver.resolve(argv[1], argv[2]);
        tcp::resolver::results_type http_endpoints = resolver.resolve(argv[1], argv[3]);

        tcp::socket chat_socket(io_context);

        boost::asio::connect(chat_socket, chat_endpoints);

        std::thread read_thread(read_messages, std::ref(chat_socket));
        std::thread write_thread(write_messages, std::ref(chat_socket), std::ref(http_endpoints), std::ref(io_context));

        read_thread.join();
        write_thread.join();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}