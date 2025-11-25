#include <cstring>
#include <vector>
#include "Network/PacketSerializer.hpp"
#include "Network/Packet.hpp"
#include "Network/PacketFactory.hpp"

template<typename T>
std::vector<uint8_t> PacketSerializer::serialize(const T& packet) {
    std::vector<uint8_t> buffer(sizeof(T));
    std::memcpy(buffer.data(), &packet, sizeof(T));
    return buffer;
}

template<typename T>
bool PacketSerializer::deserialize(const std::vector<uint8_t>& buffer, T& packet) {
    if (buffer.size() < sizeof(T)) {
        return false;
    }
    std::memcpy(&packet, buffer.data(), sizeof(T));
    return true;
}

bool PacketSerializer::validate(const void* data, size_t size) {
    if (size < sizeof(PacketHeader)) {
        return false;
    }
    PacketType type = PacketFactory::getPacketType(data);
    if (type == PacketType::INVALID) {
        return false;
    }
    size_t expected_size = PacketFactory::getPacketSize(type);
    if (expected_size == 0 || size < expected_size) {
        return false;
    }
    return true;
}
