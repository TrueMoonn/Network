#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "Packet.hpp"

class ProtocolManager {
 public:
    enum class Endianness {
        BIG,
        LITTLE
    };

    struct preambule {
        bool active;
        std::string characters;
    };

    struct packet_length {
        bool active;
        int length;
    };

    struct datetime {
        bool active;
        int length;
    };

    struct end_of_packet {
        bool active;
        std::string characters;
    };

    struct UnformattedPacket {
        std::vector<uint8_t> data;
        uint32_t packetLength;
        uint64_t timestamp;
        bool hasLength;
        bool hasTimestamp;
    };

    explicit ProtocolManager(const std::string &path = "config/protocol.json");
    ~ProtocolManager() {}

    // Format a packet with protocol elements (preambule, length, datetime, end)
    std::vector<uint8_t> formatPacket(const void *data, size_t dataSize);

    // extract the raw data and informations from a formatted packet
    UnformattedPacket unformatPacket(const std::vector<uint8_t> &formattedData);

    const preambule& getPreambule() const;
    const packet_length& getPacketLength() const;
    const datetime& getDatetime() const;
    const end_of_packet& getEndOfPacket() const;
    Endianness getEndianness() const;

    // Calculate overhead size added by the protocol
    size_t getProtocolOverhead() const;

 private:
    preambule _preambule;
    packet_length _packet_length;
    datetime _datetime;
    end_of_packet _end_of_packet;
    Endianness _endianness;

    uint64_t getCurrentTimestamp() const;

    // endianness conversion
    void writeUint32(std::vector<uint8_t>& buffer, uint32_t value) const;
    void writeUint64(std::vector<uint8_t>& buffer, uint64_t value, int numBytes) const;
    uint32_t readUint32(const std::vector<uint8_t>& buffer, size_t offset, int numBytes) const;
    uint64_t readUint64(const std::vector<uint8_t>& buffer, size_t offset, int numBytes) const;
};
