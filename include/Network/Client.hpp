#pragma once

#include <string>
#include <vector>

#include "Network/Address.hpp"
#include "Network/Packet.hpp"
#include "Network/NetworkSocket.hpp"
#include "Network/ProtocolManager.hpp"
#include "Network/PacketSerializer.hpp"

#define CAST_UINT32 static_cast<uint32_t>

namespace net {

class Client {
 public:
    explicit Client(const std::string& protocol = "UDP");
    ~Client();

    bool connect(const std::string& server_ip, uint16_t server_port);
    void disconnect();

    bool send(const std::vector<uint8_t>& data);

    template<typename T>
    bool sendPacket(const T& packet) {
        std::vector<uint8_t> data = PacketSerializer::serialize(packet);
        return send(data);
    }

    int receive(void* buffer, size_t max_size);
    void udpReceive(int timeout = 0, int maxInputs = 10);
    void tcpReceive(int timeout = 0);
    std::vector<std::vector<uint8_t>> extractPacketsFromBuffer();

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

    std::vector<uint8_t> _input_buffer;
};

}  // namespace net
