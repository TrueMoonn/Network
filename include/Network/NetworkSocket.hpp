#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "Network/Address.hpp"

enum class SocketType {
    UDP,
    TCP
};

class NetworkSocket {
 public:
    explicit NetworkSocket(SocketType = SocketType::UDP);
    ~NetworkSocket();

    bool create(SocketType type = SocketType::UDP);
    bool bind(uint16_t port);
    void close();

    bool setNonBlocking(bool enabled);
    bool setTimeout(int milliseconds);
    bool setReuseAddr(bool enabled);

    bool isValid() const;
    int getSocket() const;
    SocketType getType() const { return _type; }

    // UDP
    int sendTo(const void* data, size_t size, const Address& destination);
    int receiveFrom(void* buffer, size_t buffer_size, Address& sender);

    // TCP
    bool listen(int maxqueue = 10);
    int accept(Address& client_addr);
    bool connect(const Address& server_addr);
    int send(const void* data, size_t size);
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
    int _socket;
    bool _is_valid;
    SocketType _type;
};
