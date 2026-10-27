#include "Logger.h"
#include "Errors.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

Logger::Logger(const std::string& path) {
    file_.open(path, std::ios::app);
    if (!file_.is_open())
        throw InterfaceError("Cannot open log file: " + path);
}

Logger::~Logger() {
    if (file_.is_open()) file_.close();
}

std::string Logger::formatTimestamp() const {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void Logger::writeLog(const std::string& message, bool isCritical) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!file_.is_open()) return;
    file_ << formatTimestamp() << " ["
          << (isCritical ? "CRIT" : "INFO") << "] "
          << message << "\n";
    file_.flush();
}
