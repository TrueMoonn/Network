#include "Network/Address.hpp"

#include <arpa/inet.h>
#include <iostream>
#include <string>

Address::Address() : _ip(0), _port(0) {}

Address::Address(const std::string& ip, uint16_t port) {
    struct in_addr a;
    if (inet_pton(AF_INET, ip.c_str(), &a) != 1) {
        std::cerr << "Invalid IP address: " << ip << std::endl;
        _ip = 0;
    } else {
        _ip = a.s_addr;
    }
    _port = port;
}

Address::Address(uint32_t ip, uint16_t port) {
    _ip = htonl(ip);
    _port = port;
}

Address Address::fromSockAddr(const sockaddr_in& addr) {
    Address a;
    a._ip = addr.sin_addr.s_addr;
    a._port = ntohs(addr.sin_port);
    return a;
}

sockaddr_in Address::toSockAddr() const {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = _ip;
    return addr;
}

bool Address::operator==(const Address& other) const {
    return _ip == other._ip && _port == other._port;
}

bool Address::operator<(const Address& other) const {
    uint32_t a = ntohl(_ip);
    uint32_t b = ntohl(other._ip);
    if (a != b) return a < b;
    return _port < other._port;
}

std::string Address::getIP() const {
    char buf[INET_ADDRSTRLEN];
    struct in_addr addr;
    addr.s_addr = _ip;

    if (inet_ntop(AF_INET, &addr, buf, static_cast<socklen_t>(sizeof(buf)))
        == nullptr) {
        return std::string();
    }

    return std::string(buf);
}

uint16_t Address::getPort() const {
    return _port;
}

uint32_t Address::getIPAsInt() const {
    return _ip;
}



