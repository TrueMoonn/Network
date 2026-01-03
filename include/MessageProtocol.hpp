#pragma once
#include <stdexcept>
#include "ProtocolManager.hpp"

namespace net {

class MessageProtocol {
  public:
    /**
     * @brief Constructs a MessageProtocol object.
     *
     * @param protocolManager Reference to a ProtocolManager instance used for packet formatting and unformatting.
     */
    explicit MessageProtocol(ProtocolManager& protocolManager)
        : _protocolManager(protocolManager) {}

    /**
     * @brief Serializes a message and formats it into a packet.
     *
     * @tparam T The type of the message to be packed. The type must implement a `serialize` method.
     * @param message The message object to be serialized and packed.
     * @return A vector of bytes representing the formatted packet.
     */
    template<typename T>
    std::vector<uint8_t> pack(const T& message) {
        auto msgData = message.serialize();
        return _protocolManager.formatPacket(msgData);
    }

    /**
     * @brief Extracts the message ID from a formatted packet.
     *
     * @param packet The vector of bytes representing the formatted packet.
     * @return The message ID as a 32-bit unsigned integer.
     * @throws std::runtime_error If the packet is too small to contain a valid message ID.
     */
    uint32_t getMessageId(const std::vector<uint8_t>& packet) {
        auto unformatted = _protocolManager.unformatPacket(packet);
        if (unformatted.data.size() < 4) {
            throw std::runtime_error("Invalid packet: too small");
        }
        return readMessageId(unformatted.data);
    }

    /**
     * @brief Unpacks a formatted packet and deserializes it into a message object.
     *
     * @tparam T The type of the message to be unpacked. The type must implement a `deserialize` method.
     * @param packet The vector of bytes representing the formatted packet.
     * @return The deserialized message object of type T.
     */
    template<typename T>
    T unpack(const std::vector<uint8_t>& packet) {
        auto unformatted = _protocolManager.unformatPacket(packet);
        return T::deserialize(unformatted.data);
    }

  private:
    ProtocolManager& _protocolManager;

    uint32_t readMessageId(const std::vector<uint8_t>& data) {
        if (_protocolManager.getEndianness() == ProtocolManager::Endianness::BIG) {
            return (static_cast<uint32_t>(data[0]) << 24) |
                   (static_cast<uint32_t>(data[1]) << 16) |
                   (static_cast<uint32_t>(data[2]) << 8) |
                   static_cast<uint32_t>(data[3]);
        } else {
            return (static_cast<uint32_t>(data[3]) << 24) |
                   (static_cast<uint32_t>(data[2]) << 16) |
                   (static_cast<uint32_t>(data[1]) << 8) |
                   static_cast<uint32_t>(data[0]);
        }
    }
};

}  // namespace net
