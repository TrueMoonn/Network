#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>

class NetworkUtils {
 public:
    // Manage endianess
    static uint16_t hostToNetwork16(uint16_t value);
    static uint32_t hostToNetwork32(uint32_t value);
    static uint16_t networkToHost16(uint16_t value);
    static uint32_t networkToHost32(uint32_t value);

    static uint32_t getCurrentTimestamp();
    static bool isValidIPv4(const std::string& ip);
    static std::string getLocalIP();
};
