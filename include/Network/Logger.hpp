#pragma once

#include <string>
#include <fstream>

namespace net {

class Logger {
 public:
    Logger(bool, const std::string);
    ~Logger();

    bool write(const std::string&);

 private:
    bool _active = true;
    std::string _folderPath = "";
    std::string _filePath = "";
    std::ofstream _logFile;


    bool directoryExists(const std::string&);
    bool createDirectory(const std::string&);
    std::string getLastErrorMessage();
};

}  // namespace net
