#pragma once

#include <cstdint>
#include <string>

#include "Network/NetworkPlatform.hpp"

namespace net {

/**
 * @brief
 *
 * Address (ip and port -> ex: 127.0.0.1:8080)
 *
 * Identifier to communicate between Client and server
 */
class Address {
 public:
    /**
     * @brief Construct a new Address object
     *
     */
    Address();

    /**
     * @brief Construct a new Address object
     *
     * @param ip ex: 127.0.0.1 given as string
     * @param port ex: 8080
     */
    Address(const std::string& ip, uint16_t port);

    /**
     * @brief Construct a new Address object
     *
     * @param ip ex: 127.0.0.1 given as uint32_t
     * @param port ex: 8080
     */
    Address(uint32_t ip, uint16_t port);

    /**
     * @brief Create an Address from a sockaddr_in struct
     *
     * @param addr structure containing internet socket informations
     * @return Address created from the addr parameter
     */
    static Address fromSockAddr(const sockaddr_in& addr);

    /**
     * @brief Create a sockaddr_in struct from this class
     *
     * @return sockaddr_in struct created from the class
     */
    sockaddr_in toSockAddr() const;

    /**
     * @brief Get the IP
     *
     * @return std::string
     */
    std::string getIP() const;

    /**
     * @brief Get the port
     *
     * @return uint16_t
     */
    uint16_t getPort() const;

    /**
     * @brief Get the IP
     *
     * @return uint32_t
     */
    uint32_t getIPAsInt() const;

    bool operator==(const Address& other) const;
    bool operator<(const Address& other) const;

 private:
    uint32_t _ip;
    uint16_t _port;
};

}  // namespace net

// needed to store Address in unordered_map
namespace std {
template<>
struct hash<net::Address> {
        size_t operator()(const net::Address& addr) const {
            size_t h1 = hash<uint32_t>()(addr.getIPAsInt());
            size_t h2 = hash<uint16_t>()(addr.getPort());
            return h1 ^ (h2 << 1);
        }
};
}  // namespace std
