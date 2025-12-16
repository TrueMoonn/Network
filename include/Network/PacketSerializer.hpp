#pragma once

#include <vector>
#include <cstdint>
#include <cstring>

namespace net {

class PacketSerializer {
 public:
    /**
     * @brief Serializes a structure into a byte vector
     *
     * @tparam T Type of the structure to serialize
     * @param packet Constant reference to the structure to serialize
     * @return std::vector<uint8_t> Vector containing the serialized bytes
     */
    template<typename T>
    static std::vector<uint8_t> serialize(const T& packet) {
        std::vector<uint8_t> buffer(sizeof(T));
        std::memcpy(buffer.data(), &packet, sizeof(T));
        return buffer;
    }

    /**
     * @brief Deserializes a byte vector into a structure
     *
     * @tparam T Type of the structure to deserialize
     * @param buffer Byte vector to deserialize
     * @param packet Reference to the structure to fill
     * @return true if deserialization succeeded, false otherwise
     */
    template<typename T>
    static bool deserialize(const std::vector<uint8_t>& buffer,
        T& packet) {
        if (buffer.size() < sizeof(T)) {
            return false;
        }
        std::memcpy(&packet, buffer.data(), sizeof(T));
        return true;
    }
};

}  // namespace net
