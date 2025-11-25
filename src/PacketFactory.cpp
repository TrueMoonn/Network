#include <cstring>
#include "Network/NetworkUtils.hpp"
#include "Network/PacketFactory.hpp"

uint32_t PacketFactory::_sequence_counter = 0;

template<typename T>
T PacketFactory::createPacket(PacketType type) {
    T packet;

    std::memset(&packet, 0, sizeof(T));
    packet.header.type = type;
    packet.header.sequence_number = _sequence_counter++;
    packet.header.timestamp = NetworkUtils::getCurrentTimestamp();
    return packet;
}

PacketType PacketFactory::getPacketType(const void* data) {
    if (data == nullptr) {
        return PacketType::INVALID;
    }
    const PacketHeader* header = static_cast<const PacketHeader*>(data);
    return header->type;
}

size_t PacketFactory::getPacketSize(PacketType type) {
    switch (type) {
        case PacketType::PLAYER_JOIN:
            return sizeof(PlayerJoinPacket);
        case PacketType::PLAYER_LEAVE:
            return sizeof(PacketHeader);
        case PacketType::PLAYER_POSITION:
            return sizeof(PlayerPositionPacket);
        case PacketType::PLAYER_ACTION:
            return sizeof(PlayerActionPacket);
        case PacketType::PING:
        case PacketType::PONG:
            return sizeof(PacketHeader);
        default:
            return 0;
    }
}
