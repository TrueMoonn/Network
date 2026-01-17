#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <functional>
#include <unordered_map>

#include "Network/Client.hpp"
#include "Network/NetworkPlatform.hpp"

namespace net {

Client::Client(const std::string& protocol, const std::string& path)
    : _socket(),
    _server_address(),
    _connected(false),
    _protocol(path),
    _logger(true, "./logs", "client") {
    SocketType type;

    if (protocol == "TCP" || protocol == "tcp") {
        type = SocketType::TCP;
    } else if (protocol == "UDP" || protocol == "udp") {
        type = SocketType::UDP;
    } else {
        std::cerr << "Invalid protocol: "
                  << protocol
                  << ", Defaulting to UDP"
                  << std::endl;
        type = SocketType::UDP;
        _logger.write("Invalid protocol given (" +
            protocol + "): defaulting to UDP");
    }

    if (!_socket.create(type)) {
        std::cerr << "Failed to create socket in constructor" << std::endl;
        _logger.write("Failed to create socket in Client() constructor");
        return;
    }
    _logger.write("==============================");
    _logger.write("Client initialized ready to connect");
}

Client::~Client() {
    disconnect();
    _logger.write("==============================");
}

bool Client::connect(const std::string& server_ip,
    uint16_t server_port) {
    if (_connected) {
        std::cerr << "Client is already connected" << std::endl;
        _logger.write("ERROR\tClient is already connected");
        return false;
    }

    _server_address = Address(server_ip, server_port);

    if (getProtocol() == SocketType::TCP) {
        if (!_socket.connect(_server_address)) {
            std::cerr << "Failed to connect to TCP server" << std::endl;
            _logger.write("ERROR\tFailed to connect to server using TCP");
            return false;
        }
    }

    _connected = true;

    std::cout << "Client connected to "
              << server_ip
              << ":"
              << server_port
              << std::endl;
    _logger.write("Client connected to " + server_ip + ":" +
        std::to_string(server_port));

    return true;
}

void Client::disconnect() {
    if (!_connected) {
        _logger.write("WARNING\tClient is already disconnected");
        return;
    }
    if (_socket.isValid())
        _socket.close();
    _connected = false;
    std::cout << "Client disconnected" << std::endl;
    _logger.write("Client disconnected");
}

bool Client::initPacketTrackers(
    std::unordered_map<uint8_t, uint32_t> packetToTrace,
    std::function<void(uint8_t)> callback) {
    for (auto& packet : packetToTrace) {
        PacketTracking newEntry = {
            .expectedTime = packet.second,
            .lastRecvTime = std::chrono::steady_clock::now(),
        };
        _packetTrackers.insert({packet.first, newEntry});
    }
    _trackPacketCallback = callback;
    std::cout << "Packet trackers set for "
              << _packetTrackers.size()
              << " packets"
              << std::endl;
    return true;
}

bool Client::checkPacketTrackers() {
    auto now = std::chrono::steady_clock::now();
    for (auto& code : _packetTrackers) {
        auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - code.second.lastRecvTime)
                .count();
        if (elapsed > code.second.expectedTime * 2) {
            _trackPacketCallback(code.first);
            code.second.lastRecvTime = now;
        }
    }
    return true;
}

bool Client::markPacketCode(uint8_t code) {
    auto it = _packetTrackers.find(code);
    if (it == _packetTrackers.end()) {
        return false;
    }

    it->second.lastRecvTime = std::chrono::steady_clock::now();
    return true;
}

std::string dataToString(std::vector<uint8_t> buff) {
    std::string res;

    for (auto& byte : buff)
        res += std::to_string(byte) + " ";
    return res;
}

