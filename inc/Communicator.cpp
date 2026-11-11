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

namespace {
constexpr size_t SALT_HEX_LEN = 16;
constexpr size_t HASH_HEX_LEN = 64;
constexpr size_t AUTH_FIXED  = SALT_HEX_LEN + HASH_HEX_LEN;
constexpr size_t AUTH_MAX    = 1024;
}

Communicator::Communicator(uint16_t port, Base& base, Logger& logger, Calc& calc)
    : port_(port), base_(base), logger_(logger), calc_(calc) {}

Communicator::~Communicator() {
    if (listenFd_ >= 0) ::close(listenFd_);
}

void Communicator::start() {
    listenFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0)
        throw NetworkError(std::string("socket: ") + std::strerror(errno));

    int opt = 1;
    ::setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(port_);
    if (::bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw NetworkError(std::string("bind: ") + std::strerror(errno));

    if (::listen(listenFd_, 10) < 0)
        throw NetworkError(std::string("listen: ") + std::strerror(errno));

    logger_.writeLog("Server listening on port " + std::to_string(port_), false);
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

std::string Communicator::sha256Hex(const std::string& input) const {
    CPP::SHA256 sha;
    std::string hex;
    CPP::StringSource(input, true,
        new CPP::HashFilter(sha,
            new CPP::HexEncoder(new CPP::StringSink(hex))));
    return hex;
}

std::tuple<std::string, std::string, std::string>
Communicator::parseAuthMessage(const std::string& msg) const {
    if (msg.size() < AUTH_FIXED)
        throw CryptoError("Auth message too short");

    const std::string hash  = msg.substr(msg.size() - HASH_HEX_LEN);
    const std::string salt  = msg.substr(msg.size() - AUTH_FIXED, SALT_HEX_LEN);
    const std::string login = msg.substr(0, msg.size() - AUTH_FIXED);

    if (login.empty())
        throw CryptoError("Empty login in auth message");

    return std::make_tuple(login, salt, hash);
}

bool Communicator::verifyHash(const std::string& login,
                              const std::string& salt,
                              const std::string& hash) const {
    const std::string pass = base_.getPassword(login);
    const std::string correct_hex = sha256Hex(salt + pass);

    if (correct_hex.size() != hash.size()) return false;

    return CPP::VerifyBufsEqual(
        reinterpret_cast<const CPP::byte*>(correct_hex.data()),
        reinterpret_cast<const CPP::byte*>(hash.data()),
        correct_hex.size());
}

bool Communicator::authenticate(int fd, std::string& outLogin) {
    std::string data;
    char buf[AUTH_MAX];

    ssize_t r = ::recv(fd, buf, sizeof(buf), 0);
    if (r <= 0) throw NetworkError("Connection closed during auth");
    data.assign(buf, static_cast<size_t>(r));

    if (data.size() < AUTH_FIXED + 1) {
        while (data.size() < AUTH_FIXED + 1) {
            ssize_t r2 = ::recv(fd, buf, sizeof(buf), 0);
            if (r2 <= 0) throw NetworkError("Connection closed during auth");
            data.append(buf, static_cast<size_t>(r2));
        }
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

        std::vector<float> vec(S);
        if (S > 0)
            recvExact(clientFd, vec.data(), sizeof(float) * S);

        const float res = calc_.sumSquares(vec);
        sendExact(clientFd, &res, sizeof(res));
    }

    logger_.writeLog("Served " + peer + " (" + login + ")", false);
}
