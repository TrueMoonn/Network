#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <string>
#include <unordered_map>
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
        throw BadProtocol();
    }
    _socket = NetworkSocket(type);
    if (!_socket.create(type))
        throw NetworkSocket::SocketCreationError();

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
    if (_running)
        throw ServerAlreadyStarted();

    _socket.setReuseAddr(true);

    if (!_socket.bind(_port)) {
        _socket.close();
        throw NetworkSocket::BindFailed();
    }

    if (_socket.getType() == SocketType::TCP) {
        if (!_socket.listen(10)) {
            _socket.close();
            throw NetworkSocket::ListenFailed();
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
    _udp_clients.clear();
    _tcp_clients.clear();

    if (_socket.isValid())
        _socket.close();
    _running = false;
}

int Server::udpSend(const Address& dest, std::vector<uint8_t> data) {
    if (!_running)
        throw ServerNotStarted();
    if (!_socket.isValid())
        throw NetworkSocket::SocketNotCreated();
    if (_socket.getType() == SocketType::TCP)
        throw NetworkSocket::InvalidSocketType(
            "Socket type is TCP, udpSend() is for UDP only");

    if (data.empty())
        throw BadData();

    auto it = _udp_clients.find(dest);
    if (it == _udp_clients.end())
        throw UnknownAddressOrFd();

    std::vector<uint8_t> fullPacket = _protocol.formatPacket(data);

    int sent = _socket.sendTo(fullPacket.data(), fullPacket.size(), dest);

    if (sent < 0)
        throw NetworkSocket::DataSendFailed();
    return sent;
}

int Server::tcpSend(int dest, std::vector<uint8_t> data) {
    if (!_running)
        throw ServerNotStarted();
    if (!_socket.isValid())
        throw NetworkSocket::SocketNotCreated();
    if (_socket.getType() == SocketType::UDP)
        throw NetworkSocket::InvalidSocketType(
            "Socket type is UDP, tcpSend() is for UDP only");

    if (data.empty())
        throw BadData();

    auto it = _tcp_clients.find(dest);
    if (it == _tcp_clients.end())
        throw UnknownAddressOrFd();

    std::vector<uint8_t> fullPacket = _protocol.formatPacket(data);

    int sent = ::send(dest, fullPacket.data(), fullPacket.size(), 0);

    if (sent == 0)
        throw NetworkSocket::DataSendFailed();
    return sent;
}

std::vector<Address> Server::udpReceive(int timeout, int maxInputs) {
    std::vector<Address> results;
    if (!_running)
        throw ServerNotStarted();
    if (!_socket.isValid())
        throw NetworkSocket::SocketNotCreated();
    if (_socket.getType() == SocketType::UDP)
        throw NetworkSocket::InvalidSocketType(
            "Socket type is TCP, udpReceive() is for UDP only");

    pollfd pfd;
    pfd.fd = _socket.getSocket();
    pfd.events = POLLIN;
    pfd.revents = 0;

    int poll_result = poll(&pfd, 1, timeout);
    if (poll_result < 0)
        return results;
    if (poll_result == 0)
        return results;

    size_t bufsiz = BUFSIZ + _protocol.getProtocolOverhead();

    for (size_t count = 0; count < maxInputs; count++) {
        Address sender;
        std::vector<uint8_t> buffer(bufsiz);

        int received = _socket.receiveFrom(buffer.data(), bufsiz, sender);
        if (received < 0)
            break;

        buffer.resize(received);

        uint currentTime = static_cast<uint>(std::time(nullptr));

        auto it = _udp_clients.find(sender);
        if (it == _udp_clients.end()) {
            ClientInfo newClient;
            newClient.lastPacketTime = currentTime;
            newClient.input = buffer;
            newClient.output.clear();

            _udp_clients.insert(std::make_pair(sender, newClient));
        } else {
            it->second.input.insert(it->second.input.end(),
                                    buffer.begin(),
                                    buffer.end());
            it->second.lastPacketTime = currentTime;
        }
        results.push_back(sender);
    }
    return results;
}

std::vector<int> Server::tcpReceive(int timeout) {
    std::vector<int> results;

    if (!_running)
        throw ServerNotStarted();
    if (!_socket.isValid())
        throw NetworkSocket::SocketNotCreated();
    if (_tcp_fds.empty())
        throw NoTcpSocket();
    if (_socket.getType() == SocketType::UDP)
        throw NetworkSocket::InvalidSocketType(
            "Socket type is UDP, tcpReceive() is for TCP only");

    for (auto& pfd : _tcp_fds)
        pfd.revents = 0;

    int poll_result = poll(_tcp_fds.data(), _tcp_fds.size(), timeout);
    if (poll_result < 0)
        throw PollError();
    if (poll_result == 0)
        return results;

    uint currentTime = static_cast<uint>(std::time(nullptr));
    if (_tcp_fds[0].revents & POLLIN) {
        Address client_addr;
        int client_fd = acceptClient(client_addr, currentTime);
    }

    size_t bufsiz = BUFSIZ + _protocol.getProtocolOverhead();
    for (size_t i = 1; i < _tcp_fds.size(); i++) {
        if (!(_tcp_fds[i].revents & POLLIN))
            continue;

        size_t client_index = i - NB_SERVERFD;

        int client_fd = _tcp_fds[i].fd;
        std::vector<uint8_t> buffer(bufsiz);

        int received = ::recv(client_fd, buffer.data(), bufsiz, 0);

        if (received == 0) {
            ::close(client_fd);
            _tcp_fds.erase(_tcp_fds.begin() + i);
            _tcp_clients.erase(client_fd);
            _tcp_links.erase(client_fd);
            i--;  // recalage
            continue;
        }

        if (received < 0)
            continue;

        auto it = _tcp_clients.find(client_fd);
        if (it != _tcp_clients.end()) {
            it->second.lastPacketTime = currentTime;
            it->second.input.insert(it->second.input.end(),
                                    buffer.begin(),
                                    buffer.end());
            results.push_back(client_fd);
        }
    }
    return results;
}

int Server::acceptClient(Address& client_addr, uint currentTime) {
    if (_socket.getType() != SocketType::TCP)
        throw NetworkSocket::InvalidSocketType(
            "acceptClient() is only for TCP mode");

    if (!_running)
        throw ServerNotStarted();
    if (!_socket.isValid())
        throw NetworkSocket::SocketNotCreated();

    int client_fd = _socket.accept(client_addr);
    if (client_fd < 0)
            throw NetworkSocket::AcceptFailed();

    ClientInfo newClient;
    newClient.lastPacketTime = currentTime;
    newClient.input.clear();
    newClient.output.clear();

    _tcp_clients.insert(std::make_pair(client_fd, newClient));

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
