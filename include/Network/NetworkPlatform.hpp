#pragma once

#include <iostream>
#include <stdexcept>
#include <cstdio>

// Platform detection and socket abstraction for cross-platform compatibility

#ifdef _WIN32
    // Windows
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    
    // Link with Winsock library
    #pragma comment(lib, "ws2_32.lib")
    
    // Type definitions for Windows
    typedef SOCKET SocketHandle;
    typedef int socklen_t;
    
    // Constants
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
    #define SOCKET_ERROR_VALUE SOCKET_ERROR
    #define CLOSE_SOCKET(s) closesocket(s)
    
    // Error codes mapping
    #define WOULD_BLOCK WSAEWOULDBLOCK
    #define IN_PROGRESS WSAEINPROGRESS
    #define INTERRUPTED WSAEINTR
    
    // Get last error
    inline int GetLastSocketError() {
        return WSAGetLastError();
    }
    
    // Initialize Winsock
    class WinsockInitializer {
     public:
        WinsockInitializer() {
            WSADATA wsaData;
            int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (result != 0) {
                throw std::runtime_error("WSAStartup failed");
            }
        }
        
        ~WinsockInitializer() {
            WSACleanup();
        }
    };
    
    // Ensure Winsock is initialized
    inline void EnsureWinsockInitialized() {
        static WinsockInitializer initializer;
    }
    
#else
    // Unix-like systems (Linux, macOS, etc.)
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    
    // Type definitions for Unix
    typedef int SocketHandle;
    
    // Constants
    #define INVALID_SOCKET_VALUE -1
    #define SOCKET_ERROR_VALUE -1
    #define CLOSE_SOCKET(s) ::close(s)
    
    // Error codes mapping
    #define WOULD_BLOCK EWOULDBLOCK
    #define IN_PROGRESS EINPROGRESS
    #define INTERRUPTED EINTR
    
    // Get last error
    inline int GetLastSocketError() {
        return errno;
    }
    
    // No initialization needed for Unix
    inline void EnsureWinsockInitialized() {
        // No-op on Unix
    }
    
#endif

// Cross-platform functions

// Set socket to non-blocking mode
inline bool SetSocketNonBlocking(SocketHandle socket, bool enabled) {
#ifdef _WIN32
    u_long mode = enabled ? 1 : 0;
    return ioctlsocket(socket, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }
    if (enabled) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    return fcntl(socket, F_SETFL, flags) != -1;
#endif
}

// Set socket timeout
inline bool SetSocketTimeout(SocketHandle socket, int milliseconds) {
#ifdef _WIN32
    DWORD timeout = milliseconds;
    return setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,
                     (const char*)&timeout, sizeof(timeout)) == 0;
#else
    struct timeval tv;
    if (milliseconds > 0) {
        tv.tv_sec = milliseconds / 1000;
        tv.tv_usec = (milliseconds % 1000) * 1000;
    } else {
        tv.tv_sec = 0;
        tv.tv_usec = 0;
    }
    return setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != -1;
#endif
}

// Set socket reuse address
inline bool SetSocketReuseAddr(SocketHandle socket, bool enabled) {
#ifdef _WIN32
    BOOL opt = enabled ? TRUE : FALSE;
    return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR,
                     (const char*)&opt, sizeof(opt)) == 0;
#else
    int opt = enabled ? 1 : 0;
    return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0;
#endif
}

// Check if error is a blocking error
inline bool IsBlockingError(int error) {
    return error == WOULD_BLOCK || error == IN_PROGRESS;
}

// Check if error is an interrupt
inline bool IsInterruptError(int error) {
    return error == INTERRUPTED;
}

// Print socket error
inline void PrintSocketError(const char* msg) {
#ifdef _WIN32
    char buffer[256];
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        GetLastSocketError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buffer,
        sizeof(buffer),
        nullptr);
    std::cerr << msg << ": " << buffer << std::endl;
#else
    perror(msg);
#endif
}

// Poll abstraction - Windows uses WSAPoll, Unix uses poll
#ifdef _WIN32
    // Windows has WSAPoll which is compatible with poll
    #define POLLFD WSAPOLLFD
    #define POLL_FD_TYPE SOCKET
    #define POLL_IN POLLRDNORM
    #define POLL_OUT POLLWRNORM
    
    inline int PollSockets(WSAPOLLFD* fds, ULONG nfds, int timeout) {
        return WSAPoll(fds, nfds, timeout);
    }
#else
    // Unix systems have poll natively
    #include <poll.h>
    
    #define POLLFD pollfd
    #define POLL_FD_TYPE int
    #define POLL_IN POLLIN
    #define POLL_OUT POLLOUT
    
    inline int PollSockets(pollfd* fds, nfds_t nfds, int timeout) {
        return poll(fds, nfds, timeout);
    }
#endif
