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

    // lecture
    std::vector<std::vector<uint8_t>> unpack(int src, int nbPackets);
    std::vector<std::vector<uint8_t>> unpack(const Address& src, int nbPackets);

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

    // Accessors for testing purposes
    const std::unordered_map<Address, ClientInfo>& getUdpClients() const;
    const std::unordered_map<int, ClientInfo>& getTcpClients() const;

    // Non-const accessors for test manipulation
    std::unordered_map<Address, ClientInfo>& getUdpClientsRef();
    std::unordered_map<int, ClientInfo>& getTcpClientsRef();

    #define NB_SERVERFD 1
    #define NOFD -1
    #define NOTSENT -1

    class BadProtocol : public std::exception {
     public:
        const char* what() const noexcept override {
            return "Bad protocol given, defaulting to UDP";
        }
    };

    class NoTcpSocket : public std::exception {
     public:
        const char* what() const noexcept override {
            return "No TCP Socket in fds for poll";
        }
    };

    class ServerNotStarted : public std::exception {
     public:
        const char* what() const noexcept override {
            return "Start the server before trying to send or receive data";
        }
    };

    class ServerAlreadyStarted : public std::exception {
     public:
        const char* what() const noexcept override {
            return "The server is already running";
        }
    };

    class PollError : public std::exception {
     public:
        const char* what() const noexcept override {
            return "Poll error in receive";
        }
    };

    class BadData : public std::exception {
     public:
        const char* what() const noexcept override {
            return "Invalid data or size";
        }
    };

    class UnknownAddressOrFd : public std::exception {
     public:
        const char* what() const noexcept override {
            return "Unknown address or fd, will ignore tasks";
        }
    };

 private:
    std::vector<std::vector<uint8_t>> getDataFromBuffer(
            int nbPackets, ClientInfo &client);

    uint16_t _port;
    NetworkSocket _socket;
    bool _running;

    ProtocolManager _protocol;

    std::vector<pollfd> _tcp_fds;

    std::unordered_map<int, Address> _tcp_links;
    
    std::unordered_map<Address, ClientInfo> _udp_clients;
    std::unordered_map<int, ClientInfo> _tcp_clients;

    // // format [ IP:PORT, [01,02,03,04] ], ...
    // std::vector<std::pair<Address, std::vector<uint8_t> > > _clients;
};
