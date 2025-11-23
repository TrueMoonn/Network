#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "Address.hpp"

class NetworkSocket {
 public:
    NetworkSocket();
    ~NetworkSocket();

    bool create();
    bool bind(uint16_t port);
    void close();

    int sendTo(const void* data, size_t size, const Address& destination);
    int receiveFrom(void* buffer, size_t buffer_size, Address& sender);

    bool setNonBlocking(bool enabled);
    bool setTimeout(int milliseconds);

    bool isValid() const;
    int getSocket() const;

 private:
    int _socket;
    bool _is_valid;
};
