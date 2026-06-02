#ifndef FUNCTION_HPP
#define FUNCTION_HPP

#include <string>
#include <vector>

class Function {
public:
    Function(const std::string& name, const std::vector<std::string>& parameters, const std::vector<std::string>& body);
    static std::vector<Function> build_functions(std::ifstream& file);
    static std::string parse_name(const std::string& line);
    static std::vector<std::string> parse_parameters(const std::string& line);
    static std::vector<std::string> parse_body(std::ifstream& file);
    const std::vector<std::string>& get_parameters() const;
    const std::vector<std::string>& get_body() const;
private:
    std::string name;
    std::vector<std::string> parameters;
    std::vector<std::string> body;
};

#endif // FUNCTION_HPP
