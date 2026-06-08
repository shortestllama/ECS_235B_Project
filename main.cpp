#include <iostream>
#include <fstream>
#include <string>

#include "function.hpp"
#include "CFG.hpp"
#include "info_flow.hpp"
#include "security_policy.hpp"

void print_usage(const char* program) {
    std::cerr << "Usage: " << program << " [options] <file>" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  -h, --help      Show this help message and exit" << std::endl;
    std::cerr << "  -c, --config    Use a security policy config file" << std::endl;
    std::cerr << "  --print-cfg     Print the CFG of each function" << std::endl;
    std::cerr << "  --print-policy  Print the parsed security policy" << std::endl;
}

void analyze_function(const Function& fn, const SecurityPolicy& policy, bool print_cfg) {
    CFG cfg = CFG::build(fn);

    if (print_cfg) {
        cfg.print();
    }

    InfoFlowAnalysis analysis(cfg, policy);
    analysis.run();
    analysis.print_report();
}

int run(const char* input, const char* config, bool print_cfg, bool print_policy) {
    SecurityPolicy policy = SecurityPolicy::default_policy();
    if (config != nullptr) {
        std::ifstream config_file(config);
        if (!config_file.is_open()) {
            std::cerr << "Error opening config file: " << config << std::endl;
            return 1;
        }

        policy = SecurityPolicy::parse_file(config);
    }

    if (print_policy) {
        policy.print(std::cout);
        if (input == nullptr) {
            return 0;
        }
    }

    std::ifstream file(input);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << input << std::endl;
        return 1;
    }

    std::vector<Function> functions = Function::build_functions(file);

    for (const auto& fn : functions) {
        analyze_function(fn, policy, print_cfg);
    }

    return 0;
}

int main(int argc, char* argv[]) {
    bool print_cfg = false;
    bool print_policy = false;
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
        } else if (arg == "--print-cfg") {
            print_cfg = true;
        } else if (arg == "--print-policy") {
            print_policy = true;
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

    if (!input && !print_policy) {
        std::cerr << "No input file specified" << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    return run(input, config, print_cfg, print_policy);
}
