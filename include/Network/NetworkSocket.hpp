#pragma once
#include "Network/Address.hpp"
#include <cstddef>
#include <cstdint>

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

private:
    int _socket;
    bool _is_valid;
    SocketType _type;
};
