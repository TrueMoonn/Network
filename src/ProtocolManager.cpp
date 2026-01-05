#include <json/json.h>
#include <fstream>
#include <sstream>
#include <memory>
#include <iostream>
#include <cstring>
#include <chrono>
#include <stdexcept>
#include <vector>
#include <string>

#include "Network/ProtocolManager.hpp"

namespace net {

ProtocolManager::ProtocolManager(const std::string &path) {
    std::ifstream file(path, std::ifstream::binary);
    Json::Value protocol;

    if (!file) {
        std::cerr << "Error: Cannot open protocol config file: "
            << path << std::endl;
        throw std::runtime_error("Invalid protocol config path");
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    Json::CharReaderBuilder builder;
    std::string errs;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

    if (!reader->parse(content.c_str(), content.c_str() + content.size(),
        &protocol, &errs)) {
        std::cerr << "Error: Invalid JSON format: " << errs << std::endl;
        throw std::runtime_error("Invalid JSON format in protocol config file");
    }

    if (protocol.isMember("preambule")) {
        _preambule.active = protocol["preambule"]["active"].asBool();
        _preambule.characters = protocol["preambule"]["characters"].asString();
    } else {
        _preambule.active = false;
    }

    if (protocol.isMember("packet_length")) {
        _packet_length.active = protocol["packet_length"]["active"].asBool();
        _packet_length.length = protocol["packet_length"]["length"].asInt();
    } else {
        _packet_length.active = false;
    }

    if (protocol.isMember("datetime")) {
        _datetime.active = protocol["datetime"]["active"].asBool();
        _datetime.length = protocol["datetime"]["length"].asInt();
    } else {
        _datetime.active = false;
    }

    if (protocol.isMember("end_of_packet")) {
        _end_of_packet.active = protocol["end_of_packet"]["active"].asBool();
        _end_of_packet.characters =
            protocol["end_of_packet"]["characters"].asString();
    } else {
        _end_of_packet.active = false;
    }

    if (protocol.isMember("endianness")) {
        std::string endian = protocol["endianness"].asString();
        if (endian == "little") {
            _endianness = Endianness::LITTLE;
        } else if (endian == "big") {
            _endianness = Endianness::BIG;
        } else {
            std::cerr << "Warning: Unknown endianness '" << endian
                      << "', defaulting to big-endian" << std::endl;
            _endianness = Endianness::BIG;
        }
    } else {
        _endianness = Endianness::BIG;  // Default
    }

    std::cout << "Protocol configuration loaded successfully" << std::endl;
    std::cout << "  Endianness: "
        << (_endianness == Endianness::BIG ? "big" : "little")
        << "-endian" << std::endl;
    std::cout << "  Preambule: "
        << (_preambule.active ? "enabled" : "disabled") << std::endl;
    std::cout << "  Packet length: "
        << (_packet_length.active ? "enabled" : "disabled") << std::endl;
    std::cout << "  Datetime: " << (_datetime.active ? "enabled" : "disabled")
        << std::endl;
    std::cout << "  End of packet: "
        << (_end_of_packet.active ? "enabled" : "disabled") << std::endl;
}

uint64_t ProtocolManager::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return
        std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

std::vector<uint8_t> ProtocolManager::formatPacket(std::vector<uint8_t> data) {
    std::vector<uint8_t> formattedPacket;

    if (_preambule.active) {
        const uint8_t* preambleBytes = reinterpret_cast<const uint8_t*>(
            _preambule.characters.c_str());
        formattedPacket.insert(formattedPacket.end(), preambleBytes,
                              preambleBytes + _preambule.characters.size());
    }
    if (_packet_length.active) {
        uint32_t totalLength = static_cast<uint32_t>(data.size());
        if (_datetime.active) {
            totalLength += _datetime.length;
        }
        writeUint32(formattedPacket, totalLength);
    }
    if (_datetime.active) {
        uint64_t timestamp = getCurrentTimestamp();
        writeUint64(formattedPacket, timestamp, _datetime.length);
    }

    const uint8_t* dataBytes = static_cast<const uint8_t*>(data.data());
    formattedPacket.insert(formattedPacket.end(), dataBytes, dataBytes
        + data.size());

    if (_end_of_packet.active) {
        const uint8_t* endBytes = reinterpret_cast<const uint8_t*>(
            _end_of_packet.characters.c_str());
        formattedPacket.insert(formattedPacket.end(), endBytes,
                              endBytes + _end_of_packet.characters.size());
    }

    return formattedPacket;
}

// faut le changer lui je crois :(
ProtocolManager::UnformattedPacket ProtocolManager::unformatPacket(
    const std::vector<uint8_t> &formattedData) {
    UnformattedPacket result;
    result.packetLength = 0;
    result.timestamp = 0;
    result.hasLength = false;
    result.hasTimestamp = false;
    size_t offset = 0;

    if (_preambule.active) {
        if (formattedData.size() < _preambule.characters.size()) {
            throw std::runtime_error("Packet too small to contain preambule");
        }

        std::string receivedPreambule(formattedData.begin(),
                                     formattedData.begin()
                                     + _preambule.characters.size());
        if (receivedPreambule != _preambule.characters) {
            throw std::runtime_error("Invalid preambule in packet");
        }
        offset += _preambule.characters.size();
    }

    if (_packet_length.active) {
        if (formattedData.size() < offset +
            static_cast<size_t>(_packet_length.length)) {
            throw std::runtime_error(
                "Packet too small to contain length field");
        }
        result.hasLength = true;
        result.packetLength =
            readUint32(formattedData, offset, _packet_length.length);
        offset += static_cast<size_t>(_packet_length.length);
    }

    if (_datetime.active) {
        if (formattedData.size() <
            offset + static_cast<size_t>(_datetime.length)) {
            throw std::runtime_error(
                "Packet too small to contain datetime field");
        }
        result.hasTimestamp = true;
        result.timestamp = readUint64(formattedData, offset, _datetime.length);
        offset += static_cast<size_t>(_datetime.length);
    }

    size_t dataSize;
    if (_packet_length.active && result.hasLength) {
        dataSize = result.packetLength;
        if (_datetime.active) {
            dataSize -= _datetime.length;
        }
    } else {
        dataSize = formattedData.size() - offset;
        if (_end_of_packet.active) {
            dataSize -= _end_of_packet.characters.size();
        }
    }

    if (_end_of_packet.active) {
        if (formattedData.size() < offset + dataSize
            + _end_of_packet.characters.size()) {
            throw std::runtime_error("Packet too small to contain end marker");
        }

        size_t endMarkerPos = offset + dataSize;
        std::string receivedEnd(formattedData.begin() + endMarkerPos,
                               formattedData.begin() + endMarkerPos
                               + _end_of_packet.characters.size());
        if (receivedEnd != _end_of_packet.characters) {
            std::cerr << "Expected end marker: ";
            for (size_t i = 0; i < _end_of_packet.characters.size(); i++) {
                std::cerr << static_cast<int>(static_cast<uint8_t>(
                    _end_of_packet.characters[i])) << " ";
            }
            std::cerr << ", but got: ";
            for (size_t i = 0; i < receivedEnd.size(); i++) {
                std::cerr << static_cast<int>(static_cast<uint8_t>(
                    receivedEnd[i])) << " ";
            }
            std::cerr << std::endl;
            throw std::runtime_error("Invalid end marker in packet");
        }
    }

    result.data = std::vector<uint8_t>(formattedData.begin() + offset,
                                    formattedData.begin() + offset + dataSize);
    return result;
}

const ProtocolManager::preambule& ProtocolManager::getPreambule() const {
    return _preambule;
}

const ProtocolManager::packet_length& ProtocolManager::getPacketLength() const {
    return _packet_length;
}

const ProtocolManager::datetime& ProtocolManager::getDatetime() const {
    return _datetime;
}

const ProtocolManager::end_of_packet& ProtocolManager::getEndOfPacket() const {
    return _end_of_packet;
}

ProtocolManager::Endianness ProtocolManager::getEndianness() const {
    return _endianness;
}

size_t ProtocolManager::getProtocolOverhead() const {
    size_t overhead = 0;

    if (_preambule.active) {
        overhead += _preambule.characters.size();
    }
    if (_packet_length.active) {
        overhead += _packet_length.length;
    }
    if (_datetime.active) {
        overhead += _datetime.length;
    }
    if (_end_of_packet.active) {
        overhead += _end_of_packet.characters.size();
    }
    return overhead;
}

void ProtocolManager::writeUint32(std::vector<uint8_t>& buffer,
    uint32_t value) const {
    if (_endianness == Endianness::BIG) {
        for (int i = _packet_length.length - 1; i >= 0; --i) {
            buffer.push_back((value >> (i * 8)) & 0xFF);
        }
    } else {
        for (int i = 0; i < _packet_length.length; ++i) {
            buffer.push_back((value >> (i * 8)) & 0xFF);
        }
    }
}

void ProtocolManager::writeUint64(std::vector<uint8_t>& buffer,
                                  uint64_t value, int numBytes) const {
    if (_endianness == Endianness::BIG) {
        for (int i = numBytes - 1; i >= 0; --i) {
            buffer.push_back((value >> (i * 8)) & 0xFF);
        }
    } else {
        for (int i = 0; i < numBytes; ++i) {
            buffer.push_back((value >> (i * 8)) & 0xFF);
        }
    }
}

uint32_t ProtocolManager::readUint32(const std::vector<uint8_t>& buffer,
                                     size_t offset, int numBytes) const {
    if (offset + static_cast<size_t>(numBytes) > buffer.size()) {
        throw std::runtime_error("readUint32: buffer too small");
    }
    uint32_t value = 0;
    if (_endianness == Endianness::BIG) {
        for (int i = 0; i < numBytes; ++i) {
            if (offset + static_cast<size_t>(i) >= buffer.size()) {
                throw std::runtime_error("readUint32: index out of bounds");
            }
            value = (value << 8) | buffer[offset + i];
        }
    } else {
        for (int i = numBytes - 1; i >= 0; --i) {
            if (offset + static_cast<size_t>(i) >= buffer.size()) {
                throw std::runtime_error("readUint32: index out of bounds");
            }
            value = (value << 8) | buffer[offset + i];
        }
    }
    return value;
}

uint64_t ProtocolManager::readUint64(const std::vector<uint8_t>& buffer,
                                     size_t offset, int numBytes) const {
    if (offset + static_cast<size_t>(numBytes) > buffer.size()) {
        throw std::runtime_error("readUint64: buffer too small");
    }
    uint64_t value = 0;
    if (_endianness == Endianness::BIG) {
        for (int i = 0; i < numBytes; ++i) {
            if (offset + static_cast<size_t>(i) >= buffer.size()) {
                throw std::runtime_error("readUint64: index out of bounds");
            }
            value = (value << 8) | buffer[offset + i];
        }
    } else {
        for (int i = numBytes - 1; i >= 0; --i) {
            if (offset + static_cast<size_t>(i) >= buffer.size()) {
                throw std::runtime_error("readUint64: index out of bounds");
            }
            value = (value << 8) | buffer[offset + i];
        }
    }
    return value;
}

}  // namespace net
