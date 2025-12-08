#include <chrono>
#include <string>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <unistd.h>
#endif

#include "Network/NetworkUtils.hpp"
#include "Network/NetworkPlatform.hpp"

uint16_t NetworkUtils::hostToNetwork16(uint16_t value) {
    return htons(value);
}

uint32_t NetworkUtils::hostToNetwork32(uint32_t value) {
    return htonl(value);
}

uint16_t NetworkUtils::networkToHost16(uint16_t value) {
    return ntohs(value);
}

uint32_t NetworkUtils::networkToHost32(uint32_t value) {
    return ntohl(value);
}

uint32_t NetworkUtils::getCurrentTimestamp() {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    auto millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    return static_cast<uint32_t>(millis.count());
}

bool NetworkUtils::isValidIPv4(const std::string& ip) {
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr));
    return result == 1;
}

std::string NetworkUtils::getLocalIP() {
#ifdef _WIN32
    // Resolve the first non-loopback IPv4 bound to the host name
    EnsureWinsockInitialized();

    char hostname[256] = {0};
    if (gethostname(hostname, static_cast<int>(sizeof(hostname)))
        == SOCKET_ERROR) {
        return "127.0.0.1";
    }

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* info = nullptr;
    if (getaddrinfo(hostname, nullptr, &hints, &info) != 0) {
        return "127.0.0.1";
    }

    std::string result = "127.0.0.1";
    for (addrinfo* p = info; p != nullptr; p = p->ai_next) {
        if (p->ai_family != AF_INET || p->ai_addr == nullptr) {
            continue;
        }
        auto* addr = reinterpret_cast<sockaddr_in*>(p->ai_addr);
        char ip[INET_ADDRSTRLEN] = {0};
        if (inet_ntop(AF_INET, &(addr->sin_addr), ip, INET_ADDRSTRLEN)) {
            std::string ip_str(ip);
            if (ip_str != "127.0.0.1") {
                result = ip_str;
                break;
            }
            result = ip_str;
        }
    }

    freeaddrinfo(info);
    return result;
#else
    struct ifaddrs* ifaddr;
    std::string result = "127.0.0.1";

    if (getifaddrs(&ifaddr) == -1) {
        return result;
    }
    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* addr = (struct sockaddr_in*)ifa->ifa_addr;
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(addr->sin_addr), ip, INET_ADDRSTRLEN);
            std::string ip_str(ip);
            if (ip_str != "127.0.0.1") {
                result = ip_str;
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return result;
#endif
}
