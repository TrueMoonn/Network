#pragma once

#include <vector>
#include <cstdint>

class PacketSerializer {
 public:
    // Convert a struct to vec<uint8_t>
    template<typename T>
    static std::vector<uint8_t> serialize(const T& packet);

    // Convert vec<uint8_t> to struct
    template<typename T>
    static bool deserialize(const std::vector<uint8_t>& buffer, T& packet);

    // Check if a packet is valid
    static bool validate(const void* data, size_t size);
};
