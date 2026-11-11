#pragma once
#include <string>
#include <cstdint>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

struct Params {
    std::string baseFile = "/etc/vcalc.conf";
    std::string logFile  = "/var/log/vcalc.log";
    uint16_t    port     = 33333;
};

class Interface {
public:
    Interface();

    // true  — параметры разобраны;
    // false — запрошена справка (-h или запуск без аргументов).
    bool parseArguments(int argc, char** argv);

    std::string   displayHelp() const;
    const Params& getParams()   const { return params_; }

private:
    po::options_description desc_;
    po::variables_map       vm_;
    Params                  params_;
};