bool Client::send(const std::vector<uint8_t>& data) {
    if (!_connected) {
        std::cerr << "Client is not connected" << std::endl;
        _logger.write("ERROR\tTried to send data before connecting the client");
        return false;
    }
    if (!_socket.isValid()) {
        std::cerr << "Socket is invalid" << std::endl;
        _logger.write("ERROR\tTried to send data before setting the socket");
        return false;
    }

    if (data.empty()) {
        std::cerr << "Invalid data or size" << std::endl;
        _logger.write("ERROR\tTried to send unvalid data");
        return false;
    }

    std::vector<uint8_t> fullPacket = _protocol.formatPacket(data);

    _logger.write(
        "SEND\t" +
        _server_address.getIP() +
        ":" +
        std::to_string(_server_address.getPort()) +
        "\t" +
        dataToString(fullPacket));

    if (_socket.getType() == SocketType::UDP) {
        int sent = _socket.sendTo(fullPacket.data(), fullPacket.size(),
            _server_address);
        if (sent < 0) {
            std::cerr << "Failed to send data" << std::endl;
            _logger.write("ERROR\tFailed to send data");
            return false;
        }
        if (static_cast<size_t>(sent) != fullPacket.size()) {
            std::cerr <<
                "Partial UDP send: "
                << sent
                << "/"
                << fullPacket.size()
                << " bytes"
                << std::endl;
            _logger.write("WARNING\tPartial send of data");
            return false;
        }
    } else {
        size_t totalSent = 0;
        while (totalSent < fullPacket.size()) {
            int sent = _socket.send(fullPacket.data() + totalSent,
                fullPacket.size() - totalSent);
            if (sent < 0) {
                std::cerr << "Failed to send data" << std::endl;
                _logger.write("ERROR\tFailed to send data");
                return false;
            }
            if (sent == 0) {
                std::cerr << "Connection closed by peer during send"
                    << std::endl;
                _logger.write("ERROR\tTCP connection closed during send");
                return false;
            }
            totalSent += sent;
        }
    }

    return true;
}

int Client::receive(void* buffer, size_t max_size) {
    if (!_connected) {
        std::cerr << "Client is not connected" << std::endl;
        _logger.write(
            "ERROR\tTried to receive data before connecting the client");
        return -1;
    }
    if (!_socket.isValid()) {
        std::cerr << "Socket is invalid" << std::endl;
        _logger.write(
            "ERROR\tTried to receive data before setting the socket");
        return -1;
    }

    if (buffer == nullptr || max_size == 0) {
        std::cerr << "Invalid buffer or size" << std::endl;
        _logger.write("ERROR\tInvalid buffer or size to receive");
        return -1;
    }

    size_t tempBufferSize = max_size + _protocol.getProtocolOverhead();
    std::vector<uint8_t> tempBuffer(tempBufferSize);
    int received = 0;

    if (_socket.getType() == SocketType::UDP) {
        Address sender;
        received =
            _socket.receiveFrom(tempBuffer.data(), tempBufferSize, sender);
        if (received > 0 && !(sender == _server_address)) {
            std::cerr << "Error: Received packet from unexpected source: "
                      << sender.getIP()
                      << ":"
                      << sender.getPort()
                      << std::endl;
            _logger.write("ERROR\tPacket received from unexpected source : " +
                sender.getIP() + ":" + std::to_string(sender.getPort()));
            return -2;
        }
    } else {
        received = _socket.recv(tempBuffer.data(), tempBufferSize);
        if (received == 0) {
            std::cerr << "Server closed connection"
                << std::endl;
            _logger.write("ERROR\tServer closed connection");
            _connected = false;
            return 0;
        }
    }

    if (received < 0) {
        _logger.write("ERROR\tFailed to receive data (receive < 0)");
        std::cerr << "Failed to receive data" << std::endl;
        return -1;
    }

    try {
        tempBuffer.resize(received);

        _logger.write(
            "RECV\t" +
            _server_address.getIP() +
            ":" +
            std::to_string(_server_address.getPort()) +
            "\t" +
            dataToString(tempBuffer));

        ProtocolManager::UnformattedPacket unformatted =
            _protocol.unformatPacket(tempBuffer);
        size_t dataToCopy = std::min(unformatted.data.size(), max_size);

        markPacketCode(unformatted.data[0]);

        std::memcpy(buffer, unformatted.data.data(), dataToCopy);
        if (unformatted.data.size() > max_size) {
            std::cerr << "Warning: Received data truncated ("
                      << unformatted.data.size()
                      << " bytes received, "
                      << max_size
                      << " bytes buffer)"
                      << std::endl;
        }
        return static_cast<int>(dataToCopy);
    } catch (const std::exception& e) {
        std::cerr << "Failed to unformat packet: " << e.what() << std::endl;
        _logger.write("ERROR\tFailed to unformat packet : " +
            std::string(e.what()));
        return -1;
    }
}

