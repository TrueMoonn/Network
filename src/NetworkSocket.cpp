#include "Network/NetworkSocket.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <cerrno>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <climits>

NetworkSocket::NetworkSocket()
    : _socket(-1), _is_valid(false), _type(SocketType::UDP) {}

NetworkSocket::~NetworkSocket() {
    if (_is_valid) {
        ::close(_socket);
        _socket = -1;
        _is_valid = false;
    }
}

bool NetworkSocket::create(SocketType type) {
    _type = type;
    int sock_type = (_type == SocketType::TCP) ? SOCK_STREAM : SOCK_DGRAM;

    _socket = ::socket(AF_INET, sock_type, 0);
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
        std::cerr << "Cannot bind: socket not created"
                  << std::endl;
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
        std::cerr << "Cannot close: socket not created"
                  << std::endl;
        return;
    }

    ::close(_socket);
    _socket = -1;
    _is_valid = false;
}

bool NetworkSocket::setNonBlocking(bool enabled) {
    if (!_is_valid) {
        std::cerr << "Cannot set non-blocking: socket not created"
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
        std::cerr << "Cannot set timeout: socket not created"
                  << std::endl;
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

bool NetworkSocket::setReuseAddr(bool enabled) {
    if (!_is_valid) {
        std::cerr << "Cannot set reuse addr: socket not created"
                  << std::endl;
        return false;
    }

    int opt = enabled ? 1 : 0;
    if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
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

// UDP
int NetworkSocket::sendTo(
    const void* data, size_t size, const Address& destination) {
    if (!_is_valid) {
        std::cerr << "Cannot send: socket not created"
                  << std::endl;
        return -1;
    }

    if (_type != SocketType::UDP) {
        std::cerr << "Cannot use sendTo: Use send() for TCP mode"
                  << std::endl;
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

int NetworkSocket::receiveFrom(
    void* buffer, size_t buffer_size, Address& sender) {
    if (!_is_valid) {
        std::cerr << "Cannot receive: socket not created"
                  << std::endl;
        return -1;
    }

    if (_type != SocketType::UDP) {
        std::cerr << "Cannot use receiveFrom: Use recv() for TCP mode"
                  << std::endl;
        return -1;
    }

    if (buffer == nullptr) {
        std::cerr << "receiveFrom: buffer is null"
                  << std::endl;
        return -1;
    }

    if (buffer_size == 0) {
        std::cerr << "receiveFrom: buffer_size is 0"
                  << std::endl;
        return -1;
    }

    sockaddr_in addr{};
    socklen_t addrlen = sizeof(addr);

    ssize_t recvd;
    do {
        recvd = ::recvfrom(_socket, buffer, buffer_size, 0,
                           reinterpret_cast<struct sockaddr*>(&addr), &addrlen);
    } while (recvd < 0 && errno == EINTR);

    if (recvd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
        perror("recvfrom");
        return -1;
    }
    if (recvd > INT_MAX) {
        std::cerr << "receiveFrom: received "
                  << recvd
                  << " bytes (capping to INT_MAX)"
                  << std::endl;
        recvd = INT_MAX;
    }

    sender = Address::fromSockAddr(addr);
    return static_cast<int>(recvd);
}

// TCP
bool NetworkSocket::listen(int maxqueue) {
    if (!_is_valid) {
        std::cerr << "Cannot listen: socket not created"
                  << std::endl;
        return false;
    }

    if (_type != SocketType::TCP) {
        std::cerr << "Cannot listen: socket is not TCP"
                  << std::endl;
        return false;
    }

    if (::listen(_socket, maxqueue) < 0) {
        perror("listen failed");
        return false;
    }

    return true;
}

int NetworkSocket::accept(Address& client_addr) {
    if (!_is_valid) {
        std::cerr << "Cannot accept: socket not created"
                  << std::endl;
        return -1;
    }

    if (_type != SocketType::TCP) {
        std::cerr << "Cannot accept: socket is not TCP"
                  << std::endl;
        return -1;
    }

    sockaddr_in addr{};
    socklen_t addr_len = sizeof(addr);

    int client_fd = ::accept(_socket, (struct sockaddr*)&addr, &addr_len);

    if (client_fd < 0) {
        perror("accept failed");
        return -1;
    }

    client_addr = Address::fromSockAddr(addr);
    return client_fd;
}

bool NetworkSocket::connect(const Address& server_addr) {
    if (!_is_valid) {
        std::cerr << "Cannot connect: socket not created"
                  << std::endl;
        return false;
    }

    if (_type != SocketType::TCP) {
        std::cerr << "Cannot connect: socket is not TCP"
                  << std::endl;
        return false;
    }

    sockaddr_in addr = server_addr.toSockAddr();

    if (::connect(_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect failed");
        return false;
    }

    return true;
}

int NetworkSocket::send(const void* data, size_t size) {
    if (!_is_valid) {
        std::cerr << "Cannot send: socket not created"
                  << std::endl;
        return -1;
    }

    if (_type != SocketType::TCP) {
        std::cerr << "Cannot use send: Use sendTo() for UDP"
                  << std::endl;
        return -1;
    }

    if (data == nullptr || size == 0) {
        std::cerr << "Invalid data or size"
                  << std::endl;
        return -1;
    }

    ssize_t sent = ::send(_socket, data, size, 0);

    if (sent < 0) {
        perror("send");
        return -1;
    }

    return static_cast<int>(sent);
}

int NetworkSocket::recv(void* buffer, size_t buffer_size) {
    if (!_is_valid) {
        std::cerr << "Cannot receive: socket not created"
                  << std::endl;
        return -1;
    }

    if (_type != SocketType::TCP) {
        std::cerr << "Cannot use recv: Use receiveFrom() for UDP"
                  << std::endl;
        return -1;
    }

    if (buffer == nullptr || buffer_size == 0) {
        std::cerr << "Invalid buffer or size"
                  << std::endl;
        return -1;
    }

    ssize_t recvd;
    do {
        recvd = ::recv(_socket, buffer, buffer_size, 0);
    } while (recvd < 0 && errno == EINTR);

    if (recvd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        perror("recv");
        return -1;
    }

    if (recvd > INT_MAX) {
        std::cerr << "recv: received "
                  << recvd
                  << " bytes (capping to INT_MAX)"
                  << std::endl;
        recvd = INT_MAX;
    }

    return static_cast<int>(recvd);
}
