#pragma once
#include <vector>
#include <optional>

// PacketType: 패킷을 구분을 위한 변수
enum class PacketType : uint8_t {
    defEchoString = 100,
};

// pack(push, 1): 
#pragma pack(push, 1)

struct PacketHeader {
	PacketType type;           // 패킷 타입: 100
    char checkSum[16];
    uint32_t size;
};

struct PacketTail {
    uint8_t value;
};

struct Packet {
    PacketHeader header;
    char payload[128];
    PacketTail tail;              // 총 
};
#pragma pack(pop)


class PacketBuffer {
private:
    std::vector<char> buffer;
    const size_t packet_size;

public:
    PacketBuffer() : packet_size(sizeof(Packet)) {
        buffer.reserve(packet_size * 2);  // 최소 2개 패킷 크기로 예약
    }

    void append(const char* data, size_t length) {
        buffer.insert(buffer.end(), data, data + length);
    }

    bool hasCompletePacket() const {
        return buffer.size() >= packet_size;
    }

    // 완전한 패킷을 추출하고 나머지는 버퍼에 유지
    std::optional<Packet> extractPacket() {
        if (buffer.size() < packet_size) {
            return std::nullopt;
        }

        Packet packet;
        std::memcpy(&packet, buffer.data(), packet_size);

        // 처리된 데이터를 버퍼에서 제거
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