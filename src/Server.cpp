#include <iostream>
#include <cstring>
#include <unistd.h>

#include "Network/Server.hpp"

// remplacer les cerr par autre chose genre pour l'ECS ? faire remonter dans des try / catchs ?

Server::Server(const std::string& protocol, uint16_t port)
    : _port(port), _socket(), _running(false) {

    SocketType type;

    if (protocol == "TCP" || protocol == "tcp") {
        type = SocketType::TCP;
    } else if (protocol == "UDP" || protocol == "udp") {
        type = SocketType::UDP;
    } else {
        std::cerr << "Invalid protocol: " << protocol << ", Defaulting to UDP" << std::endl;
        type = SocketType::UDP;
    }

    if (!_socket.create(type)) {
        std::cerr << "Failed to create socket in constructor" << std::endl;
    }
}

Server::~Server() {
    stop();
}

bool Server::start() {
    if (_running) {
        std::cerr << "Server is already running" << std::endl;
        return false;
    }

    _socket.setReuseAddr(true);

    if (!_socket.bind(_port)) {
        std::cerr << "Failed to bind socket to port " << _port << std::endl;
        _socket.close();
        return false;
    }

    if (_socket.getType() == SocketType::TCP) {
        if (!_socket.listen(10)) {
            std::cerr << "Failed to listen on TCP socket" << std::endl;
            _socket.close();
            return false;
        }
    }

    _running = true;
    return true;
}

void Server::stop() {
    for (auto& pair : _tcp_clients) {
        ::close(pair.first);
    }
    _tcp_clients.clear();

    if (_socket.isValid()) {
        _socket.close();
    }
    _running = false;
}

bool Server::send(const Address& dest, const void* data, size_t size) {
    if (!_running || !_socket.isValid()) {
        std::cerr << "Server is not running or socket is invalid" << std::endl;
        return false;
    }

    if (data == nullptr || size == 0) {
        std::cerr << "Invalid data or size" << std::endl;
        return false;
    }

    int sent = 0;

    if (_socket.getType() == SocketType::UDP) {
        sent = _socket.sendTo(data, size, dest);
    } else {
        // checker cette partie pcq il est tard frr
        for (auto& pair : _tcp_clients) {
            if (pair.second == dest) {
                sent = ::send(pair.first, data, size, 0);
                break;
            }
        }
        if (sent == 0) {
            std::cerr << "Client not found" << std::endl;
            return false;
        }
    }

    if (sent < 0) {
        std::cerr << "Failed to send data" << std::endl;
        return false;
    }

    if (static_cast<size_t>(sent) != size) {
        std::cerr << "Partial send: " << sent << " / " << size << "bytes" << std::endl;
        return false;
    }

    return true;
}

// ouais j'ai mis un raw pointer on en parlera plus tard ce sera plus simple a remplacer au moins j'avance
int Server::receive(void* buffer, size_t max_size, Address& sender) {
    if (!_running || !_socket.isValid()) {
        std::cerr << "Server is not running or socket is invalid" << std::endl;
        return -1;
    }

    if (buffer == nullptr || max_size == 0) {
        std::cerr << "Invalid buffer or size" << std::endl;
        return -1;
    }

    int received = 0;

    if (_socket.getType() == SocketType::UDP) {
        received = _socket.receiveFrom(buffer, max_size, sender);
    } else {
        if (_tcp_clients.empty()) {
            std::cerr << "No TCP clients connected" << std::endl;
            return -1;
        }

        // ca faut revérifier j'ai des bugs mais que des fois ?
        auto it = _tcp_clients.begin();
        int client_fd = it->first;
        sender = it->second;

        received = ::recv(client_fd, buffer, max_size, 0);

        if (received == 0) {
            ::close(client_fd);
            _tcp_clients.erase(it);
        }
    }

    if (received < 0) {
        std::cerr << "Failed to receive data" << std::endl;
        return -1;
    }

    return received;
}

int Server::acceptClient(Address& client_addr) {
    if (_socket.getType() != SocketType::TCP) {
        std::cerr << "acceptClient() is only valid for TCP mode" << std::endl;
        return -1;
    }

    if (!_running || !_socket.isValid()) {
        std::cerr << "Server is not running" << std::endl;
        return -1;
    }

    int client_fd = _socket.accept(client_addr);

    if (client_fd < 0) {
        std::cerr << "Failed to accept client connection" << std::endl;
        return -1;
    }

    _tcp_clients[client_fd] = client_addr;

    return client_fd;
}

// J'ai pompé tes fonctions huhu
bool Server::setNonBlocking(bool enabled) {
    if (!_socket.isValid())
        return false;
    return _socket.setNonBlocking(enabled);
}

bool Server::setTimeout(int milliseconds) {
    if (!_socket.isValid())
        return false;
    return _socket.setTimeout(milliseconds);
}
