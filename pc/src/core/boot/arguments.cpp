#include "external/CLI11.hpp"

#include "momentum/boot/arguments.hpp"

namespace momentum::args {

Config parse(int& argc, char** argv) {
    CLI::App app{"Momentum"};
    Config config;

    app.add_option("-a,--assets", config.assets_root, "Custom Assets Root");
    app.add_option("-b,--backend", config.backend, "Override Backend Selection");
    app.add_option("-d,--disc", config.disc_location, "Game Disc Location");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        int exit_code = app.exit(e);
        exit(exit_code); 
    }

    return config;
}

}
