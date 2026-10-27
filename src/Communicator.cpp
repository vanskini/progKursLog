#include "Communicator.h"
#include "Errors.h"

#include <cryptopp/hex.h>
#include <cryptopp/sha.h>
#include <cryptopp/cryptlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <memory>
#include <vector>

namespace CPP = CryptoPP;

Communicator::Communicator(uint16_t port, Base& base, Logger& logger, Calc& calc)
    : port_(port), base_(base), logger_(logger), calc_(calc) {}

Communicator::~Communicator() {
    if (listenFd_ >= 0) ::close(listenFd_);
}

int Communicator::connection(int port,
                             const std::map<std::string,std::string>& /*database*/,
                             Logger& logger) {
    listenFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0)
        throw NetworkError(std::string("socket: ") + std::strerror(errno));

    int opt = 1;
    ::setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(static_cast<uint16_t>(port));
    if (::bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw NetworkError(std::string("bind: ") + std::strerror(errno));

    if (::listen(listenFd_, 10) < 0)
        throw NetworkError(std::string("listen: ") + std::strerror(errno));

    logger.writeLog("Server listening on port " + std::to_string(port), false);
    return listenFd_;
}

void Communicator::run() {
    while (true) {
        sockaddr_in cli{};
        socklen_t   len = sizeof(cli);
        int fd = ::accept(listenFd_, reinterpret_cast<sockaddr*>(&cli), &len);
        if (fd < 0) {
            if (errno == EINTR) continue;
            logger_.writeLog(std::string("accept: ") + std::strerror(errno), false);
            continue;
        }
        char ipbuf[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &cli.sin_addr, ipbuf, sizeof(ipbuf));
        std::string peer = std::string(ipbuf) + ":" +
                           std::to_string(ntohs(cli.sin_port));
        logger_.writeLog("Accepted " + peer, false);

        buffer_.clear();

        try {
            serveClient(fd, peer);
        } catch (const std::exception& e) {
            logger_.writeLog("Client " + peer + " error: " + e.what(), false);
        }
        ::close(fd);
        logger_.writeLog("Closed " + peer, false);
    }
}

void Communicator::recvExact(int fd, void* buf, size_t n) {
    size_t got = 0;
    auto*  p   = static_cast<char*>(buf);

    if (!buffer_.empty()) {
        size_t take = std::min(n, buffer_.size());
        std::memcpy(p, buffer_.data(), take);
        buffer_.erase(0, take);
        got += take;
    }

    while (got < n) {
        ssize_t r = ::recv(fd, p + got, n - got, 0);
        if (r == 0) throw NetworkError("Connection closed by peer");
        if (r < 0)  throw NetworkError(std::string("recv: ") + std::strerror(errno));
        got += static_cast<size_t>(r);
    }
}

void Communicator::sendExact(int fd, const void* buf, size_t n) {
    size_t sent = 0;
    auto*  p    = static_cast<const char*>(buf);
    while (sent < n) {
        ssize_t r = ::send(fd, p + sent, n - sent, 0);
        if (r < 0) throw NetworkError(std::string("send: ") + std::strerror(errno));
        sent += static_cast<size_t>(r);
    }
}

void Communicator::sendLine(int fd, const std::string& s) {
    std::string out = s + "\n";
    sendExact(fd, out.data(), out.size());
}

std::string Communicator::SHA256(const std::string& inputString) const {
    CPP::SHA256 sha;
    std::string hex;
    CPP::StringSource(inputString, true,
        new CPP::HashFilter(sha,
            new CPP::HexEncoder(new CPP::StringSink(hex))));
    return hex;
}

std::tuple<std::string, std::string, std::string>
Communicator::parseAuthMessage(const std::string& msg) const {
    constexpr size_t SALT_LEN = 16;
    constexpr size_t HASH_LEN = 64;
    constexpr size_t FIXED    = SALT_LEN + HASH_LEN;

    if (msg.size() < FIXED)
        throw CryptoError("Auth message too short");

    // Отрезаем строго с конца
    std::string hash  = msg.substr(msg.size() - HASH_LEN);
    std::string salt  = msg.substr(msg.size() - FIXED, SALT_LEN);
    std::string login = msg.substr(0, msg.size() - FIXED);
    
    return std::make_tuple(login, salt, hash);
}

bool Communicator::verifyHash(const std::string& login,
                              const std::string& salt,
                              const std::string& hash) const {
    std::string pass = base_.getPassword(login);
    std::string to_hash = salt + pass;
    
    CPP::SHA256 sha;
    std::string correct_hex;
    CPP::StringSource(to_hash, true,
        new CPP::HashFilter(sha,
            new CPP::HexEncoder(new CPP::StringSink(correct_hex))));

    if (correct_hex.size() != hash.size()) {
        return false;
    }

    return CPP::VerifyBufsEqual(
        reinterpret_cast<const CPP::byte*>(correct_hex.data()),
        reinterpret_cast<const CPP::byte*>(hash.data()),
        correct_hex.size()
    );
}

bool Communicator::authenticate(int fd, std::string& outLogin) {
    char buf[1024];
    std::string data;
    
    ssize_t peeked = ::recv(fd, buf, sizeof(buf), MSG_PEEK);
    if (peeked <= 0) throw NetworkError("Connection closed during auth");
    
    size_t msg_len = peeked;
    
    if (msg_len > 84) {
        uint32_t possible_n = 0;
        std::memcpy(&possible_n, buf + msg_len - 4, 4);
        if (possible_n > 0 && possible_n < 10000) {
            msg_len -= 4; // Отрезаем N
            buffer_ = std::string(buf + msg_len, 4);
        }
    }
    
    ssize_t r = ::recv(fd, buf, msg_len, 0);
    if (r <= 0) throw NetworkError("Connection closed during auth");
    data.assign(buf, r);
    
    while (data.size() < 80) {
        ssize_t r2 = ::recv(fd, buf, 1024, 0);
        if (r2 <= 0) throw NetworkError("Connection closed during auth");
        data.append(buf, r2);
    }

    std::string login, salt, hash;
    try {
        std::tie(login, salt, hash) = parseAuthMessage(data);
    } catch (const CryptoError& e) {
        sendLine(fd, "ERR");
        logger_.writeLog(std::string("Auth failed: ") + e.what(), false);
        return false;
    }

    if (!base_.findLogin(login)) {
        sendLine(fd, "ERR");
        logger_.writeLog("Auth failed: unknown login '" + login + "'", false);
        return false;
    }

    if (!verifyHash(login, salt, hash)) {
        sendLine(fd, "ERR");
        logger_.writeLog("Auth failed: bad hash for '" + login + "'", false);
        return false;
    }

    sendLine(fd, "OK");
    outLogin = login;
    logger_.writeLog("Auth OK for '" + login + "'", false);
    return true;
}

void Communicator::serveClient(int clientFd, const std::string& peer) {
    std::string login;
    if (!authenticate(clientFd, login)) {
        logger_.writeLog("Auth rejected for " + peer, false);
        return;
    }

    uint32_t N = 0;
    recvExact(clientFd, &N, sizeof(N));

    for (uint32_t i = 0; i < N; ++i) {
        uint32_t S = 0;
        recvExact(clientFd, &S, sizeof(S));

        std::unique_ptr<float[]> buf(new float[S]);
        if (S > 0)
            recvExact(clientFd, buf.get(), sizeof(float) * S);

        std::vector<float> vec(buf.get(), buf.get() + S);
        float res = calc_.sumSquares(vec);

        sendExact(clientFd, &res, sizeof(res));
    }

    logger_.writeLog("Served " + peer + " (" + login + ")", false);
}
