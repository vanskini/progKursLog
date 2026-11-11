#pragma once
#include <string>
#include <map>

class Base {
public:
    void load(const std::string& path);

    bool        findLogin(const std::string& login) const;
    std::string getPassword(const std::string& login) const; // BaseError если нет

private:
    std::map<std::string, std::string> data_;
};
