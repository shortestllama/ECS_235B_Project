#include <string>
#include <vector>
#include <fstream>
#include <regex>

#include "function.hpp"

namespace {

std::string trim(const std::string& line) {
    const auto first = line.find_first_not_of(" \t");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = line.find_last_not_of(" \t");
    return line.substr(first, last - first + 1);
}

std::string strip_comment(const std::string& line) {
    const auto comment = line.find(';');
    if (comment == std::string::npos) {
        return line;
    }
    return line.substr(0, comment);
}

bool starts_with_switch_block(const std::string& line) {
    return line.rfind("switch ", 0) == 0
        && line.find('[') != std::string::npos
        && line.find(']') == std::string::npos;
}

} // namespace

Function::Function(const std::string& name, const std::vector<std::string>& parameters, const std::vector<std::string>& body)
    : name(name), parameters(parameters), body(body) {
}

std::vector<Function> Function::build_functions(std::ifstream& file) {
    std::vector<Function> functions;
    std::string line;

    while (std::getline(file, line)) {
        if (line.compare(0, 6, "define") == 0) {
            std::string fn_name = Function::parse_name(line);
            std::vector<std::string> fn_parameters = Function::parse_parameters(line);
            std::vector<std::string> fn_body = Function::parse_body(file);
            Function fn(fn_name, fn_parameters, fn_body);
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

std::vector<std::string> Function::parse_parameters(const std::string& line) {
    std::vector<std::string> parameters;
    size_t left = line.find('(');
    size_t right = line.rfind(')');
    if (left == std::string::npos || right == std::string::npos || right <= left) {
        return parameters;
    }

    std::string parameter_list = line.substr(left + 1, right - left - 1);
    std::regex parameter_re(R"(%[\w.]+)");
    auto begin = std::sregex_iterator(parameter_list.begin(), parameter_list.end(), parameter_re);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        parameters.push_back((*it).str());
    }
    return parameters;
}

std::vector<std::string> Function::parse_body(std::ifstream& file) {
    std::vector<std::string> body;
    std::string line;

    while (std::getline(file, line)) {
        if (line == "}") {
            break;
        }

        line = trim(strip_comment(line));
        if (line.empty()) {
            continue;
        }

        if (starts_with_switch_block(line)) {
            std::string switch_line = line + " ";
            while (std::getline(file, line)) {
                line = trim(strip_comment(line));
                if (line.empty()) {
                    continue;
                }

                switch_line += line + " ";
                if (line.find(']') != std::string::npos) {
                    break;
                }
            }
            body.push_back(trim(switch_line));
            continue;
        }

        body.push_back(line);
    }

    return body;
}

const std::vector<std::string>& Function::get_parameters() const {
    return parameters;
}

const std::string& Function::get_name() const {
    return name;
}

const std::vector<std::string>& Function::get_body() const {
    return body;
}
