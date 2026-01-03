#pragma once
#include <stdexcept>
#include "ProtocolManager.hpp"

namespace net {

class MessageProtocol {
  public:
    explicit MessageProtocol(ProtocolManager& protocolManager)
        : _protocolManager(protocolManager) {}

    template<typename T>
    std::vector<uint8_t> pack(const T& message) {
        auto msgData = message.serialize();
        return _protocolManager.formatPacket(msgData);
    }

    uint32_t getMessageId(const std::vector<uint8_t>& packet) {
        auto unformatted = _protocolManager.unformatPacket(packet);
        if (unformatted.data.size() < 4) {
            throw std::runtime_error("Invalid packet: too small");
        }
        return readMessageId(unformatted.data);
    }

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
