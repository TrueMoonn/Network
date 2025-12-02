#include "Network/Client.hpp"
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

Client::Client(const std::string& protocol)
: _socket(), _server_address(), _connected(false), _protocol("config/protocol.json") {
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

    int sent = 0;

    if (_socket.getType() == SocketType::UDP) {
        sent = _socket.sendTo(fullPacket.data(), fullPacket.size(), _server_address);
    } else {
        sent = _socket.send(fullPacket.data(), fullPacket.size());
    }

    if (sent < 0) {
        std::cerr << "Failed to send data" << std::endl;
        return false;
    }

    if (static_cast<size_t>(sent) != fullPacket.size()) {
        std::cerr << "Partial send: "
                  << sent
                  << "/"
                  << fullPacket.size()
                  << " bytes"
                  << std::endl;
        return false;
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
        received = _socket.receiveFrom(tempBuffer.data(), tempBufferSize, sender);
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
        ProtocolManager::UnformattedPacket unformatted = _protocol.unformatPacket(tempBuffer);
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
