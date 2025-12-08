#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include "Network/Client.hpp"
#include "Network/NetworkPlatform.hpp"

Client::Client(const std::string& protocol)
    : _socket(),
    _server_address(),
    _connected(false),
    _protocol("config/protocol.json") {
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
    }

    if (!_socket.create(type)) {
        std::cerr << "Failed to create socket in constructor" << std::endl;
    }
}

Client::~Client() {
    disconnect();
}

bool Client::connect(const std::string& server_ip,
    uint16_t server_port) {
    if (_connected) {
        std::cerr << "Client is already connected" << std::endl;
        return false;
    }

    _server_address = Address(server_ip, server_port);

    if (_socket.getType() == SocketType::TCP) {
        if (!_socket.connect(_server_address)) {
            std::cerr << "Failed to connect to TCP server" << std::endl;
            return false;
        }
    }

    _connected = true;

    std::cout << "Client connected to "
              << server_ip
              << ":"
              << server_port
              << std::endl;

    return true;
}

void Client::disconnect() {
    if (_socket.isValid()) {
        _socket.close();
    }
    _connected = false;
    std::cout << "Client disconnected" << std::endl;
}

bool Client::send(const std::vector<uint8_t>& data) {
    if (!_connected || !_socket.isValid()) {
        std::cerr << "Client is not connected or socket is invalid"
                  << std::endl;
        return false;
    }

    if (data.empty()) {
        std::cerr << "Invalid data or size"
                  << std::endl;
        return false;
    }

    std::vector<uint8_t> fullPacket = _protocol.formatPacket(data);

    if (_socket.getType() == SocketType::UDP) {
        int sent = _socket.sendTo(fullPacket.data(), fullPacket.size(),
            _server_address);
        if (sent < 0) {
            std::cerr << "Failed to send data" << std::endl;
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
            return false;
        }
    } else {
        size_t totalSent = 0;
        while (totalSent < fullPacket.size()) {
            int sent = _socket.send(fullPacket.data() + totalSent,
                fullPacket.size() - totalSent);
            if (sent < 0) {
                std::cerr << "Failed to send data" << std::endl;
                return false;
            }
            if (sent == 0) {
                std::cerr << "Connection closed by peer during send"
                    << std::endl;
                return false;
            }
            totalSent += sent;
        }
    }

    return true;
}

int Client::receive(void* buffer, size_t max_size) {
    if (!_connected || !_socket.isValid()) {
        std::cerr << "Client is not connected or socket is invalid"
                  << std::endl;
        return -1;
    }

    if (buffer == nullptr || max_size == 0) {
        std::cerr << "Invalid buffer or size" << std::endl;
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
            return -2;
        }
    } else {
        received = _socket.recv(tempBuffer.data(), tempBufferSize);
        if (received == 0) {
            std::cerr << "Server closed connection"
                      << std::endl;
            _connected = false;
            return 0;
        }
    }

    if (received < 0) {
        std::cerr << "Failed to receive data" << std::endl;
        return -1;
    }

    try {
        tempBuffer.resize(received);
        ProtocolManager::UnformattedPacket unformatted =
            _protocol.unformatPacket(tempBuffer);
        size_t dataToCopy = std::min(unformatted.data.size(), max_size);
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
        return -1;
    }
}

bool Client::setNonBlocking(bool enabled) {
    if (!_socket.isValid())
        return false;
    return _socket.setNonBlocking(enabled);
}

bool Client::setTimeout(int milliseconds) {
    if (!_socket.isValid())
        return false;
    return _socket.setTimeout(milliseconds);
}

void Client::udpReceive(int timeout, int maxInputs) {
    if (!_connected || !_socket.isValid()) {
        std::cerr << "Client is not connected or socket is invalid"
            << std::endl;
        return;
    }

    POLLFD pfd;
    pfd.fd = _socket.getSocket();
    pfd.events = POLL_IN;
    pfd.revents = 0;

    int poll_result = PollSockets(&pfd, 1, timeout);
    if (poll_result < 0) {
        std::cerr << "Poll error in udpReceive()" << std::endl;
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
        if (received <= 0) {
            break;
        }

        if (!(sender == _server_address)) {
            std::cerr <<
                "Warning: Received UDP packet from unexpected source: "
                << sender.getIP() << ":" << sender.getPort() << std::endl;
            continue;
        }

        std::vector<uint8_t> packetData(tempBuffer.begin(),
                                        tempBuffer.begin() + received);
        _input_buffer.insert(_input_buffer.end(),
                            packetData.begin(), packetData.end());
    }
}

void Client::tcpReceive(int timeout) {
    if (!_connected || !_socket.isValid()) {
        std::cerr << "Client is not connected or socket is invalid"
            << std::endl;
        return;
    }

    POLLFD pfd;
    pfd.fd = _socket.getSocket();
    pfd.events = POLL_IN;
    pfd.revents = 0;

    int poll_result = PollSockets(&pfd, 1, timeout);
    if (poll_result < 0) {
        std::cerr << "Poll error in tcpReceive()" << std::endl;
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
            break;
        }

        if (received < 0) {
            break;
        }

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
