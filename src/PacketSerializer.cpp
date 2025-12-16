#include <cstring>
#include <vector>
#include "Network/PacketSerializer.hpp"
#include "Network/Packet.hpp"
#include "Network/PacketFactory.hpp"

namespace net {

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

}  // namespace net
