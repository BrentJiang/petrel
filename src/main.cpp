#include "server.h"
#include "server_impl.h"
#include "log.h"
#include "options.h"

int main(int argc, char** argv) {
    // Parse command line
    if (!petrel::options::parse(argc, argv)) {
        return 1;
    }
    if (petrel::options::opts.count("help")) {
        std::cout << "petrel version: petrel/" << PETREL_VERSION << std::endl
                  << "Usage: " << argv[0] << " <options>" << std::endl
                  << petrel::options::desc << std::endl;
        return 0;
    }
    if (petrel::options::opts.count("version")) {
        std::cout << "petrel version: petrel/" << PETREL_VERSION << std::endl;
        return 0;
    }
    if (petrel::options::opts.count("test")) {
        std::cout << "Config OK" << std::endl;
        return 0;
    }

    std::clog.rdbuf(new petrel::log("petrel", LOG_LOCAL0, petrel::options::opts.count("log.syslog"),
                                    petrel::options::opts["log.level"].as<int>()));
    std::clog << std::fixed << std::setprecision(2);

    set_log_tag("main");

    try {
        petrel::server s;
        s.impl()->init();
        s.impl()->run();
    } catch (std::exception& e) {
        log_emerg(e.what());
        return 1;
    }

    return 0;
}