bool Client::setNonBlocking(bool enabled) {
    if (!_socket.isValid()) {
        _logger.write("ERROR\tCannot set socket.nonblocking of invalid socket");
        return false;
    }
    return _socket.setNonBlocking(enabled);
}

bool Client::setTimeout(int milliseconds) {
    if (!_socket.isValid()) {
        _logger.write("ERROR\tCannot set socket.timeout of invalid socket");
        return false;
    }
    return _socket.setTimeout(milliseconds);
}

void Client::udpReceive(int timeout, int maxInputs) {
    if (!_connected) {
        std::cerr << "Client is not connected" << std::endl;
        _logger.write(
            "ERROR\tTried to receive data before connecting the client");
        return;
    }
    if (!_socket.isValid()) {
        std::cerr << "Socket is invalid" << std::endl;
        _logger.write(
            "ERROR\tTried to receive data before setting the socket");
        return;
    }

    POLLFD pfd;
    pfd.fd = _socket.getSocket();
    pfd.events = POLL_IN;
    pfd.revents = 0;

    int poll_result = PollSockets(&pfd, 1, timeout);
    if (poll_result < 0) {
        std::cerr << "Poll error in udpReceive()" << std::endl;
        _logger.write("ERROR\tPoll error in receive");
        return;
    }
    if (poll_result == 0) {
        return;
    }

    size_t bufferSize = BUFSIZ + _protocol.getProtocolOverhead();

    for (int count = 0; count < maxInputs; count++) {
        Address sender;
        std::vector<uint8_t> tempBuffer(bufferSize);

        int received =
            _socket.receiveFrom(tempBuffer.data(), bufferSize, sender);

        if (received <= 0)
            break;

        if (!(sender == _server_address)) {
            std::cerr <<
                "Warning: Received UDP packet from unexpected source: "
                << sender.getIP() << ":" << sender.getPort() << std::endl;
            _logger.write("ERROR\tPacket received from unexpected source : " +
                sender.getIP() + ":" + std::to_string(sender.getPort()));
            continue;
        }

        std::vector<uint8_t> packetData(tempBuffer.begin(),
                                        tempBuffer.begin() + received);

        _logger.write(
            "RECV\t" +
            _server_address.getIP() +
            ":" +
            std::to_string(_server_address.getPort()) +
            "\t" +
            dataToString(packetData));

        _input_buffer.insert(_input_buffer.end(),
                            packetData.begin(), packetData.end());
    }
}

void Client::tcpReceive(int timeout) {
    if (!_connected) {
        std::cerr << "Client is not connected" << std::endl;
        _logger.write(
            "ERROR\tTried to receive data before connecting the client");
        return;
    }
    if (!_socket.isValid()) {
        std::cerr << "Socket is invalid" << std::endl;
        _logger.write(
            "ERROR\tTried to receive data before setting the socket");
        return;
    }

    POLLFD pfd;
    pfd.fd = _socket.getSocket();
    pfd.events = POLL_IN;
    pfd.revents = 0;

    int poll_result = PollSockets(&pfd, 1, timeout);
    if (poll_result < 0) {
        std::cerr << "Poll error in tcpReceive()" << std::endl;
        _logger.write("ERROR\tPoll error in receive");
        return;
    }
    if (poll_result == 0) {
        return;
    }

    if (!(pfd.revents & POLL_IN)) {
        return;
    }

    size_t bufferSize = BUFSIZ + _protocol.getProtocolOverhead();
    std::vector<uint8_t> tempBuffer(bufferSize);

    while (true) {
        int received = _socket.recv(tempBuffer.data(), bufferSize);

        if (received == 0) {
            std::cerr << "Server closed connection" << std::endl;
            _connected = false;
            _logger.write("ERROR\tServer force closed connection");
            break;
        }

        if (received < 0) {
            break;
        }

        tempBuffer.resize(received);

        _logger.write(
            "RECV\t" +
            _server_address.getIP() +
            ":" +
            std::to_string(_server_address.getPort()) +
            "\t" +
            dataToString(tempBuffer));

        _input_buffer.insert(_input_buffer.end(),
                            tempBuffer.begin(),
                            tempBuffer.begin() + received);
    }
}

