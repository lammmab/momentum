
#include <string>

namespace momentum::args {

    struct Config {
        std::string backend = "auto";
        std::string assets_root = "";
        std::string disc_location = "";
    };

    Config parse(int& argc, char** argv);
}
