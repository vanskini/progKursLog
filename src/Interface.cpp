#include "Interface.h"
#include "Errors.h"
#include <sstream>

Interface::Interface() : desc_("Allowed options") {
    desc_.add_options()
        ("help,h", "Show help")
        ("base,b", po::value<std::string>(&params_.baseFile)
                       ->default_value("/etc/vcalc.conf"),
                       "Path to auth base file")
        ("log,l",  po::value<std::string>(&params_.logFile)
                       ->default_value("/var/log/vcalc.log"),
                       "Path to log file")
        ("port,p", po::value<uint16_t>(&params_.port)
                       ->default_value(33333),
                       "TCP port to listen");
}

bool Interface::parseArguments(int argc, char** argv) {
    try {
        po::store(po::parse_command_line(argc, argv, desc_), vm_);
    } catch (const po::error& e) {
        throw InterfaceError(e.what());
    }

    if (vm_.count("help") || argc == 1)
        return false;

    try {
        po::notify(vm_);
    } catch (const po::error& e) {
        throw InterfaceError(e.what());
    }
    return true;
}

std::string Interface::displayHelp() const {
    std::ostringstream ss;
    ss << desc_;
    return ss.str();
}
