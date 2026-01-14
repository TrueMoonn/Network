#include <cstring>
#include <cstdio>
#include <ctime>
#include <string>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <vector>

#include "Network/Server.hpp"
#include "Network/ProtocolManager.hpp"
#include "Network/Logger.hpp"

namespace net {

Server::Server(
    const std::string& protocol,
    uint16_t port,
    ProtocolManager protocolManager)
    : _port(port), _running(false), _protocol(protocolManager),
    _logger(true, "./logs", "server") {
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
        POLLFD server_pfd;
        server_pfd.fd = _socket.getSocket();
        server_pfd.events = POLL_IN;
        server_pfd.revents = 0;
        _tcp_fds.push_back(server_pfd);
    }
    _logger.write("==============================");
    _logger.write("Server initialized ready to listen");
}

Server::~Server() {
    stop();
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

bool Server::start() {
    if (_running)
        throw ServerAlreadyStarted();

    _socket.setReuseAddr(true);

    if (!_socket.bind(_port)) {
        _socket.close();
        _logger.write("ERROR\tSocket binding failed");
        throw NetworkSocket::BindFailed();
    }

    if (_socket.getType() == SocketType::TCP) {
        if (!_socket.listen(10)) {
            _socket.close();
            _logger.write("ERROR\tListen failed");
            throw NetworkSocket::ListenFailed();
        }
    }

    _logger.write("Server listening on port " +
        std::to_string(_port) +
        " using protocol " +
        (_socket.getType() == SocketType::TCP ? "TCP" : "UDP"));

    _running = true;
    return true;
}

void Server::stop() {
    for (size_t i = 1; i < _tcp_fds.size(); ++i) {
        CLOSE_SOCKET(static_cast<SocketHandle>(_tcp_fds[i].fd));
    }
    _tcp_fds.clear();
    _udp_clients.clear();
    _tcp_clients.clear();

    if (_socket.isValid())
        _socket.close();
    _running = false;
    _logger.write("Server stopped");
    _logger.write("==============================");
}

int Server::acceptClient(Address& client_addr, uint64_t currentTime) {
    if (_socket.getType() != SocketType::TCP) {
        _logger.write("ERROR\tTried an accept when using UDP mode");
        throw NetworkSocket::InvalidSocketType(
            "acceptClient() is only for TCP mode");
    }

    if (!_running) {
        _logger.write("ERROR\tCannnot accept before starting server");
        throw ServerNotStarted();
    }
    if (!_socket.isValid()) {
        _logger.write("ERROR\tCannot accept with no socket created");
        throw NetworkSocket::SocketNotCreated();
    }

    int client_fd = _socket.accept(client_addr);
    if (client_fd < 0) {
        _logger.write("ERROR\tAccept error (fd < 0)");
        throw NetworkSocket::AcceptFailed();
    }

    ClientInfo newClient;
    newClient.lastPacketTime = currentTime;
    newClient.input.clear();
    newClient.output.clear();

    _tcp_clients.insert(std::make_pair(client_fd, newClient));

    POLLFD client_pfd;
    client_pfd.fd = client_fd;
    client_pfd.events = POLL_IN;
    client_pfd.revents = 0;
    _tcp_fds.push_back(client_pfd);

    return client_fd;
}

std::string dataToString(std::vector<uint8_t> buff) {
    std::string res;

    for (auto& byte : buff)
        res += std::to_string(byte) + " ";
    return res;
}

int Server::udpSend(const Address& dest, std::vector<uint8_t> data) {
    if (!_running) {
        _logger.write("ERROR\tCannot send before starting server");
        throw ServerNotStarted();
    }
    if (!_socket.isValid()) {
        _logger.write("ERROR\tCannot send before setting socket");
        throw NetworkSocket::SocketNotCreated();
    }
    if (_socket.getType() == SocketType::TCP) {
        _logger.write("ERROR\tCannot send in UDP mode when socket is TCP");
        throw NetworkSocket::InvalidSocketType(
            "Socket type is TCP, udpSend() is for UDP only");
    }

    if (data.empty()) {
        _logger.write("ERROR\tCannot send empty packet");
        throw BadData();
    }

    auto it = _udp_clients.find(dest);
    if (it == _udp_clients.end()) {
        _logger.write("ERROR\tUnknown address given to send");
        throw UnknownAddressOrFd();
    }

    std::vector<uint8_t> fullPacket = _protocol.formatPacket(data);

    _logger.write(
        "SEND\t" +
        dest.getIP() +
        ":" +
        std::to_string(dest.getPort()) +
        "\t" +
        dataToString(fullPacket));

    int sent = _socket.sendTo(fullPacket.data(), fullPacket.size(), dest);

    if (sent < 0) {
        _logger.write("ERROR\tFailed to send data to given dest");
        throw NetworkSocket::DataSendFailed();
    }
    return sent;
}

int Server::tcpSend(int dest, std::vector<uint8_t> data) {
    if (!_running) {
        _logger.write("ERROR\tCannot send before starting server");
        throw ServerNotStarted();
    }
    if (!_socket.isValid()) {
        _logger.write("ERROR\tCannot send before setting socket");
        throw NetworkSocket::SocketNotCreated();
    }
    if (_socket.getType() == SocketType::UDP) {
        _logger.write("ERROR\tCannot send in TCP mode when socket is UDP");
        throw NetworkSocket::InvalidSocketType(
            "Socket type is UDP, tcpSend() is for UDP only");
    }

    if (data.empty()) {
        _logger.write("ERROR\tCannot send empty packet");
        throw BadData();
    }

    auto it = _tcp_clients.find(dest);
    if (it == _tcp_clients.end()) {
        _logger.write("ERROR\tUnknown address given to send");
        throw UnknownAddressOrFd();
    }

    std::vector<uint8_t> fullPacket = _protocol.formatPacket(data);

    _logger.write(
        "SEND\t" +
        std::to_string(dest) +
        "\t" +
        dataToString(fullPacket));

    size_t totalSent = 0;
    while (totalSent < fullPacket.size()) {
        int sent = ::send(dest, reinterpret_cast<const char*>(fullPacket.data()
            + totalSent), static_cast<int>(fullPacket.size() - totalSent), 0);
        if (sent == SOCKET_ERROR_VALUE || sent == 0) {
            _logger.write("ERROR\tFailed to send data to given dest");
            throw NetworkSocket::DataSendFailed();
        }
        totalSent += sent;
    }
    return static_cast<int>(totalSent);
}

std::vector<Address> Server::udpReceive(int timeout, int maxInputs) {
    std::vector<Address> results;
    if (!_running) {
        _logger.write("ERROR\tCannot send before starting server");
        throw ServerNotStarted();
    }
    if (!_socket.isValid()) {
        _logger.write("ERROR\tCannot send before setting socket");
        throw NetworkSocket::SocketNotCreated();
    }
    if (_socket.getType() == SocketType::TCP) {
        _logger.write("ERROR\tCannot receive in UDP mode when socket is TCP");
        throw NetworkSocket::InvalidSocketType(
            "Socket type is TCP, udpReceive() is for UDP only");
    }

    POLLFD pfd;
    pfd.fd = _socket.getSocket();
    pfd.events = POLL_IN;
    pfd.revents = 0;

    int poll_result = PollSockets(&pfd, 1, timeout);
    if (poll_result < 0)
        return results;
    if (poll_result == 0)
        return results;

    size_t bufsiz = BUFSIZ + _protocol.getProtocolOverhead();

    for (size_t count = 0; count < maxInputs; count++) {
        Address sender;
        std::vector<uint8_t> buffer(bufsiz);

        int received = _socket.receiveFrom(buffer.data(), bufsiz, sender);
        if (received <= 0)
            break;

        buffer.resize(received);

        _logger.write(
            "RECV\t" +
            sender.getIP() +
            ":" +
            std::to_string(sender.getPort()) +
            "\t" +
            dataToString(buffer));

        uint64_t currentTime = static_cast<uint64_t>(std::time(nullptr));

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

    if (!_running) {
        _logger.write("ERROR\tCannot send before starting server");
        throw ServerNotStarted();
    }
    if (!_socket.isValid()) {
        _logger.write("ERROR\tCannot send before setting socket");
        throw NetworkSocket::SocketNotCreated();
    }
    if (_tcp_fds.empty()) {
        _logger.write("ERROR\tCannot send before setting socket");
        throw NoTcpSocket();
    }
    if (_socket.getType() == SocketType::UDP) {
        _logger.write("ERROR\tCannot receive in TCP mode when socket is UDP");
        throw NetworkSocket::InvalidSocketType(
            "Socket type is UDP, tcpReceive() is for TCP only");
    }

    for (auto& pfd : _tcp_fds)
        pfd.revents = 0;

#ifdef _WIN32
    int poll_result = PollSockets(_tcp_fds.data(),
        static_cast<ULONG>(_tcp_fds.size()), timeout);
#else
    int poll_result = PollSockets(_tcp_fds.data(), _tcp_fds.size(), timeout);
#endif
    if (poll_result < 0) {
        _logger.write("ERROR\tPoll error in TCP receive");
        throw PollError();
    }
    if (poll_result == 0)
        return results;

    uint64_t currentTime = static_cast<uint64_t>(std::time(nullptr));
    if (_tcp_fds[0].revents & POLL_IN) {
        Address client_addr;
        int client_fd = acceptClient(client_addr, currentTime);
    }

    size_t bufsiz = BUFSIZ + _protocol.getProtocolOverhead();
    for (size_t i = 1; i < _tcp_fds.size(); i++) {
        int client_fd = static_cast<int>(_tcp_fds[i].fd);

        // Check for errors or hangup (connection closed by peer)
        if ((_tcp_fds[i].revents & POLL_ERR)
            || (_tcp_fds[i].revents & POLL_HUP)) {
            CLOSE_SOCKET(client_fd);
            _tcp_fds.erase(_tcp_fds.begin() + i);
            _tcp_clients.erase(client_fd);
            _tcp_links.erase(client_fd);
            i--;
            continue;
        }

        if (!(_tcp_fds[i].revents & POLL_IN))
            continue;

        size_t client_index = i - NB_SERVERFD;

        std::vector<uint8_t> buffer(bufsiz);

        int received = ::recv(client_fd, reinterpret_cast<char*>(buffer.data()),
                             static_cast<int>(bufsiz), 0);

        _logger.write(
            "RECV\t" +
            std::to_string(client_fd) +
            "\t" +
            dataToString(buffer));

        if (received == 0) {
            CLOSE_SOCKET(client_fd);
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
                                    buffer.begin() + received);
            results.push_back(client_fd);
        }
    }
    return results;
}

// sketchy j'ai pas le temps de tester
std::vector<std::vector<uint8_t>> Server::getDataFromBuffer(
        int nbPackets, ClientInfo& client) {
    std::vector<std::vector<uint8_t>> result;

    ProtocolManager::preambule preamble = _protocol.getPreambule();
    ProtocolManager::datetime datetime = _protocol.getDatetime();
    ProtocolManager::packet_length packetLength = _protocol.getPacketLength();
    ProtocolManager::end_of_packet packetEnd = _protocol.getEndOfPacket();
    ProtocolManager::Endianness endianness = _protocol.getEndianness();

    int packetsToUnpack = (nbPackets < 0) ? 1000 : nbPackets;
    int packetCount = 0;

    while (!client.input.empty() && packetCount < packetsToUnpack) {
        size_t offset = 0;
        if (preamble.active) {
            if (client.input.size() < preamble.characters.size())
                break;
            offset += preamble.characters.size();
        }

        uint32_t dataLength = 0;

        if (packetLength.active) {
            if (client.input.size() < offset + packetLength.length)
                break;

            if (endianness == ProtocolManager::Endianness::LITTLE) {
                if (packetLength.length == 4) {
                    dataLength = CAST_UINT32(client.input[offset]) |
                               (CAST_UINT32(client.input[offset + 1]) << 8) |
                               (CAST_UINT32(client.input[offset + 2]) << 16) |
                               (CAST_UINT32(client.input[offset + 3]) << 24);
                } else if (packetLength.length == 2) {
                    dataLength = CAST_UINT32(client.input[offset]) |
                               (CAST_UINT32(client.input[offset + 1]) << 8);
                } else if (packetLength.length == 1) {
                    dataLength = client.input[offset];
                }
            } else {
                if (packetLength.length == 4) {
                    dataLength = (CAST_UINT32(client.input[offset]) << 24) |
                               (CAST_UINT32(client.input[offset + 1]) << 16) |
                               (CAST_UINT32(client.input[offset + 2]) << 8) |
                               CAST_UINT32(client.input[offset + 3]);
                } else if (packetLength.length == 2) {
                    dataLength = (CAST_UINT32(client.input[offset]) << 8) |
                               CAST_UINT32(client.input[offset + 1]);
                } else if (packetLength.length == 1) {
                    dataLength = client.input[offset];
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
                if (client.input.size() < offset + datetime.length)
                    break;
                offset += datetime.length;
            }

            if (client.input.size() < offset + actualDataLength)
                break;

            std::vector<uint8_t> packetData(
                    client.input.begin() + offset,
                    client.input.begin() + offset + actualDataLength);

            result.push_back(packetData);
            packetCount++;
            offset += actualDataLength;
            if (packetEnd.active) {
                if (client.input.size()
                    < offset + packetEnd.characters.size()) {
                    break;
                }
                offset += packetEnd.characters.size();
            }

            client.input.erase(client.input.begin(),
                    client.input.begin() + offset);

        } else if (packetEnd.active) {
            if (datetime.active) {
                if (client.input.size() < offset + datetime.length)
                    break;
                offset += datetime.length;
            }

            std::string endMarker = packetEnd.characters;
            auto it = std::search(
                    client.input.begin() + offset,
                    client.input.end(),
                    endMarker.begin(),
                    endMarker.end());

            if (it == client.input.end())
                break;

            size_t dataEnd = std::distance(client.input.begin(), it);
            size_t dataStart = offset;
            dataLength = dataEnd - dataStart;

            std::vector<uint8_t> packetData(
                    client.input.begin() + dataStart,
                    client.input.begin() + dataEnd);

            result.push_back(packetData);
            packetCount++;

            client.input.erase(client.input.begin(),
                    client.input.begin() + dataEnd + endMarker.size());
        } else {
            _logger.write("ERROR\tData unpacking error, probably bad format");
            throw BadData();
        }
    }

    return result;
}

std::vector<std::vector<uint8_t>> Server::unpack(int src, int nbPackets) {
    auto it = _tcp_clients.find(src);
    if (it == _tcp_clients.end()) {
        _logger.write("ERROR\tUnknown fd given to unpack data");
        throw UnknownAddressOrFd();
    }

    ClientInfo& client = it->second;

    return getDataFromBuffer(nbPackets, client);
}

std::vector<std::vector<uint8_t>> Server::unpack(
        const Address& src, int nbPackets) {
    auto it = _udp_clients.find(src);
    if (it == _udp_clients.end()) {
        _logger.write("ERROR\tUnknown address given to unpack data");
        throw UnknownAddressOrFd();
    }

    ClientInfo& client = it->second;

    return getDataFromBuffer(nbPackets, client);
}

const std::unordered_map<Address, Server::ClientInfo>& Server::getUdpClients()
    const {
    return _udp_clients;
}

const std::unordered_map<int, Server::ClientInfo>& Server::getTcpClients()
    const {
    return _tcp_clients;
}

std::unordered_map<Address, Server::ClientInfo>& Server::getUdpClientsRef() {
    return _udp_clients;
}

std::unordered_map<int, Server::ClientInfo>& Server::getTcpClientsRef() {
    return _tcp_clients;
}

}  // namespace net
