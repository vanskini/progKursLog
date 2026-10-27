#include "Base.h"
#include "Errors.h"
#include <fstream>

void Base::load(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open())
        throw BaseError("Cannot open auth base: " + path);

    data_.clear();
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        auto pos = line.find(':');
        if (pos == std::string::npos || pos == 0)
            throw BaseError("Malformed line in auth base: " + line);
        data_[line.substr(0, pos)] = line.substr(pos + 1);
    }
}

bool Base::findLogin(const std::string& login) const {
    return data_.count(login) != 0;
}

std::string Base::getPassword(const std::string& login) const {
    auto it = data_.find(login);
    if (it == data_.end()) throw BaseError("Unknown login: " + login);
    return it->second;
}
