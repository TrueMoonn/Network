#pragma once

#include <string>
#include <vector>

#include "Network/Address.hpp"
#include "Network/Packet.hpp"
#include "Network/NetworkSocket.hpp"
#include "Network/ProtocolManager.hpp"

class Client {
 public:
    explicit Client(const std::string& protocol = "UDP");
    ~Client();

    bool connect(const std::string& server_ip, uint16_t server_port);
    void disconnect();

    bool send(const std::vector<uint8_t>& data);
    int receive(void* buffer, size_t max_size);

    bool setNonBlocking(bool enabled);
    bool setTimeout(int milliseconds);

    bool isConnected() const { return _connected; }
    const Address& getServerAddress() const { return _server_address; }
    SocketType getProtocol() const { return _socket.getType(); }

 private:
    NetworkSocket _socket;
    Address _server_address;
    bool _connected;
    ProtocolManager _protocol;
};
