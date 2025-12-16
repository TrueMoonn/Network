#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace net {

/**
 * @brief Read a config.json and allow the user to format and unformat packet accordingly to his configuration.
 * 
 */
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

    /**
     * @brief Construct a new Protocol Manager object
     * 
     * @param path Path to file containing the protocol configuration.
     * Exemple:
     * {
     *   "endianness": "little",
     *   "preambule": {
     *       "active": true,
     *       "characters": "\r\t\r\t"
     *   },
     *   "packet_length": {
     *       "active": true,
     *       "length": 4
     *   },
     *   "datetime": {
     *       "active": true,
     *       "length": 8
     *   },
     *   "end_of_packet": {
     *       "active": true,
     *       "characters": "\r\n"
     *   }
     * }
     */
    explicit ProtocolManager(const std::string &path = "config/protocol.json");

    /**
     * @brief Destroy the ProtocolManager object
     * 
     */
    ~ProtocolManager() {}

    /**
     * @brief Format a packet with protocol elements (preambule, length, datetime, end)
     * 
     * @param data Data that you want to format
     * @return std::vector<uint8_t> Formatted packet
     */
    std::vector<uint8_t> formatPacket(std::vector<uint8_t> data);

    /**
     * @brief Extract the raw data and informations from a formatted packet
     * 
     * @param formattedData Packet formatted
     * @return UnformattedPacket Struct containing all informations from the packet
     */
    UnformattedPacket unformatPacket(const std::vector<uint8_t> &formattedData);

    /**
     * @brief Calculate overhead size added by the protocol
     * 
     * @return size_t Number combining the size all elements added to the packet accordingly to the protocol
     */
    size_t getProtocolOverhead() const;

    /**
     * @brief Get the Preambule object
     * 
     * @return const preambule& 
     */
    const preambule& getPreambule() const;

    /**
     * @brief Get the _packet_length object
     * 
     * @return const packet_length& 
     */
    const packet_length& getPacketLength() const;

    /**
     * @brief Get the _datetime object
     * 
     * @return const datetime& 
     */
    const datetime& getDatetime() const;

    /**
     * @brief Get the _end_of_packet object
     * 
     * @return const end_of_packet& 
     */
    const end_of_packet& getEndOfPacket() const;

    /**
     * @brief Get the _endianness object
     * 
     * @return Endianness 
     */
    Endianness getEndianness() const;


 private:
    preambule _preambule;
    packet_length _packet_length;
    datetime _datetime;
    end_of_packet _end_of_packet;
    Endianness _endianness;

    uint64_t getCurrentTimestamp() const;

    // endianness conversion
    void writeUint32(std::vector<uint8_t>& buffer, uint32_t value) const;
    void writeUint64(std::vector<uint8_t>& buffer,
        uint64_t value, int numBytes) const;
    uint32_t readUint32(const std::vector<uint8_t>& buffer,
        size_t offset, int numBytes) const;
    uint64_t readUint64(const std::vector<uint8_t>& buffer,
        size_t offset, int numBytes) const;
};

}  // namespace net
