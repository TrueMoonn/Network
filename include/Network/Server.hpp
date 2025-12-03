#pragma once

#include <poll.h>

#include <string>
#include <map>
#include <unordered_map>
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

    // envoi
    int tcpSend(const int dest, std::vector<uint8_t> data);
    int udpSend(const Address& dest, std::vector<uint8_t> data);

    // reception
    std::vector<int> tcpReceive(int timeout);
    std::vector<Address> udpReceive(int timeout, int maxInputs);

    bool isRunning() const { return _running; }
    SocketType getProtocol() const { return _socket.getType(); }
    uint16_t getPort() const { return _port; }

    // TCP
    int acceptClient(Address& client_addr, uint currentTime);

    struct ClientInfo {
        uint lastPacketTime;
        std::vector<uint8_t> input;
        std::vector<uint8_t> output;
    };

    #define NB_SERVERFD 1
    #define NOFD -1
    #define NOTSENT -1

 private:
    uint16_t _port;
    NetworkSocket _socket;
    bool _running;

    ProtocolManager _protocol;

    std::vector<pollfd> _tcp_fds;

    std::unordered_map<Address, ClientInfo> _udp_clients;
    std::unordered_map<int, ClientInfo> _tcp_clients;

    std::unordered_map<int, Address> _tcp_links;

    // // format [ IP:PORT, [01,02,03,04] ], ...
    // std::vector<std::pair<Address, std::vector<uint8_t> > > _clients;
};
