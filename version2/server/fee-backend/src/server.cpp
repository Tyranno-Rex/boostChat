#include "../include/server.hpp"
#include "../include/packet.hpp"
#include <openssl/md5.h>
#include <openssl/evp.h>

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

void Server::handleAccept(std::shared_ptr<tcp::socket> socket, tcp::acceptor& acceptor) {
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.push_back(socket);
    }
    chatSession(socket);
    auto next_socket = std::make_shared<tcp::socket>(io_context);
    acceptor.async_accept(*next_socket,
        std::bind(&Server::handleAccept, this, next_socket, std::ref(acceptor)));
}

void Server::chatSession(std::shared_ptr<tcp::socket> client_socket) {
    auto temp_buffer = std::make_shared<std::array<char, 1024>>();
    auto packet_buffer = std::make_shared<PacketBuffer>();

    client_socket->async_read_some(
        boost::asio::buffer(*temp_buffer),
        std::bind(&Server::handleRead, this,
            std::placeholders::_1,
            std::placeholders::_2,
            client_socket,
            temp_buffer,
            packet_buffer));
}

void Server::handleRead(const boost::system::error_code& error,
    size_t bytes_transferred,
    std::shared_ptr<tcp::socket> client_socket,
    std::shared_ptr<std::array<char, 1024>> temp_buffer,
    std::shared_ptr<PacketBuffer> packet_buffer) {

    if (!error) {
        if (bytes_transferred > 0) {
            packet_buffer->append(temp_buffer->data(), bytes_transferred);

            while (auto maybe_packet = packet_buffer->extractPacket()) {
                Packet& packet = *maybe_packet;
                std::vector<char> payload_data(packet.payload,
                    packet.payload + packet.header.size);

                auto calculated_checksum = calculate_checksum(payload_data);
                bool checksum_valid = std::memcmp(packet.header.checkSum,
                    calculated_checksum.data(), MD5_DIGEST_LENGTH) == 0;

                if (!checksum_valid) {
                    std::cerr << "Checksum validation failed for packet" << std::endl;
                    continue;
                }

                std::string message(packet.payload, packet.header.size);
                std::cout << message << "\n";
            }
        }

        client_socket->async_read_some(
            boost::asio::buffer(*temp_buffer),
            std::bind(&Server::handleRead, this,
                std::placeholders::_1,
                std::placeholders::_2,
                client_socket,
                temp_buffer,
                packet_buffer));
    }
    else {
        if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
        }
        else {
            std::cerr << "Error in chat session: " << error.message() << std::endl;
        }
    }
}

void Server::chatRun() {
    try {
        tcp::acceptor acceptor(io_context, { tcp::v4(), static_cast<boost::asio::ip::port_type>(port) });

        auto first_socket = std::make_shared<tcp::socket>(io_context);
        acceptor.async_accept(*first_socket,
            std::bind(&Server::handleAccept, this, first_socket, std::ref(acceptor)));

        io_context.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error in chatRun: " << e.what() << std::endl;
    }
}