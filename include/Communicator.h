#pragma once
#include <string>
#include <cstdint>
#include <tuple>
#include <map>
#include <vector>
#include "Base.h"
#include "Logger.h"
#include "Calc.h"

class Communicator {
public:
    Communicator(uint16_t port, Base& base, Logger& logger, Calc& calc);
    ~Communicator();

    int  connection(int port,
                    const std::map<std::string,std::string>& database,
                    Logger& logger);

    void run();

    std::string SHA256(const std::string& inputString) const;

    std::tuple<std::string, std::string, std::string>
        parseAuthMessage(const std::string& msg) const;

    bool verifyHash(const std::string& login,
                    const std::string& salt,
                    const std::string& hash) const;

private:
    void serveClient(int clientFd, const std::string& peer);
    bool authenticate(int fd, std::string& outLogin);

    void recvExact(int fd, void* buf, size_t n);
    void sendExact(int fd, const void* buf, size_t n);
    void sendLine(int fd, const std::string& s);

    int      listenFd_ = -1;
    uint16_t port_;
    Base&    base_;
    Logger&  logger_;
    Calc&    calc_;
    
    std::string buffer_;
};
