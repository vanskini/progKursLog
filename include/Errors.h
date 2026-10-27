#pragma once
#include <stdexcept>
#include <string>

class ProgramError : public std::runtime_error {
public:
    explicit ProgramError(const std::string& msg) : std::runtime_error(msg) {}
};

class InterfaceError : public ProgramError {
public:
    explicit InterfaceError(const std::string& msg) : ProgramError(msg) {}
};

class BaseError : public ProgramError {
public:
    explicit BaseError(const std::string& msg) : ProgramError(msg) {}
};

class CryptoError : public ProgramError {
public:
    explicit CryptoError(const std::string& msg) : ProgramError(msg) {}
};

class NetworkError : public ProgramError {
public:
    explicit NetworkError(const std::string& msg) : ProgramError(msg) {}
};

class NoCritError : public ProgramError {
public:
    explicit NoCritError(const std::string& msg) : ProgramError(msg) {}
};
