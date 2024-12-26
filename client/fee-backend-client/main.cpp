#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <array>
#include <zlib.h> 
#include <openssl/md5.h>
#include <openssl/evp.h>

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

const char expected_hcv = 0x02;
const char expected_tcv = 0x03; 


struct PacketHeader {
    short checkValue;
    int PacketType;
	int PacketSize;
};

struct Packet {
	PacketHeader pheader;
	char payload[128];
};


void send_http_request(boost::asio::io_context& io_context, tcp::resolver::results_type& http_endpoints, const std::string& target, http::verb method, const std::string& body = "") {
    try {
		// HTTP ��û ������
        beast::tcp_stream stream(io_context);
        stream.connect(http_endpoints);

		// ��û �ۼ�
        http::request<http::string_body> req{ method, target, 11 };
        req.set(http::field::host, "localhost");
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.body() = body;
        req.prepare_payload();

		// ��û ������
        http::write(stream, req);

        // ���� �б�
        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

		// ���� ���
        std::cout << res << std::endl;

        // ���� �ݱ�
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        if (ec && ec != beast::errc::not_connected)
            throw beast::system_error{ ec };
    }
    catch (std::exception& e) {
        std::cerr << "Exception in HTTP request: " << e.what() << std::endl;
    }
}

// �Ʒ� �ް� �Ǵ� �ּ����� ������ ó�� ���� ������� �����ϱ� ���� �ּ��Դϴ�.
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

void read_messages(tcp::socket& socket) {
    try {
        while (true) {
			// ������ ������ ũ�⸦ ���� ����
			// hcv: Header Check Value
            std::array<char, 1> hcv;
			// checksum: MD5 �ؽð�
            std::array<unsigned char, MD5_DIGEST_LENGTH> checksum;
			// size: ������ ũ��
            std::array<char, 4> size;
			// tcv: Tail Check Value
            std::array<char, 1> tcv;

			// ������ ����
            boost::asio::read(socket, boost::asio::buffer(hcv));
            boost::asio::read(socket, boost::asio::buffer(checksum));
            boost::asio::read(socket, boost::asio::buffer(size));

			// ������ ũ�⸦ �о payload ũ�⸦ �˾Ƴ�
            uint32_t payload_size = *reinterpret_cast<uint32_t*>(size.data());
            std::vector<char> payload(payload_size);

			// ������ ����
            boost::asio::read(socket, boost::asio::buffer(payload));
            boost::asio::read(socket, boost::asio::buffer(tcv));

            // ���ŵ� CheckSum ��� (������)
            std::cout << "Received checksum: ";
            for (unsigned char byte : checksum) {
                std::cout << std::hex << static_cast<int>(byte);
            }
			// ���ŵ� ������ ũ�� ��� (������)
            std::cout << std::dec << std::endl;

            std::cout << "reading" << std::endl;
            std::string message(payload.begin(), payload.end());
			// message: ���ŵ� �޽���
            std::cout << "Received: " << message << std::endl;
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception in read thread: " << e.what() << std::endl;
    }
}

void write_messages(tcp::socket& chat_socket, tcp::resolver::results_type& http_endpoints, boost::asio::io_context& io_context) {
    try {
        while (true) {
			// �޽��� �Է�
            std::string message;
            std::getline(std::cin, message);

			// �޽����� payload�� ��ȯ
            std::vector<char> payload(message.begin(), message.end());
            uint32_t payload_size = payload.size();
            auto checksum = calculate_checksum(payload);

			// hcv: Header Check Value
			// checksum: MD5 �ؽð�
			// size: ������ ũ��
			// tcv: Tail Check Value
            std::array<char, 1> hcv = { expected_hcv };
            std::array<char, 4> size = *reinterpret_cast<std::array<char, 4>*>(&payload_size);
            std::array<char, 1> tcv = { expected_tcv };

			// ��ɾ����� ä�� �޽������� �Ǵ�
            if (message.rfind("/", 0) == 0) { // ��ɾ�� '/'�� ����
                //send_http_request(io_context, http_endpoints, "/", http::verb::post, message);
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
            else { // ä�� �޽���
                boost::asio::write(chat_socket, boost::asio::buffer(hcv));
                boost::asio::write(chat_socket, boost::asio::buffer(checksum.data(), checksum.size()));
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

		// ä�� ������ HTTP ������ ���� ������ �����, ä�� ������ ���� ������ ����
        std::thread read_thread(read_messages, std::ref(chat_socket));
        std::thread write_thread(write_messages, std::ref(chat_socket), std::ref(http_endpoints), std::ref(io_context));
        
		// �����尡 ����� ������ ���
        read_thread.join();
        write_thread.join();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}