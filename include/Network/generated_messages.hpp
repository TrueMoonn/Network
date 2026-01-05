#pragma once
#include <cstdint>
#include <vector>
#include <cstring>

#include <string>

namespace net {

// ===== Messages =====

struct LOGIN_REQUEST {
    static constexpr uint32_t ID = 1;

    char username[32];
    char password[64];
    uint16_t client_version;

    std::vector<uint8_t> serialize() const;
    static LOGIN_REQUEST deserialize(const std::vector<uint8_t>& data);
};

struct CHAT_MESSAGE {
    static constexpr uint32_t ID = 10;

    char content[128];
    char sender[32];

    std::vector<uint8_t> serialize() const;
    static CHAT_MESSAGE deserialize(const std::vector<uint8_t>& data);
};

}  // namespace net
