#pragma once
#include <string>
#include <cstdint>

struct Params {
    std::string baseFile = "/etc/vcalc.conf";
    std::string logFile  = "/var/log/vcalc.log";
    uint16_t    port     = 33333;
};

class Interface {
public:
    Interface() = default;

    bool parseArguments(int argc, char** argv);

    std::string   displayHelp() const;
    const Params& getParams()   const { return params_; }

private:
    Params params_;
};
