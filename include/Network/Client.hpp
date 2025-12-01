#pragma once
#include "Network/Address.hpp"
#include "Network/Packet.hpp"
#include "Network/NetworkSocket.hpp"
#include <string>

class Client {
public:
    Client(const std::string& protocol = "UDP");
    ~Client();

    bool connect(const std::string& server_ip, uint16_t server_port);
    void disconnect();

    bool send(const void* data, size_t size);
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
};
