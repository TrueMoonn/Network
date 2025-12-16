#pragma once

#include <cstdint>
#include <string>

#include "Network/NetworkPlatform.hpp"

namespace net {

class NetworkUtils {
 public:
    /**
     * @brief Converts a 16-bit value from host to network byte order.
     * @param value The 16-bit value in host byte order.
     * @return The value in network byte order.
     */
    static uint16_t hostToNetwork16(uint16_t value);

    /**
     * @brief Converts a 32-bit value from host to network byte order.
     * @param value The 32-bit value in host byte order.
     * @return The value in network byte order.
     */
    static uint32_t hostToNetwork32(uint32_t value);

    /**
     * @brief Converts a 16-bit value from network to host byte order.
     * @param value The 16-bit value in network byte order.
     * @return The value in host byte order.
     */
    static uint16_t networkToHost16(uint16_t value);

    /**
     * @brief Converts a 32-bit value from network to host byte order.
     * @param value The 32-bit value in network byte order.
     * @return The value in host byte order.
     */
    static uint32_t networkToHost32(uint32_t value);

    /**
     * @brief Gets the current timestamp in seconds since the epoch.
     * @return The current timestamp as a 32-bit unsigned integer.
     */
    static uint32_t getCurrentTimestamp();

    /**
     * @brief Checks if the given string is a valid IPv4 address.
     * @param ip The string to check.
     * @return true if the string is a valid IPv4 address, false otherwise.
     */
    static bool isValidIPv4(const std::string& ip);

    /**
     * @brief Gets the local machine's IPv4 address as a string.
     * @return The local IP address as a string.
     */
    static std::string getLocalIP();
};

}  // namespace net
