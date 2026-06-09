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

void analyze_function(const std::string& function_name,
                      const CFG& cfg,
                      const SecurityPolicy& policy,
                      const InfoFlowAnalysis::SummaryMap& summaries,
                      bool print_cfg,
                      std::vector<std::string>& findings) {
    if (print_cfg) {
        std::cout << "----------------------------" << std::endl;
        std::cout << "Function: " << function_name << std::endl;
        cfg.print();
    }

    InfoFlowAnalysis analysis(cfg, policy, summaries);
    analysis.run();
    for (const auto& finding : analysis.get_findings()) {
        findings.push_back("in " + function_name + ": " + finding);
    }
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
    std::vector<CFG> cfgs;
    cfgs.reserve(functions.size());
    for (const auto& fn : functions) {
        cfgs.push_back(CFG::build(fn));
    }

    InfoFlowAnalysis::SummaryMap summaries = InfoFlowAnalysis::build_summaries(functions, cfgs, policy);

    std::vector<std::string> findings;
    for (size_t i = 0; i < functions.size() && i < cfgs.size(); ++i) {
        analyze_function(functions[i].get_name(), cfgs[i], policy, summaries, print_cfg, findings);
    }

    InfoFlowAnalysis::print_report(std::cout, findings);

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
