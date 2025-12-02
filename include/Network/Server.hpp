#pragma once

#include <string>
#include <map>
#include <vector>

#include "Network/Address.hpp"
#include "Network/Packet.hpp"
#include "Network/NetworkSocket.hpp"
#include "Network/ProtocolManager.hpp"

class Server {
 public:
    explicit Server(const std::string& protocol, uint16_t port);
    ~Server();

    bool start();
    void stop();

    bool setNonBlocking(bool enabled);
    bool setTimeout(int milliseconds);

    bool send(const Address& dest, const std::vector<uint8_t>& data);
    int receive(void* buffer, size_t max_size, Address& sender);

    bool isRunning() const { return _running; }
    SocketType getProtocol() const { return _socket.getType(); }
    uint16_t getPort() const { return _port; }

    // TCP
    int acceptClient(Address& client_addr);

 private:
    uint16_t _port;
    NetworkSocket _socket;
    bool _running;

    ProtocolManager _protocol;

    std::map<int, Address> _tcp_clients;
};
