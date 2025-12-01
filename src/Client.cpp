#include "Network/Client.hpp"
#include <iostream>
#include <cstring>
#include <string>

Client::Client(const std::string& protocol)
: _socket(), _server_address(), _connected(false) {
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

bool Client::send(const void* data, size_t size) {
    if (!_connected || !_socket.isValid()) {
        std::cerr << "Client is not connected or socket is invalid"
                  << std::endl;
        return false;
    }

    if (data == nullptr || size == 0) {
        std::cerr << "Invalid data or size"
                  << std::endl;
        return false;
    }

    int sent = 0;

    if (_socket.getType() == SocketType::UDP) {
        sent = _socket.sendTo(data, size, _server_address);
    } else {
        sent = _socket.send(data, size);
    }

    if (sent < 0) {
        std::cerr << "Failed to send data" << std::endl;
        return false;
    }

    if (static_cast<size_t>(sent) != size) {
        std::cerr << "Partial send: "
                  << sent
                  << "/"
                  << size
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

    int received = 0;

    if (_socket.getType() == SocketType::UDP) {
        Address sender;
        received = _socket.receiveFrom(buffer, max_size, sender);

        if (received > 0 && !(sender == _server_address)) {
            std::cerr << "Error: Received packet from unexpected source: "
                      << sender.getIP()
                      << ":"
                      << sender.getPort()
                      << std::endl;
            return -2;
        }
    } else {
        received = _socket.recv(buffer, max_size);

        if (received == 0) {
            std::cerr << "Server closed connection"
                      << std::endl;
            _connected = false;
        }
    }

    if (received < 0) {
        std::cerr << "Failed to receive data" << std::endl;
        return -1;
    }

    return received;
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
