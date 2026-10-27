#pragma once
#include <string>
#include <fstream>
#include <mutex>

class Logger {
public:
    explicit Logger(const std::string& path);
    ~Logger();

    void writeLog(const std::string& message, bool isCritical);

private:
    std::string formatTimestamp() const;

    std::ofstream file_;
    std::mutex    mtx_;
};
