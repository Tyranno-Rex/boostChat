#include <iostream>
#include <array>
#include <vector>
#include <thread>
#include <cstdint>
#include <zlib.h>
#include <chrono>

#include <openssl/md5.h>
#include <openssl/evp.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

const char expected_hcv = 0x02;
const char expected_tcv = 0x03;

enum class PacketType : uint8_t {
    defEchoString = 100,
	JH = 101,
	YJ = 102,
	ES = 103,
};

#pragma pack(push, 1)
struct PacketHeader {
    PacketType type;           // �⺻ : 100
    char checkSum[16];
    uint32_t size;
};

struct PacketTail {
    uint8_t value;
};

struct Packet {
    PacketHeader header;
    char payload[128];
    PacketTail tail;              // �⺻ : 255
};
#pragma pack(pop)

//It is a truth universally acknowledged, that a single man in possession of a good fortune, must be in want of a wife.However lit...
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

void send_message(tcp::socket& socket, const std::string& message) {
    try {
        Packet packet;
		packet.header.type = PacketType::ES;
        packet.tail.value = 255; // �⺻ ��

        // payload �ʱ�ȭ
        std::memset(packet.payload, 0, sizeof(packet.payload));
        // message�� payload�� ���� (128����Ʈ������)
        std::memcpy(packet.payload, message.data(), (size_t)128);

        // ���� �޽��� ũ�� ����
        //packet.header.size = static_cast<uint32_t>(std::min(message.length(), (size_t)128));
        packet.header.size = static_cast<uint32_t>(std::min(message.length(), (size_t)128));

        // checksum ���
        auto checksum = calculate_checksum(std::vector<char>(message.begin(), message.end()));
        std::memcpy(packet.header.checkSum, checksum.data(), MD5_DIGEST_LENGTH);

        // ��Ŷ ����
        boost::asio::write(socket, boost::asio::buffer(&packet, sizeof(packet)));
        std::cout << "Message sent\n";
    }
    catch (std::exception& e) {
        std::cerr << "Error sending message: " << e.what() << std::endl;
    }
}

void handle_sockets(boost::asio::io_context& io_context, const std::string& host, const std::string& port, uint8_t socket_cnt, const std::string& message) {
    try {
        //std::vector<std::thread> threads;
		std::vector<std::future<void>> futures;
        std::vector<std::shared_ptr<tcp::socket>> sockets(socket_cnt);

        for (uint8_t i = 0; i < socket_cnt; ++i) {
            auto socket = std::make_shared<tcp::socket>(io_context);
            tcp::resolver resolver(io_context);
            auto endpoints = resolver.resolve(host, port);

			auto promise = std::make_shared<std::promise<void>>();
			futures.push_back(promise->get_future());


            // �񵿱� ���� ����
            boost::asio::async_connect(*socket, endpoints,
                [socket, message, promise](const boost::system::error_code& ec, const tcp::endpoint&) {
                    if (!ec) {
                        send_message(*socket, message);
                    }
                    else {
                        std::cerr << "Error connecting: " << ec.message() << std::endl;
                    }
					promise->set_value();
                });
            sockets[i] = socket;
        }
		for (auto& future : futures) {
			future.get();
		}
    }
    catch (std::exception& e) {
        std::cerr << "Error handling sockets: " << e.what() << std::endl;
    }
}

void write_messages(boost::asio::io_context& io_context, const std::string& host, const std::string& port) {
    try {
		std::thread io_thread([&io_context]() { io_context.run(); });

        while (true) {
            std::string message;
            int thread_cnt, socket_cnt;

			std::cout << "Enter command: 1 ~ 3 or /debug <message>\n";
            std::getline(std::cin, message);

			if (message == "1") {
                message = "It is a truth universally acknowledged, that a single man in possession of a good fortune, must be in want of a wife.However lit...";
                thread_cnt = 10;
				socket_cnt = 10;

				if (message.size() > 128) {
					message = message.substr(0, 128);
				}

				boost::asio::thread_pool pool(thread_cnt);
				for (int i = 0; i < thread_cnt; ++i) {
					boost::asio::post(pool, [&io_context, &host, &port, socket_cnt, message]() {
						handle_sockets(io_context, host, port, socket_cnt, message);
						});
				}
				pool.join();
			} else if (message == "2") {
                message = "In a quiet corner of the sprawling forest, where sunlight barely reached the ground, a small fox watched as shadows danced on the leaves.";
                thread_cnt = 20;
				socket_cnt = 10;

				if (message.size() > 128) {
					message = message.substr(0, 128);
				}

				boost::asio::thread_pool pool(thread_cnt);
				for (int i = 0; i < thread_cnt; ++i) {
					boost::asio::post(pool, [&io_context, &host, &port, socket_cnt, message]() {
						handle_sockets(io_context, host, port, socket_cnt, message);
						});
				}
				pool.join();
			} else if (message == "3") {
				message = "The sun was setting, casting a warm glow over the city as the last of the day's light faded away. the streets were quiet, the only sound the distant hum of traffic.";
                thread_cnt = 50;
				socket_cnt = 10;

				if (message.size() > 128) {
					message = message.substr(0, 128);
				}

				boost::asio::thread_pool pool(thread_cnt);
				for (int i = 0; i < thread_cnt; ++i) {
					boost::asio::post(pool, [&io_context, &host, &port, socket_cnt, message]() {
						handle_sockets(io_context, host, port, socket_cnt, message);
						});
				}
				pool.join();
			} else if (message.rfind("/debug", 0) == 0) {
                std::cout << "Write Thread Count: ";
                std::cin >> thread_cnt;
                std::cout << "Socket Count: ";
                std::cin >> socket_cnt;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                std::string debug_message = message.substr(7);
                if (debug_message.size() > 128) {
                    debug_message = debug_message.substr(0, 128);
                }

                //while (true) {
                    boost::asio::thread_pool pool(thread_cnt);
                    for (int i = 0; i < thread_cnt; ++i) {
                        boost::asio::post(pool, [&io_context, &host, &port, socket_cnt, debug_message]() {
                            handle_sockets(io_context, host, port, socket_cnt, debug_message);
                            });
                    }
                    pool.join();
                //}
            }
            message.clear();
        }
		io_context.stop();
		io_thread.join();
    }
    catch (std::exception& e) {
        std::cerr << "Exception in write thread: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    

    if (argc != 4) {
        std::cerr << "Usage: client <host> <chat_port> <http_port>\n";
        return 1;
    }

    try {
        boost::asio::io_context io_context;
        const std::string host = argv[1];
        const std::string chat_port = argv[2];
        //const std::string http_port = argv[3];

        write_messages(io_context, host, chat_port);
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
