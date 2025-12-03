#include <unistd.h>
#include <iostream>
#include <cstring>
#include <string>
#include <utility>
#include <algorithm>
#include <vector>

#include "Network/Server.hpp"
#include "Network/PacketSerializer.hpp"
#include "Network/ProtocolManager.hpp"

Server::Server(
    const std::string& protocol,
    uint16_t port,
    ProtocolManager protocolManager)
    : _port(port), _running(false), _protocol(protocolManager) {

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
    _socket = NetworkSocket(type);

    if (!_socket.create(type)) {
        std::cerr << "Failed to create socket in constructor"
                  << std::endl;
    }

    if (type == SocketType::TCP) {
        pollfd server_pfd;
        server_pfd.fd = _socket.getSocket();
        server_pfd.events = POLLIN;
        server_pfd.revents = 0;
        _tcp_fds.push_back(server_pfd);
    }
}
Server::~Server() {
    stop();
}

bool Server::start() {
    if (_running) {
        std::cerr << "Server is already running"
                  << std::endl;
        return false;
    }

    _socket.setReuseAddr(true);

    if (!_socket.bind(_port)) {
        std::cerr << "Failed to bind socket to port "
                  << _port
                  << std::endl;
        _socket.close();
        return false;
    }

    if (_socket.getType() == SocketType::TCP) {
        if (!_socket.listen(10)) {
            std::cerr << "Failed to listen on TCP socket"
                      << std::endl;
            _socket.close();
            return false;
        }
    }

    _running = true;
    return true;
}

void Server::stop() {
    for (size_t i = 1; i < _tcp_fds.size(); ++i) {
        ::close(_tcp_fds[i].fd);
    }
    _tcp_fds.clear();
    _clients.clear();

    if (_socket.isValid()) {
        _socket.close();
    }
    _running = false;
}

bool Server::send(const Address& dest, const std::vector<uint8_t>& data) {
    if (!_running || !_socket.isValid()) {
        std::cerr << "Server is not running or socket is invalid"
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
        sent = _socket.sendTo(
            fullPacket.data(),
            fullPacket.size(),
            dest);
    } else {
        for (size_t i = 0; i < _clients.size(); ++i) {
            if (_clients[i].first == dest) {
// le +1 c pcq j'ai mis le server fd a index 0 je vais y mettre une macro tkt
                int client_fd = _tcp_fds[i + 1].fd;

                sent = ::send(
                    client_fd,
                    fullPacket.data(),
                    fullPacket.size(),
                    0);
                break;
            }
        }

        if (sent == 0) {
            std::cerr << "Client not found"
                      << std::endl;
            return false;
        }
    }

    if (sent < 0) {
        std::cerr << "Failed to send data"
                  << std::endl;
        return false;
    }

    if (static_cast<size_t>(sent) != fullPacket.size()) {
        std::cerr << "Partial send: "
                  << sent
                  << " / "
                  << fullPacket.size()
                  << " bytes"
                  << std::endl;
        return false;
    }

    return true;
}

