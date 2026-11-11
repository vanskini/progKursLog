#include "Interface.h"
#include "Errors.h"
#include <cstdlib>
#include <cstring>
#include <sstream>

bool Interface::parseArguments(int argc, char** argv) {
    if (argc == 1)
        return false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        auto needValue = [&](const char* name) -> std::string {
            if (i + 1 >= argc)
                throw InterfaceError(std::string("Missing value for ") + name);
            return argv[++i];
        };

        if (arg == "-h" || arg == "--help") {
            return false;
        }
        else if (arg == "-b" || arg == "--base") {
            params_.baseFile = needValue(arg.c_str());
        }
        else if (arg == "-l" || arg == "--log") {
            params_.logFile = needValue(arg.c_str());
        }
        else if (arg == "-p" || arg == "--port") {
            const std::string v = needValue(arg.c_str());
            try {
                size_t pos = 0;
                unsigned long p = std::stoul(v, &pos);
                if (pos != v.size() || p == 0 || p > 65535)
                    throw std::invalid_argument("range");
                params_.port = static_cast<uint16_t>(p);
            } catch (const std::exception&) {
                throw InterfaceError("Invalid port value: " + v);
            }
        }
        else {
            throw InterfaceError("Unknown option: " + arg);
        }
    }

    return true;
}

std::string Interface::displayHelp() const {
    std::ostringstream ss;
    ss << "Usage: vcalc [options]\n"
       << "Options:\n"
       << "  -h, --help          Show help\n"
       << "  -b, --base <path>   Path to auth base file "
          "(default: /etc/vcalc.conf)\n"
       << "  -l, --log <path>    Path to log file "
          "(default: /var/log/vcalc.log)\n"
       << "  -p, --port <n>      TCP port to listen "
          "(default: 33333)\n";
    return ss.str();
}
