#pragma once
#include <vector>
#include <optional>

// PacketType: ��Ŷ�� ������ ���� ����
enum class PacketType : uint8_t {
    defEchoString = 100,
};

// pack(push, 1): 
#pragma pack(push, 1)

struct PacketHeader {
	PacketType type;           // ��Ŷ Ÿ��: 100
    char checkSum[16];
    uint32_t size;
};

struct PacketTail {
    uint8_t value;
};

struct Packet {
    PacketHeader header;
    char payload[128];
    PacketTail tail;              // �� 
};
#pragma pack(pop)


class PacketBuffer {
private:
    std::vector<char> buffer;
    const size_t packet_size;

public:
    PacketBuffer() : packet_size(sizeof(Packet)) {
        buffer.reserve(packet_size * 2);  // �ּ� 2�� ��Ŷ ũ��� ����
    }

    void append(const char* data, size_t length) {
        buffer.insert(buffer.end(), data, data + length);
    }

    bool hasCompletePacket() const {
        return buffer.size() >= packet_size;
    }

    // ������ ��Ŷ�� �����ϰ� �������� ���ۿ� ����
    std::optional<Packet> extractPacket() {
        if (buffer.size() < packet_size) {
            return std::nullopt;
        }

        Packet packet;
        std::memcpy(&packet, buffer.data(), packet_size);

        // ó���� �����͸� ���ۿ��� ����
        buffer.erase(buffer.begin(), buffer.begin() + packet_size);

        return packet;
    }

    size_t size() const {
        return buffer.size();
    }

    void clear() {
        buffer.clear();
    }
};