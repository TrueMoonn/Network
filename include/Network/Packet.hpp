#pragma once

#include <cstdint>

namespace net {

enum class PacketType : uint8_t {
    INVALID = 0,
    PLAYER_JOIN,        // Player ask to join
    PLAYER_LEAVE,
    PLAYER_POSITION,    // Update player pos
    PLAYER_ACTION,      // One fo player possible action
    GAME_STATE,
    PING,               // Request ping (get latency)
    PONG                // ping response
};

// Force 1 byte alignment to avoid compiler padding
// Thanks to that, sizeof() matches the exact bytes sent over the wire
#pragma pack(push, 1)

// At the beginning of all packets
struct PacketHeader {
    PacketType type;
    uint32_t sequence_number;   // Number incremented at each new packet
    uint32_t timestamp;         // Timestamp
};

struct PlayerJoinPacket {
    PacketHeader header;
    char player_name[32];
};

struct PlayerPositionPacket {
    PacketHeader header;
    uint32_t player_id;
    float x;
    float y;
    float rotation;
    float velocity_x;
    float velocity_y;
};

struct PlayerActionPacket {
    PacketHeader header;
    uint32_t player_id;
    uint8_t action_type;
    float target_x;
    float target_y;
};

#pragma pack(pop)  // Restore default alignment

}  // namespace net
