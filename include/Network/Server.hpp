#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>

#include "Network/NetworkPlatform.hpp"
#include "Network/Address.hpp"
#include "Network/Packet.hpp"
#include "Network/NetworkSocket.hpp"
#include "Network/ProtocolManager.hpp"

#define CAST_UINT32 static_cast<uint32_t>

namespace net {

/**
 * @brief Communicate in UDP or TCP with multiple Client
 * 
 * Use ProtocolManager class to format and unformat packets sent and received.
 * @see ProtocolManager
 */
class Server {
 public:
    /**
     * @brief Construct a new Server object
     * 
     * @param protocol Choose the communication type of your Server. Mode -> "UDP" or "TCP".
     */
    explicit Server(const std::string&, uint16_t, ProtocolManager);

    /**
     * @brief Destroy the Server object
     * 
     * Call stop method
     * @see Server#stop
     */
    ~Server();

    /**
     * @brief Start the Server
     * 
     * @return true If succeed
     * @return false If failed (bind failed, listen failed, ...)
     */
    bool start();

    /**
     * @brief Stop the Server
     * 
     * Close all sockets connected to and clear vectors.
     */
    void stop();

    /**
     * @brief Set the Server non-blocking
     * It will not wait packets before doing something
     * @param enabled true if you wrant to set it non-blocking, false if you don't
     * @return true If succeed
     * @return false If failed (not connected to a Server)
     */
    bool setNonBlocking(bool enabled);

    /**
     * @brief Set the Timeout in poll()
     * 
     * @param milliseconds The timeout to set (in ms)
     * @return true If succeed
     * @return false If failed (Socket invalid)
     */
    bool setTimeout(int milliseconds);

    /**
     * @brief Send data to specific client
     * 
     * @param dest FD of the client
     * @param data Datas that will be sent
     * @return int Size of datas sent 
     */
    int tcpSend(const int dest, std::vector<uint8_t> data);

    /**
     * @brief Send data to specific client
     * 
     * @param dest Address of the client. @see Address
     * @param data Datas that will be sent
     * @return int Size of datas sent
     */
    int udpSend(const Address& dest, std::vector<uint8_t> data);

    /**
     * @brief Receive datas sent by connected clients (TCP mode)
     * 
     * @param timeout Time to wait for receive
     * @return std::vector<int> Vector of FD from which data has been received
     */
    std::vector<int> tcpReceive(int timeout);

    /**
     * @brief Receive datas sent by connected clients (UDP mode)
     * 
     * @param timeout Time to wait for receive
     * @param maxInputs Max input to get from clients before stop receiving
     * @return std::vector<Address> Vector of Address from which data has been received
     */
    std::vector<Address> udpReceive(int timeout, int maxInputs);

    /**
     * @brief Extract packet from datas on ClientInfo->input and remove protocol informations (TCP mode)
     * 
     * @param src Client's FD that you want to unpack datas
     * @param nbPackets Number of packets you want to extract
     * @return std::vector<std::vector<uint8_t>> Unpacked datas
     */
    std::vector<std::vector<uint8_t>> unpack(int src, int nbPackets);

    /**
     * @brief Extract packet from datas on ClientInfo->input and remove protocol informations (UDP mode)
     * 
     * @param src Client's Address that you want to unpack datas
     * @param nbPackets Number of packets you want to extract
     * @return std::vector<std::vector<uint8_t>> Unpacked Datas @see
     */
    std::vector<std::vector<uint8_t>> unpack(const Address& src, int nbPackets);

    /**
     * @brief Return the state of the server
     * 
     * @return true Means running
     * @return false Means not running
     */
    bool isRunning() const { return _running; }

    /**
     * @brief Get the Protocol object
     * 
     * @return SocketType The protocol type (UDP or TCP)
     */
    SocketType getProtocol() const { return _socket.getType(); }

    /**
     * @brief Get Server's port
     * 
     * @return uint16_t Server's port
     */
    uint16_t getPort() const { return _port; }

    /**
     * @brief Accept client in TCP mode
     * 
     * @param client_addr Client's Address
     * @param currentTime Timestamp at the connection
     * @return int Client's FD if accepted
     */
    int acceptClient(Address& client_addr, uint64_t currentTime);

    struct ClientInfo {
        uint64_t lastPacketTime;
        std::vector<uint8_t> input;
        std::vector<uint8_t> output;
    };

    // Testing purpose
    const std::unordered_map<Address, ClientInfo>& getUdpClients() const;
    const std::unordered_map<int, ClientInfo>& getTcpClients() const;
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

    std::vector<POLLFD> _tcp_fds;

    std::unordered_map<int, Address> _tcp_links;

    std::unordered_map<Address, ClientInfo> _udp_clients;
    std::unordered_map<int, ClientInfo> _tcp_clients;
};

}  // namespace net
