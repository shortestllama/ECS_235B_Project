#include <iostream>
#include <fstream>
#include <string>

#include "function.hpp"
#include "CFG.hpp"

void enforce_BLP(const Function& fn) {
    CFG cfg = CFG::build(fn);

    cfg.print();
}

int run(const char* input) {
    std::ifstream file(input);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << input << std::endl;
        return 1;
    }

    std::vector<Function> functions = Function::build_functions(file);

    for (const auto& fn : functions) {
        enforce_BLP(fn);
    }

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
        return 1;
    }

    return run(argv[1]);
}