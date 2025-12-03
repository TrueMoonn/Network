#pragma once

#include <poll.h>

#include <string>
#include <map>
#include <vector>

#include "Network/Address.hpp"
#include "Network/Packet.hpp"
#include "Network/NetworkSocket.hpp"
#include "Network/ProtocolManager.hpp"

class Server {
 public:
    explicit Server(const std::string&, uint16_t, ProtocolManager);
    ~Server();

    bool start();
    void stop();

    bool setNonBlocking(bool enabled);
    bool setTimeout(int milliseconds);

    bool send(const Address& dest, const std::vector<uint8_t>& data);
    int receive(void* buffer, size_t max_size, Address& sender);
    int loopReceive(int timeout_ms, size_t maxInputs);

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

    std::vector<pollfd> _tcp_fds;

    // format [ IP:PORT, [01,02,03,04] ], ...
    std::vector<std::pair<Address, std::vector<uint8_t> > > _clients;
};
