#pragma once

#include "Packet.hpp"

namespace net {

class PacketFactory {
 public:
    template<typename T>
    static T createPacket(PacketType type);

    static PacketType getPacketType(const void* data);
    static size_t getPacketSize(PacketType type);

 private:
    static uint32_t _sequence_counter;
};

}  // namespace net
