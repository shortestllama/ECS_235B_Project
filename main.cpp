#include <iostream>
#include <fstream>
#include <string>

#include "function.hpp"
#include "CFG.hpp"

void print_usage(const char* program) {
    std::cerr << "Usage: " << program << " [options] <file>" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  -h, --help    Show this help message and exit" << std::endl;
    std::cerr << "  -c, --config  Use a security policy config file" << std::endl;
    std::cerr << "  -p, --print   Print the CFG of each function" << std::endl;
}

void enforce_BLP(const Function& fn, bool print_cfg) {
    CFG cfg = CFG::build(fn);

    if (print_cfg) {
        cfg.print();
    }
}

int run(const char* input, const char* config, bool print_cfg) {
    std::ifstream file(input);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << input << std::endl;
        return 1;
    }

    if (config != nullptr) {
        std::ifstream config_file(config);
        if (!config_file.is_open()) {
            std::cerr << "Error opening config file: " << config << std::endl;
            return 1;
        }

        // TODO: Parse the security policy config and pass it into enforce_BLP.
    }

    std::vector<Function> functions = Function::build_functions(file);

    for (const auto& fn : functions) {
        enforce_BLP(fn, print_cfg);
    }

    return 0;
}

int main(int argc, char* argv[]) {
    bool print_cfg = false;
    const char* input = nullptr;
    const char* config = nullptr;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 >= argc) {
                std::cerr << "Missing config file after " << arg << std::endl;
                print_usage(argv[0]);
                return 1;
            }
            config = argv[++i];
        } else if (arg == "-p" || arg == "--print") {
            print_cfg = true;
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        } else if (input == nullptr) {
            input = argv[i];
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    if (!input) {
        std::cerr << "No input file specified" << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    return run(input, config, print_cfg);
}
