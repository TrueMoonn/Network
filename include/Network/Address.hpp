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

// needed to store Address in unordered_map
namespace std {
template<>
struct hash<Address> {
        size_t operator()(const Address& addr) const {
            size_t h1 = hash<uint32_t>()(addr.getIPAsInt());
            size_t h2 = hash<uint16_t>()(addr.getPort());
            return h1 ^ (h2 << 1);
        }
};
}  // namespace std
