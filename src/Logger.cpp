#include <iostream>
#include <string>
#include <cstring>
#include <ctime>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <cerrno>
#endif

#include "Network/Logger.hpp"

namespace net {

Logger::Logger(
    bool active,
    const std::string& folderPath,
    const std::string& filename) :
    _active(active), _folderPath(folderPath) {
    if (!directoryExists(_folderPath))
        if (!createDirectory(_folderPath))
            throw std::runtime_error("Failed to create log directory: " +
            _folderPath + " - " + getLastErrorMessage());

    std::time_t now = std::time(nullptr);
    char timestamp[64];
    std::strftime(timestamp,
        sizeof(timestamp),
        "%Y-%m-%d",
        std::localtime(&now));

    if (_active) {
        _filePath = _folderPath + "/" + filename + "-" + timestamp + ".log";
        _logFile.open(_filePath.c_str(), std::ios::app);

        if (!_logFile.is_open())
            throw std::runtime_error("Failed to open log file: " + _filePath);
    }
}

Logger::~Logger() {
    if (_logFile.is_open())
        _logFile.close();
}

bool Logger::createDirectory(const std::string& path) {
#ifdef _WIN32
    return _mkdir(path.c_str()) == 0 || errno == EEXIST;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

void Logger::setActive(bool active) {
    _active = active;
}

bool Logger::directoryExists(const std::string& path) {
#ifdef _WIN32
    DWORD attrib = GetFileAttributesA(path.c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES &&
            (attrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
#endif
}

std::string Logger::getLastErrorMessage() {
#ifdef _WIN32
    DWORD error = GetLastError();
    if (error == 0) return "Unknown error";

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    return message;
#else
    return std::strerror(errno);
#endif
}

bool Logger::write(const std::string& message) {
    if (!_active || !_logFile.is_open())
        return false;

    std::time_t now = std::time(nullptr);
    char timestamp[64];
    std::strftime(timestamp,
        sizeof(timestamp),
        "%Y-%m-%d %H:%M:%S",
        std::localtime(&now));

    _logFile << timestamp << " - " << message << std::endl;
    return _logFile.good();
}

}  // namespace net
