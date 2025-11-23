#include "Network/NetworkSocket.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <cerrno>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <climits>

NetworkSocket::NetworkSocket() : _socket(-1), _is_valid(false) {}

NetworkSocket::~NetworkSocket() {
    if (_is_valid) {
        ::close(_socket);
        _socket = -1;
        _is_valid = false;
    }
}

bool NetworkSocket::create() {
    _socket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (_socket < 0) {
        perror("socket");
        _is_valid = false;
        return false;
    }
    _is_valid = true;
    return true;
}

bool NetworkSocket::bind(uint16_t port) {
    if (!_is_valid) {
        std::cerr << "Cannot bind: socket not created." << std::endl;
        return false;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    int res = ::bind(_socket, (struct sockaddr *)&address, sizeof(sockaddr_in));

    if (res < 0) {
        perror("bind failed");
        return false;
    }
    return true;
}

void NetworkSocket::close() {
    if (!_is_valid) {
        std::cerr << "Cannot close: socket not created." << std::endl;
        return;
    }

    ::close(_socket);
    _socket = -1;
    _is_valid = false;
}

int NetworkSocket::sendTo(const void* data, size_t size,
        const Address& destination) {
    if (!_is_valid) {
        std::cerr << "Cannot send: socket not created." << std::endl;
        return -1;
    }

    sockaddr_in addr = destination.toSockAddr();
    ssize_t sent = ::sendto(_socket, data, size, 0,
                            reinterpret_cast<struct sockaddr*>(&addr),
                            static_cast<socklen_t>(sizeof(addr)));
    if (sent < 0) {
        perror("sendto");
        return -1;
    }
    return static_cast<int>(sent);
}

int NetworkSocket::receiveFrom(void* buffer, size_t buffer_size,
        Address& sender) {
    if (!_is_valid) {
        std::cerr << "Cannot receive: socket not created." << std::endl;
        return -1;
    }

    if (buffer == nullptr) {
        std::cerr << "receiveFrom: buffer is null" << std::endl;
        return -1;
    }

    if (buffer_size == 0) {
        std::cerr << "receiveFrom: buffer_size is 0" << std::endl;
        return -1;
    }

    sockaddr_in addr{};
    socklen_t addrlen = sizeof(addr);

    // handle EINTR (interrupt by signal)
    ssize_t recvd;
    do {
        recvd = ::recvfrom(_socket, buffer, buffer_size, 0,
                           reinterpret_cast<struct sockaddr*>(&addr), &addrlen);
    } while (recvd < 0 && errno == EINTR);

    if (recvd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // non blocking socket or timeout expired
            return 0;
        }
        perror("recvfrom");
        return -1;
    }
    if (recvd > INT_MAX) {
        std::cerr << "receiveFrom: received " << recvd
            << " bytes (capping to INT_MAX)" << std::endl;
        recvd = INT_MAX;
    }

    sender = Address::fromSockAddr(addr);
    return static_cast<int>(recvd);
}

bool NetworkSocket::setNonBlocking(bool enabled) {
    if (!_is_valid) {
        std::cerr << "Cannot set non-blocking: socket not created."
            << std::endl;
        return false;
    }
    int flags = fcntl(_socket, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL)");
        return false;
    }
    if (enabled) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    if (fcntl(_socket, F_SETFL, flags) == -1) {
        perror("fcntl(F_SETFL)");
        return false;
    }
    return true;
}

bool NetworkSocket::setTimeout(int milliseconds) {
    if (!_is_valid) {
        std::cerr << "Cannot set timeout: socket not created." << std::endl;
        return false;
    }
    struct timeval tv{};
    if (milliseconds > 0) {
        tv.tv_sec = milliseconds / 1000;
        tv.tv_usec = (milliseconds % 1000) * 1000;
    } else {
        tv.tv_sec = 0;
        tv.tv_usec = 0;
    }
    if (setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        perror("setsockopt(SO_RCVTIMEO)");
        return false;
    }
    return true;
}

bool NetworkSocket::isValid() const {
    return _is_valid;
}

int NetworkSocket::getSocket() const {
    return _socket;
}
