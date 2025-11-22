#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>

class Address {
 public:
    Address();
    Address(const std::string& ip, uint16_t port);
    Address(uint32_t ip, uint16_t port);

    static Address fromSockAddr(const sockaddr_in& addr);

    sockaddr_in toSockAddr() const;

    // Getters
    std::string getIP() const;
    uint16_t getPort() const;
    uint32_t getIPAsInt() const;

    bool operator==(const Address& other) const;
    bool operator<(const Address& other) const;

 private:
    uint32_t _ip;
    uint16_t _port;
};