int Server::receive(void* buffer, size_t max_size, Address& sender) {
    if (!_running || !_socket.isValid()) {
        std::cerr << "Server is not running or socket is invalid"
                  << std::endl;
        return -1;
    }

    if (buffer == nullptr || max_size == 0) {
        std::cerr << "Invalid buffer or size"
                  << std::endl;
        return -1;
    }

    size_t tempBsize = max_size + _protocol.getProtocolOverhead();
    std::vector<uint8_t> tempBuffer(tempBsize);
    int received = 0;

    if (_socket.getType() == SocketType::UDP) {
        received = _socket.receiveFrom(tempBuffer.data(), tempBsize, sender);
    } else {
        if (_clients.empty()) {
            std::cerr << "No TCP clients connected"
                      << std::endl;
            return -1;
        }

        for (size_t i = 0; i < _clients.size(); ++i) {
            if (_clients[i].first == sender) {
// le +1 c pcq j'ai mis le server fd a index 0 je vais y mettre une macro tkt
                int client_fd = _tcp_fds[i + 1].fd;
                received = ::recv(client_fd, tempBuffer.data(), tempBsize, 0);

                if (received == 0) {
                    ::close(client_fd);
                    _tcp_fds.erase(_tcp_fds.begin() + i + 1);
                    _clients.erase(_clients.begin() + i);
                }
                break;
            }
        }
    }

    if (received < 0) {
        std::cerr << "Failed to receive data"
                  << std::endl;
        return -1;
    }

    if (received == 0) {
        return 0;
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

// Elle est immense cette dinde je la split demain
int Server::loopReceive(int timeout_ms, size_t maxInputs) {
    if (!_running || !_socket.isValid()) {
        std::cerr << "Server is not running or socket is invalid" << std::endl;
        return -1;
    }

    if (maxInputs == 0) {
        std::cerr << "Invalid maxInputs" << std::endl;
        return -1;
    }

    int connWithData = 0;
    size_t bufsiz = 65536 + _protocol.getProtocolOverhead();

    if (_socket.getType() == SocketType::UDP) {
        // Jsp frr il est tard le poll + udp ca me parait bizarre
        pollfd pfd;
        pfd.fd = _socket.getSocket();
        pfd.events = POLLIN;
        pfd.revents = 0;

        int poll_result = poll(&pfd, 1, timeout_ms);
        if (poll_result < 0)
            return -1;
        if (poll_result == 0)
            return 0;

        for (size_t count = 0; count < maxInputs; count++) {
            Address sender;
            std::vector<uint8_t> buffer(bufsiz);

            int received = _socket.receiveFrom(buffer.data(), bufsiz, sender);
            if (received < 0)
                break;

            // c broken ca resize tout seul en enlevant le surplus
            buffer.resize(received);

            bool found = false;
            for (auto& client : _clients) {
                if (client.first == sender) {
                    client.second = buffer;
                    found = true;
                    break;
                }
            }
            if (!found) {
                _clients.push_back(std::make_pair(sender, buffer));
            }

            connWithData++;
        }
    } else {
        // TCP: poll server socket (index 0) + all connected clients
        if (_tcp_fds.empty()) {
            std::cerr << "No TCP socket available" << std::endl;
            return -1;
        }

        // Reset revents for all fds
        for (auto& pfd : _tcp_fds) {
            pfd.revents = 0;
        }

        int poll_result = poll(_tcp_fds.data(), _tcp_fds.size(), timeout_ms);

        if (poll_result < 0)
            return -1;

        if (poll_result == 0)
            return 0;

        // je vais caser ca dans une fonction newConnection
        if (_tcp_fds[0].revents & POLLIN) {
            Address client_addr;
            int client_fd = acceptClient(client_addr);
            if (client_fd >= 0) {
                connWithData++;
            }
        }

        for (size_t i = 1;
        i < _tcp_fds.size() && connWithData < static_cast<int>(maxInputs);
        i++) {
            if (!(_tcp_fds[i].revents & POLLIN)) {
                continue;
            }

            size_t client_index = i - 1;  // magic values de SERVERFDLEN = 1
            if (client_index >= _clients.size()) {
                std::cerr << "Client index out of bounds" << std::endl;
                continue;
            }

            _clients[client_index].second.resize(bufsiz);

            int received = ::recv(_tcp_fds[i].fd,
                                _clients[client_index].second.data(),
                                bufsiz,
                                0);

            if (received == 0) {
                // Connection closed
                ::close(_tcp_fds[i].fd);
                _tcp_fds.erase(_tcp_fds.begin() + i);
                _clients.erase(_clients.begin() + client_index);
                i--;  // faut se recaler pcq on a delete un client
                continue;
            }

            if (received < 0)
                continue;

            _clients[client_index].second.resize(received);

            connWithData++;
        }
    }

    return connWithData;
}

int Server::acceptClient(Address& client_addr) {
    if (_socket.getType() != SocketType::TCP) {
        std::cerr << "acceptClient() is only for TCP mode"
                  << std::endl;
        return -1;
    }

    if (!_running || !_socket.isValid()) {
        std::cerr << "Server is not running"
                  << std::endl;
        return -1;
    }

    int client_fd = _socket.accept(client_addr);
    if (client_fd < 0) {
        std::cerr << "Failed to accept client connection"
                  << std::endl;
        return -1;
    }

    std::vector<uint8_t> empty_buffer;
    _clients.push_back(std::make_pair(client_addr, empty_buffer));

    pollfd client_pfd;
    client_pfd.fd = client_fd;
    client_pfd.events = POLLIN;
    client_pfd.revents = 0;
    _tcp_fds.push_back(client_pfd);

    return client_fd;
}

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
