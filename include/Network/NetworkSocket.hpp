#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "Network/NetworkPlatform.hpp"
#include "Network/Address.hpp"

namespace net {

enum class SocketType {
    UDP,
    TCP
};

class NetworkSocket {
 public:
    /**
     * @brief Constructs a NetworkSocket object
     * @param type The type of socket (UDP or TCP) Default is UDP
     */
    explicit NetworkSocket(SocketType type = SocketType::UDP);

    /**
     * @brief Destructor for NetworkSocket
     */
    ~NetworkSocket();

    /**
     * @brief Creates a socket of the specified type
     * @param type The type of socket to create (UDP or TCP). Default is UDP
     * @return true if the socket was created successfully, false otherwise
     */
    bool create(SocketType type = SocketType::UDP);

    /**
     * @brief Binds the socket to the specified port
     * @param port The port number to bind to
     * @return true if binding was successful, false otherwise
     */
    bool bind(uint16_t port);

    /**
     * @brief Closes the socket
     */
    void close();

    /**
     * @brief Sets the socket to non-blocking or blocking mode
     * @param enabled True to enable non-blocking mode, false for blocking
     * @return true if the operation was successful, false otherwise
     */
    bool setNonBlocking(bool enabled);

    /**
     * @brief Sets a timeout for socket operations
     * @param milliseconds Timeout duration in milliseconds
     * @return true if the operation was successful, false otherwise
     */
    bool setTimeout(int milliseconds);

    /**
     * @brief Enables or disables address reuse for the socket
     * @param enabled True to enable address reuse, false to disable
     * @return true if the operation was successful, false otherwise
     */
    bool setReuseAddr(bool enabled);

    /**
     * @brief Checks if the socket is valid
     * @return true if the socket is valid, false otherwise
     */
    bool isValid() const;

    /**
     * @brief Gets the underlying socket handle
     * @return The socket handle
     */
    SocketHandle getSocket() const;

    /**
     * @brief Gets the type of the socket (UDP or TCP)
     * @return The socket type
     */
    SocketType getType() const { return _type; }

    /**
     * @brief Sends data to a specific destination address (UDP)
     * @param data Pointer to the data to send
     * @param size Size of the data in bytes
     * @param destination Destination address
     * @return Number of bytes sent, or -1 on failure
     */
    int sendTo(const void* data, size_t size, const Address& destination);

    /**
     * @brief Receives data from a sender (UDP)
     * @param buffer Pointer to the buffer to store received data
     * @param buffer_size Size of the buffer in bytes
     * @param sender Address of the sender (output)
     * @return Number of bytes received, or -1 on failure
     */
    int receiveFrom(void* buffer, size_t buffer_size, Address& sender);

    /**
     * @brief Listens for incoming TCP connections
     * @param maxqueue Maximum number of pending connections. Default is 10
     * @return true if the socket is now listening, false otherwise
     */
    bool listen(int maxqueue = 10);

    /**
     * @brief Accepts an incoming TCP connection
     * @param client_addr Address of the connecting client (output)
     * @return Socket handle for the accepted connection, or -1 on failure
     */
    int accept(Address& client_addr);

    /**
     * @brief Connects to a TCP server
     * @param server_addr Address of the server to connect to
     * @return true if the connection was successful, false otherwise
     */
    bool connect(const Address& server_addr);

    /**
     * @brief Sends data over a connected TCP socket
     * @param data Pointer to the data to send
     * @param size Size of the data in bytes
     * @return Number of bytes sent, or -1 on failure
     */
    int send(const void* data, size_t size);

    /**
     * @brief Receives data from a connected TCP socket
     * @param buffer Pointer to the buffer to store received data
     * @param buffer_size Size of the buffer in bytes
     * @return Number of bytes received, or -1 on failure
     */
    int recv(void* buffer, size_t buffer_size);

    class InvalidSocketType : public std::exception {
     public:
        explicit InvalidSocketType(std::string msg = "Wrong Socket Type")
            : _msg(msg) {}

        const char* what() const noexcept override {
            return _msg.c_str();
        }
     private:
        std::string _msg;
    };

    class SocketCreationError : public std::exception {
     public:
        const char* what() const noexcept override {
            return "Failed to create socket";
        }
    };

    class SocketNotCreated : public std::exception {
     public:
        const char* what() const noexcept override {
            return "Socket not created";
        }
    };

    class DataSendFailed : public std::exception {
     public:
        const char* what() const noexcept override {
            return "Failed to send data to given dest";
        }
    };

    class AcceptFailed : public std::exception {
     public:
        const char* what() const noexcept override {
            return "Failed to accept client connection";
        }
    };

    class ListenFailed : public std::exception {
     public:
        const char* what() const noexcept override {
            return "Failed to listen to socket";
        }
    };

    class BindFailed : public std::exception {
     public:
        const char* what() const noexcept override {
            return "Failed to bind socket with given port";
        }
    };

 private:
    SocketHandle _socket;
    bool _is_valid;
    SocketType _type;
};

}  // namespace net
