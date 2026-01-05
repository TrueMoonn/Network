#pragma once

#include <string>
#include <vector>

#include "Network/Address.hpp"
#include "Network/NetworkSocket.hpp"
#include "Network/ProtocolManager.hpp"
#include "Network/PacketSerializer.hpp"

#define CAST_UINT32 static_cast<uint32_t>

namespace net {

/**
 * @brief Communicate in UDP or TCP with a Server
 *
 * Use ProtocolManager class to format and unformat packets sent and received.
 * @see ProtocolManager
 */
class Client {
 public:
    /**
     * @brief Construct a new Client object
     *
     * @param protocol Choose the communication type of your client. Mode -> "UDP" or "TCP".
     */
    explicit Client(const std::string& protocol = "UDP");

    /**
     * @brief Destroy the Client object
     * Automatically disconnect from the server by calling disconnect method.
     */
    ~Client();

    /**
     * @brief Connect to a Server
     *
     * @param server_ip IP of the Server that you want to connect to.
     * @param server_port Port of the Server that you want to connect to.
     * @return true If the connexion succeed
     * @return false If the connexion fail (too many clients, Server not running...)
     */
    bool connect(const std::string& server_ip, uint16_t server_port);

    /**
     * @brief Disconnect from the Server
     *
     */
    void disconnect();

    /**
     * @brief Send datas to the Server which you are connected.
     *
     * @param data Datas to send. They will be formatted accordingly to the ProtocolManager.
     * @return true If the send succeed
     * @return false If the send failed (not connected to a Server, data empty...)
     */
    bool send(const std::vector<uint8_t>& data);

    /**
     * @brief
     *
     * @tparam T The type that you want to send (a structure, ...)
     * @param packet The data that you want to send.
     * @return true If the send succeed.
     * @return false If the send failed (not connected to a Server, data empty...)
     */
    template<typename T>
    bool sendPacket(const T& packet) {
        std::vector<uint8_t> data = PacketSerializer::serialize(packet);
        return send(data);
    }

    /**
     * @brief Receive datas and put them in buffer
     * Automatically unformat them accordingly to the ProtocolManager
     *
     * @param buffer Filled by the datas received
     * @param max_size Maximum size of the buffer
     * @return int Negative value if error. 0 if Succeed
     */
    int receive(void* buffer, size_t max_size);

    /**
     * @brief Receive datas in "UDP" mode and put them in _input_buffer
     *
     * @param timeout Duration while the Client will wait datas
     * @param maxInputs Maximum input before break
     */
    void udpReceive(int timeout = 0, int maxInputs = 10);

    /**
     * @brief Receive datas in "TCP" mode and put them in _input_buffer
     * Datas in _input_buffer are not unformatted @see Client#extractPacketsFromBuffer
     *
     * @param timeout Duration while the Client will wait datas
     */
    void tcpReceive(int timeout = 0);

    /**
     * @brief Unformat packets accordingly to PorotocolManager and put only their content in a std::vector<uint8_t>
     *
     * @return std::vector<std::vector<uint8_t>> Vector of containing the content of each packet received
     */
    std::vector<std::vector<uint8_t>> extractPacketsFromBuffer();

    /**
     * @brief Set the Client non-blocking
     * It will not wait packets before doing something
     * @param enabled true if you wrant to set it non-blocking, false if you don't
     * @return true If succeed
     * @return false If failed (not connected to a Server)
     */
    bool setNonBlocking(bool enabled);

    /**
     * @brief Set the Timeout
     *
     * @param milliseconds The time to wait when waiting to receive a packet.
     * @return true If succeed
     * @return false If failed (not connected to a Server)
     */
    bool setTimeout(int milliseconds);

    /**
     * @brief Check if the Client is connected to a Server
     *
     * @return true If you are connected
     * @return false If you are not connected
     */
    bool isConnected() const { return _connected; }

    /**
     * @brief Get the _server_address object
     *
     * @return const Address&
     */
    const Address& getServerAddress() const { return _server_address; }

 private:
    NetworkSocket _socket;
    Address _server_address;
    bool _connected;
    ProtocolManager _protocol;

    std::vector<uint8_t> _input_buffer;
};

}  // namespace net
