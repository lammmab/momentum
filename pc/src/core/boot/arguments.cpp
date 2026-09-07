#include "external/CLI11.hpp"

#include "cr/boot/arguments.hpp"

namespace cr::args {

Config parse(int& argc, char** argv) {
    CLI::App app{"Courage Reborn"};
    Config config;
#if DEBUG
    app.allow_extras();
#endif

    app.add_option("-a,--assets", config.assets_root, "Custom Assets Root");
    app.add_option("-b,--backend", config.backend, "Override Backend Selection");
    app.add_option("-d,--disc", config.disc_location, "Game Disc Location");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        int exit_code = app.exit(e);
        exit(exit_code); 
    }

#if DEBUG
    // What CLI11 didn't consume, in original relative order
    std::vector<std::string> remaining = app.remaining();

    // Compact argv in place: keep argv[0] (program name), then only tokens that are still in `remaining`
    int write = 1;
    for (int read = 1; read < argc; ++read) {
        auto it = std::find(remaining.begin(), remaining.end(), argv[read]);
        if (it != remaining.end()) {
            argv[write++] = argv[read];
            remaining.erase(it);
        }
    }
    argc = write;
    argv[argc] = nullptr; // keep the argv NULL-terminator convention intact
#endif

    return config;
}

}
