#include "Interface.h"
#include "Logger.h"
#include "Base.h"
#include "Calc.h"
#include "Communicator.h"
#include "Errors.h"

#include <iostream>
#include <memory>

int main(int argc, char** argv) {
    try {
        Interface iface;
        if (!iface.parseArguments(argc, argv)) {
            std::cout << iface.displayHelp() << std::endl;
            return 0;
        }

        const Params& p = iface.getParams();

        auto logger = std::make_unique<Logger>(p.logFile);
        logger->writeLog("Server starting...", false);

        Base base;
        base.load(p.baseFile);
        logger->writeLog("Auth base loaded: " + p.baseFile, false);

        Calc calc;

        Communicator comm(p.port, base, *logger, calc);
        comm.connection(p.port, /*database=*/{}, *logger);

        comm.run();
        return 0;
    }
    catch (const InterfaceError& e) {
        std::cerr << "Interface error: " << e.what() << "\n"
                  << "Use -h for help\n";
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 2;
    }
    catch (...) {
        std::cerr << "Unknown exception" << std::endl;
        return 3;
    }
}
