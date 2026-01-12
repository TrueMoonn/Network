#include "Network/NetworkSocket.hpp"

#include <cstring>
#include <cstdio>
#include <iostream>

namespace net {

NetworkSocket::NetworkSocket()
    : _socket(INVALID_SOCKET_VALUE), _is_valid(false) {
    EnsureWinsockInitialized();
}

NetworkSocket::~NetworkSocket() {
    if (_is_valid) {
        CLOSE_SOCKET(_socket);
        _socket = INVALID_SOCKET_VALUE;
        _is_valid = false;
    }
}

bool NetworkSocket::create(SocketType type) {
    EnsureWinsockInitialized();
    _type = type;
    int sock_type = (_type == SocketType::TCP) ? SOCK_STREAM : SOCK_DGRAM;

    _socket = ::socket(AF_INET, sock_type, 0);
    if (_socket == INVALID_SOCKET_VALUE) {
        PrintSocketError("socket");
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

    if (res == SOCKET_ERROR_VALUE) {
        PrintSocketError("bind failed");
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

    CLOSE_SOCKET(_socket);
    _socket = INVALID_SOCKET_VALUE;
    _is_valid = false;
}

bool NetworkSocket::setNonBlocking(bool enabled) {
    if (!_is_valid) {
        std::cerr << "Cannot set non-blocking: socket not created"
            << std::endl;
        return false;
    }
    return SetSocketNonBlocking(_socket, enabled);
}

bool NetworkSocket::setTimeout(int milliseconds) {
    if (!_is_valid) {
        std::cerr << "Cannot set timeout: socket not created"
                  << std::endl;
        return false;
    }
    return SetSocketTimeout(_socket, milliseconds);
}

bool NetworkSocket::setReuseAddr(bool enabled) {
    if (!_is_valid) {
        std::cerr << "Cannot set reuse addr: socket not created"
                  << std::endl;
        return false;
    }

    return SetSocketReuseAddr(_socket, enabled);
}

bool NetworkSocket::isValid() const {
    return _is_valid;
}

SocketHandle NetworkSocket::getSocket() const {
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
    int sent = ::sendto(_socket, static_cast<const char*>(data),
                        static_cast<int>(size), 0,
                        reinterpret_cast<struct sockaddr*>(&addr),
                        static_cast<socklen_t>(sizeof(addr)));
    if (sent == SOCKET_ERROR_VALUE) {
        PrintSocketError("sendto");
        return -1;
    }
    return sent;
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

    int recvd;
    do {
        recvd = ::recvfrom(_socket, static_cast<char*>(buffer),
                        static_cast<int>(buffer_size), 0,
                        reinterpret_cast<struct sockaddr*>(&addr), &addrlen);
    } while (recvd == SOCKET_ERROR_VALUE
            && IsInterruptError(GetLastSocketError()));

    if (recvd == SOCKET_ERROR_VALUE) {
        int error = GetLastSocketError();
        if (IsBlockingError(error))
            return -1;  // No data available in non-blocking mode
        PrintSocketError("recvfrom");
        return -1;
    }

    sender = Address::fromSockAddr(addr);
    return recvd;
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

    if (::listen(_socket, maxqueue) == SOCKET_ERROR_VALUE) {
        PrintSocketError("listen failed");
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

    SocketHandle client_fd = ::accept(_socket, (struct sockaddr*)&addr,
        &addr_len);

    if (client_fd == INVALID_SOCKET_VALUE) {
        PrintSocketError("accept failed");
        return INVALID_SOCKET_VALUE;
    }

    client_addr = Address::fromSockAddr(addr);
    return static_cast<int>(client_fd);
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

    if (::connect(_socket, (struct sockaddr*)&addr,
        sizeof(addr)) == SOCKET_ERROR_VALUE) {
        PrintSocketError("connect failed");
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

    int sent = ::send(_socket, static_cast<const char*>(data),
                    static_cast<int>(size), 0);

    if (sent == SOCKET_ERROR_VALUE) {
        PrintSocketError("send");
        return -1;
    }

    return sent;
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

    int recvd;
    do {
        recvd = ::recv(_socket, static_cast<char*>(buffer),
                    static_cast<int>(buffer_size), 0);
    } while (recvd == SOCKET_ERROR_VALUE
        && IsInterruptError(GetLastSocketError()));

    if (recvd == SOCKET_ERROR_VALUE) {
        int error = GetLastSocketError();
        if (IsBlockingError(error)) {
            return 0;
        }
        PrintSocketError("recv");
        return -1;
    }

    return recvd;
}

}  // namespace net