std::vector<std::vector<uint8_t>> Client::extractPacketsFromBuffer() {
    std::vector<std::vector<uint8_t>> result;

    ProtocolManager::preambule preamble = _protocol.getPreambule();
    ProtocolManager::datetime datetime = _protocol.getDatetime();
    ProtocolManager::packet_length packetLength = _protocol.getPacketLength();
    ProtocolManager::end_of_packet packetEnd = _protocol.getEndOfPacket();
    ProtocolManager::Endianness endianness = _protocol.getEndianness();

    while (!_input_buffer.empty()) {
        size_t offset = 0;
        if (preamble.active) {
            if (_input_buffer.size() < preamble.characters.size())
                break;
            offset += preamble.characters.size();
        }

        uint32_t dataLength = 0;

        if (packetLength.active) {
            if (_input_buffer.size() < offset + packetLength.length)
                break;

            if (endianness == ProtocolManager::Endianness::LITTLE) {
                if (packetLength.length == 4) {
                    dataLength = CAST_UINT32(_input_buffer[offset]) |
                               (CAST_UINT32(_input_buffer[offset + 1]) << 8) |
                               (CAST_UINT32(_input_buffer[offset + 2]) << 16) |
                               (CAST_UINT32(_input_buffer[offset + 3]) << 24);
                } else if (packetLength.length == 2) {
                    dataLength = CAST_UINT32(_input_buffer[offset]) |
                               (CAST_UINT32(_input_buffer[offset + 1]) << 8);
                } else if (packetLength.length == 1) {
                    dataLength = _input_buffer[offset];
                }
            } else {
                if (packetLength.length == 4) {
                    dataLength =
                        (CAST_UINT32(_input_buffer[offset]) << 24) |
                        (CAST_UINT32(_input_buffer[offset + 1]) << 16) |
                        (CAST_UINT32(_input_buffer[offset + 2]) << 8) |
                        CAST_UINT32(_input_buffer[offset + 3]);
                } else if (packetLength.length == 2) {
                    dataLength =
                        (CAST_UINT32(_input_buffer[offset]) << 8) |
                            CAST_UINT32(_input_buffer[offset + 1]);
                } else if (packetLength.length == 1) {
                    dataLength = _input_buffer[offset];
                }
            }

            offset += packetLength.length;
            uint32_t actualDataLength = dataLength;
            if (datetime.active) {
                if (dataLength < CAST_UINT32(datetime.length)) {
                    break;
                }
                actualDataLength = dataLength - datetime.length;
            }

            if (datetime.active) {
                if (_input_buffer.size() < offset + datetime.length)
                    break;
                offset += datetime.length;
            }

            if (_input_buffer.size() < offset + actualDataLength)
                break;

            std::vector<uint8_t> packetData(
                _input_buffer.begin() + offset,
                _input_buffer.begin() + offset + actualDataLength);

            markPacketCode(packetData[0]);
            result.push_back(packetData);
            offset += actualDataLength;

            if (packetEnd.active) {
                if (_input_buffer.size() < offset + packetEnd.characters.size())
                    break;
                offset += packetEnd.characters.size();
            }
            _input_buffer.erase(_input_buffer.begin(),
                               _input_buffer.begin() + offset);

        } else if (packetEnd.active) {
            if (datetime.active) {
                if (_input_buffer.size() < offset + datetime.length)
                    break;
                offset += datetime.length;
            }
            std::string endMarker = packetEnd.characters;
            auto it = std::search(
                _input_buffer.begin() + offset,
                _input_buffer.end(),
                endMarker.begin(),
                endMarker.end());

            if (it == _input_buffer.end())
                break;

            size_t dataEnd = std::distance(_input_buffer.begin(), it);
            size_t dataStart = offset;
            dataLength = dataEnd - dataStart;

            std::vector<uint8_t> packetData(
                _input_buffer.begin() + dataStart,
                _input_buffer.begin() + dataEnd);

            result.push_back(packetData);

            _input_buffer.erase(_input_buffer.begin(),
                               _input_buffer.begin()
                               + dataEnd + endMarker.size());
        } else {
            std::cerr << "Protocol error: no packet_length or end_of_packet"
                << std::endl;
            break;
        }
    }

    return result;
}

}  // namespace net
