#include <string>
#include <vector>
#include <fstream>

#include "function.hpp"

Function::Function(const std::string& name, const std::vector<std::string>& body)
    : name(name), body(body) {
}

std::vector<Function> Function::build_functions(std::ifstream& file) {
    std::vector<Function> functions;
    std::string line;

    while (std::getline(file, line)) {
        if (line.compare(0, 6, "define") == 0) {
            std::string fn_name = Function::parse_name(line);
            std::vector<std::string> fn_body = Function::parse_body(file);
            Function fn(fn_name, fn_body);
            functions.push_back(fn);
        }
    }
    return functions;
}

std::string Function::parse_name(const std::string& line) {
    size_t left = line.find('@');
    size_t right = line.find('(');
    return line.substr(left + 1, right - left - 1);
}

std::vector<std::string> Function::parse_body(std::ifstream& file) {
    std::vector<std::string> body;
    std::string line;

    while (std::getline(file, line)) {
        if (line == "}") {
            break;
        }

        const auto first = line.find_first_not_of(" \t");
        if (first == std::string::npos) {
            continue;
        }

        const auto last = line.find_last_not_of(" \t");
        body.push_back(line.substr(first, last - first + 1));
    }

    return body;
}

const std::vector<std::string>& Function::get_body() const {
    return body;
}