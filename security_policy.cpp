#include "security_policy.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace {

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

std::string strip_comment(const std::string& value) {
    bool in_quote = false;
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '"') {
            in_quote = !in_quote;
        } else if (value[i] == '#' && !in_quote) {
            return value.substr(0, i);
        }
    }
    return value;
}

std::vector<std::string> parse_string_array(const std::string& line) {
    std::vector<std::string> values;
    std::regex re("\"([^\"]+)\"");
    auto begin = std::sregex_iterator(line.begin(), line.end(), re);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        values.push_back((*it)[1].str());
    }
    return values;
}

std::vector<int> parse_int_array(const std::string& line) {
    std::vector<int> values;
    std::regex re("-?[0-9]+");
    auto begin = std::sregex_iterator(line.begin(), line.end(), re);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        values.push_back(std::stoi((*it).str()));
    }
    return values;
}

int top_index(const std::vector<std::string>& levels) {
    if (levels.empty()) {
        return 0;
    }
    return static_cast<int>(levels.size() - 1);
}

std::optional<std::string> parse_quoted_value_after_key(const std::string& line, const std::string& key) {
    std::regex re(key + "\\s*=\\s*\"([^\"]+)\"");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return match[1].str();
    }
    return std::nullopt;
}

std::optional<bool> parse_bool_after_key(const std::string& line, const std::string& key) {
    std::regex re(key + "\\s*=\\s*(true|false)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return match[1].str() == "true";
    }
    return std::nullopt;
}

std::vector<int> parse_int_array_after_key(const std::string& line, const std::string& key) {
    std::regex re(key + R"(\s*=\s*\[([^\]]*)\])");
    std::smatch match;
    if (!std::regex_search(line, match, re)) {
        return {};
    }
    return parse_int_array(match[1].str());
}

std::set<std::string> parse_categories_after_key(const std::string& line, const std::string& key) {
    std::regex re(key + R"(\s*=\s*\[([^\]]*)\])");
    std::smatch match;
    if (!std::regex_search(line, match, re)) {
        return {};
    }

    std::vector<std::string> values = parse_string_array(match[1].str());
    return std::set<std::string>(values.begin(), values.end());
}

std::string parse_table_name(const std::string& section, const std::string& prefix) {
    std::string rest = section.substr(prefix.size());
    if (rest.size() >= 2 && rest.front() == '"' && rest.back() == '"') {
        return rest.substr(1, rest.size() - 2);
    }
    return rest;
}

std::string key_before_equals(const std::string& line) {
    const auto equals = line.find('=');
    if (equals == std::string::npos) {
        return "";
    }
    return trim(line.substr(0, equals));
}

} // namespace

bool SecurityLabel::dominates(const SecurityLabel& other) const {
    return level >= other.level
        && std::includes(categories.begin(), categories.end(),
                         other.categories.begin(), other.categories.end());
}

SecurityLabel SecurityLabel::join(const SecurityLabel& lhs, const SecurityLabel& rhs) {
    SecurityLabel out;
    out.level = std::max(lhs.level, rhs.level);
    std::set_union(lhs.categories.begin(), lhs.categories.end(),
                   rhs.categories.begin(), rhs.categories.end(),
                   std::inserter(out.categories, out.categories.begin()));
    return out;
}

SecurityLabel SecurityLabel::integrity_join(const SecurityLabel& lhs, const SecurityLabel& rhs) {
    SecurityLabel out;
    out.level = std::min(lhs.level, rhs.level);
    std::set_union(lhs.categories.begin(), lhs.categories.end(),
                   rhs.categories.begin(), rhs.categories.end(),
                   std::inserter(out.categories, out.categories.begin()));
    return out;
}

bool InfoFlowFact::operator==(const InfoFlowFact& other) const {
    return confidentiality.level == other.confidentiality.level
        && confidentiality.categories == other.confidentiality.categories
        && integrity.level == other.integrity.level
        && integrity.categories == other.integrity.categories;
}

InfoFlowFact InfoFlowFact::join(const InfoFlowFact& lhs, const InfoFlowFact& rhs) {
    return {
        SecurityLabel::join(lhs.confidentiality, rhs.confidentiality),
        SecurityLabel::integrity_join(lhs.integrity, rhs.integrity)
    };
}

SecurityPolicy SecurityPolicy::default_policy() {
    SecurityPolicy policy;
    policy.models = {"Bell-LaPadula"};
    policy.confidentiality_levels = {"Public", "Secret"};
    policy.integrity_levels = {"Untrusted", "Trusted"};
    policy.objects["public_output"] = {{0, {}}, {1, {}}};
    policy.objects["secret_input"] = {{1, {}}, {0, {}}};

    Effect source;
    source.reads = "secret_input";
    source.returns = true;
    policy.effects["SOURCE"] = source;

    Effect sink;
    sink.writes = "public_output";
    sink.reads_from_args = {0};
    policy.effects["SINK"] = sink;
    return policy;
}

SecurityPolicy SecurityPolicy::parse_file(const std::string& path) {
    SecurityPolicy policy;
    std::ifstream file(path);
    std::string section;
    std::string line;
    std::string current_object;
    std::string current_effect;

    while (std::getline(file, line)) {
        line = trim(strip_comment(line));
        if (line.empty() || line[0] == '#') {
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            current_object.clear();
            current_effect.clear();
            if (section.rfind("objects.", 0) == 0) {
                current_object = parse_table_name(section, "objects.");
                policy.objects[current_object] = policy.objects[current_object];
            } else if (section.rfind("effects.", 0) == 0) {
                current_effect = parse_table_name(section, "effects.");
                policy.effects[current_effect] = policy.effects[current_effect];
            }
            continue;
        }

        if (section == "model" && line.rfind("names", 0) == 0) {
            policy.models = parse_string_array(line);
        } else if ((section == "levels" || section == "confidentiality_levels") && line.rfind("order", 0) == 0) {
            policy.confidentiality_levels = parse_string_array(line);
        } else if (section == "integrity_levels" && line.rfind("order", 0) == 0) {
            policy.integrity_levels = parse_string_array(line);
        } else if (section == "objects") {
            std::regex object_re("\"([^\"]+)\"\\s*=\\s*\\{(.+)\\}");
            std::smatch match;
            if (std::regex_search(line, match, object_re)) {
                std::string name = match[1].str();
                std::string body = match[2].str();
                PolicyObject object = policy.objects[name];
                if (auto level = parse_quoted_value_after_key(body, "level")) {
                    object.confidentiality.level = policy.confidentiality_index(*level);
                }
                if (auto integrity = parse_quoted_value_after_key(body, "integrity")) {
                    object.integrity.level = policy.integrity_index(*integrity);
                }
                object.confidentiality.categories = parse_categories_after_key(body, "categories");
                object.integrity.categories = object.confidentiality.categories;
                policy.objects[name] = object;
            }
        } else if (section.rfind("objects.", 0) == 0 && !current_object.empty()) {
            PolicyObject object = policy.objects[current_object];
            if (line.rfind("classification", 0) == 0) {
                auto values = parse_string_array(line);
                if (!values.empty()) {
                    object.confidentiality.level = policy.confidentiality_index(values.front());
                }
            } else if (line.rfind("confidentiality_categories", 0) == 0) {
                auto values = parse_string_array(line);
                object.confidentiality.categories = std::set<std::string>(values.begin(), values.end());
            } else if (line.rfind("integrity_categories", 0) == 0) {
                auto values = parse_string_array(line);
                object.integrity.categories = std::set<std::string>(values.begin(), values.end());
            } else if (line.rfind("integrity", 0) == 0) {
                auto values = parse_string_array(line);
                if (!values.empty()) {
                    object.integrity.level = policy.integrity_index(values.front());
                }
            }
            policy.objects[current_object] = object;
        } else if (section.rfind("effects.", 0) == 0 && !current_effect.empty()) {
            Effect effect = policy.effects[current_effect];
            std::string key = key_before_equals(line);
            if (key == "reads") {
                effect.reads = parse_string_array(line).empty() ? std::optional<std::string>{} : parse_string_array(line).front();
            } else if (key == "writes") {
                effect.writes = parse_string_array(line).empty() ? std::optional<std::string>{} : parse_string_array(line).front();
            } else if (key == "returns") {
                if (auto value = parse_bool_after_key(line, "returns")) {
                    effect.returns = *value;
                }
            } else if (key == "reads_from_args") {
                effect.reads_from_args = parse_int_array_after_key(line, "reads_from_args");
            } else if (key == "writes_to_args") {
                effect.writes_to_args = parse_int_array_after_key(line, "writes_to_args");
            }
            policy.effects[current_effect] = effect;
        }
    }

    for (const auto& [function, effect] : policy.effects) {
        if (effect.reads && policy.objects.find(*effect.reads) == policy.objects.end()) {
            throw std::runtime_error("Effect " + function + " reads unknown object " + *effect.reads);
        }
        if (effect.writes && policy.objects.find(*effect.writes) == policy.objects.end()) {
            throw std::runtime_error("Effect " + function + " writes unknown object " + *effect.writes);
        }
    }

    return policy;
}

bool SecurityPolicy::has_blp() const {
    return std::find(models.begin(), models.end(), "Bell-LaPadula") != models.end();
}

bool SecurityPolicy::has_biba() const {
    return std::find(models.begin(), models.end(), "Biba") != models.end();
}

InfoFlowFact SecurityPolicy::public_input_fact() const {
    return {{0, {}}, {top_index(integrity_levels), {}}};
}

std::optional<Effect> SecurityPolicy::get_effect(const std::string& function) const {
    auto it = effects.find(function);
    if (it == effects.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<PolicyObject> SecurityPolicy::get_policy_object(const std::string& object) const {
    return get_object(object);
}

std::optional<Effect> SecurityPolicy::read_effect(const std::string& function) const {
    return resolve_effect_object(function, true);
}

std::optional<Effect> SecurityPolicy::write_effect(const std::string& function) const {
    return resolve_effect_object(function, false);
}

std::string SecurityPolicy::describe_confidentiality(const SecurityLabel& label) const {
    if (label.level >= 0 && label.level < static_cast<int>(confidentiality_levels.size())) {
        return confidentiality_levels[label.level];
    }
    return "level " + std::to_string(label.level);
}

std::string SecurityPolicy::describe_integrity(const SecurityLabel& label) const {
    if (label.level >= 0 && label.level < static_cast<int>(integrity_levels.size())) {
        return integrity_levels[label.level];
    }
    return "level " + std::to_string(label.level);
}

std::string SecurityPolicy::describe_fact(const InfoFlowFact& fact) const {
    return "confidentiality " + describe_confidentiality(fact.confidentiality)
        + ", integrity " + describe_integrity(fact.integrity);
}

void SecurityPolicy::print(std::ostream& out) const {
    out << "POLICY\n";

    out << "  Models:\n";
    for (const auto& model : models) {
        out << "    - " << model << '\n';
    }

    if (!confidentiality_levels.empty()) {
        out << "\n  Confidentiality levels:\n";
        for (size_t i = 0; i < confidentiality_levels.size(); ++i) {
            out << "    " << i << ": " << confidentiality_levels[i] << '\n';
        }
    }

    if (!integrity_levels.empty()) {
        out << "\n  Integrity levels:\n";
        for (size_t i = 0; i < integrity_levels.size(); ++i) {
            out << "    " << i << ": " << integrity_levels[i] << '\n';
        }
    }

    out << "\n  Objects:\n";
    std::vector<std::string> object_names;
    for (const auto& [name, object] : objects) {
        (void)object;
        object_names.push_back(name);
    }
    std::sort(object_names.begin(), object_names.end());

    for (const auto& name : object_names) {
        const auto& object = objects.at(name);
        out << "    " << name << '\n';
        if (!confidentiality_levels.empty()) {
            out << "      confidentiality: "
                << describe_confidentiality(object.confidentiality)
                << " " << format_categories(object.confidentiality.categories) << '\n';
        }
        if (!integrity_levels.empty()) {
            out << "      integrity: "
                << describe_integrity(object.integrity)
                << " " << format_categories(object.integrity.categories) << '\n';
        }
    }

    out << "\n  Effects:\n";
    std::vector<std::string> effect_names;
    for (const auto& [name, effect] : effects) {
        (void)effect;
        effect_names.push_back(name);
    }
    std::sort(effect_names.begin(), effect_names.end());

    for (const auto& name : effect_names) {
        const auto& effect = effects.at(name);
        out << "    " << name << '\n';
        if (effect.reads) {
            out << "      reads: " << *effect.reads << '\n';
        }
        if (effect.writes) {
            out << "      writes: " << *effect.writes << '\n';
        }
        out << "      returns: " << (effect.returns ? "true" : "false") << '\n';
        if (!effect.reads_from_args.empty()) {
            out << "      reads_from_args: " << format_args(effect.reads_from_args) << '\n';
        }
        if (!effect.writes_to_args.empty()) {
            out << "      writes_to_args: " << format_args(effect.writes_to_args) << '\n';
        }
    }

    out << "\n  Restrictions:\n";
    bool printed = false;
    if (has_blp()) {
        out << "    Bell-LaPadula confidentiality restrictions:\n";
        for (const auto& source_name : object_names) {
            const auto& source = objects.at(source_name);
            for (const auto& destination_name : object_names) {
                if (source_name == destination_name) {
                    continue;
                }
                const auto& destination = objects.at(destination_name);
                if (!destination.confidentiality.dominates(source.confidentiality)) {
                    out << "      " << source_name << " cannot flow to " << destination_name
                        << " because " << describe_confidentiality(destination.confidentiality)
                        << " " << format_categories(destination.confidentiality.categories)
                        << " does not dominate "
                        << describe_confidentiality(source.confidentiality)
                        << " " << format_categories(source.confidentiality.categories)
                        << '\n';
                    printed = true;
                }
            }
        }
    }

    if (has_biba()) {
        out << "    Biba integrity restrictions:\n";
        for (const auto& source_name : object_names) {
            const auto& source = objects.at(source_name);
            for (const auto& destination_name : object_names) {
                if (source_name == destination_name) {
                    continue;
                }
                const auto& destination = objects.at(destination_name);
                if (!source.integrity.dominates(destination.integrity)) {
                    out << "      " << source_name << " cannot flow to " << destination_name
                        << " because " << describe_integrity(source.integrity)
                        << " " << format_categories(source.integrity.categories)
                        << " does not dominate "
                        << describe_integrity(destination.integrity)
                        << " " << format_categories(destination.integrity.categories)
                        << '\n';
                    printed = true;
                }
            }
        }
    }

    out << "    Configured effect restrictions:\n";
    bool printed_effect_restriction = false;
    for (const auto& read_name : effect_names) {
        const auto& read_effect = effects.at(read_name);
        if (!read_effect.reads) {
            continue;
        }

        auto read_object = get_object(*read_effect.reads);
        if (!read_object) {
            continue;
        }

        for (const auto& write_name : effect_names) {
            const auto& write_effect = effects.at(write_name);
            if (!write_effect.writes) {
                continue;
            }

            auto write_object = get_object(*write_effect.writes);
            if (!write_object) {
                continue;
            }

            if (has_blp() && !write_object->confidentiality.dominates(read_object->confidentiality)) {
                out << "      data read by " << read_name << " from " << *read_effect.reads
                    << " cannot be written by " << write_name << " to " << *write_effect.writes
                    << " under Bell-LaPadula\n";
                printed_effect_restriction = true;
            }

            if (has_biba() && !read_object->integrity.dominates(write_object->integrity)) {
                out << "      data read by " << read_name << " from " << *read_effect.reads
                    << " cannot be written by " << write_name << " to " << *write_effect.writes
                    << " under Biba\n";
                printed_effect_restriction = true;
            }
        }
    }
    if (!printed_effect_restriction) {
        out << "      No configured read/write effect restrictions found.\n";
    }

    if (!printed) {
        out << "    No object-to-object restrictions found for the enabled models.\n";
    }
}

int SecurityPolicy::confidentiality_index(const std::string& level) const {
    auto it = std::find(confidentiality_levels.begin(), confidentiality_levels.end(), level);
    if (it == confidentiality_levels.end()) {
        throw std::runtime_error("Unknown confidentiality level: " + level);
    }
    return static_cast<int>(std::distance(confidentiality_levels.begin(), it));
}

int SecurityPolicy::integrity_index(const std::string& level) const {
    auto it = std::find(integrity_levels.begin(), integrity_levels.end(), level);
    if (it == integrity_levels.end()) {
        throw std::runtime_error("Unknown integrity level: " + level);
    }
    return static_cast<int>(std::distance(integrity_levels.begin(), it));
}

std::optional<PolicyObject> SecurityPolicy::get_object(const std::string& object) const {
    auto it = objects.find(object);
    if (it == objects.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<Effect> SecurityPolicy::resolve_effect_object(const std::string& function, bool is_read) const {
    auto it = effects.find(function);
    if (it == effects.end()) {
        return std::nullopt;
    }

    const auto& object_name = is_read ? it->second.reads : it->second.writes;
    if (!object_name) {
        return std::nullopt;
    }

    auto object = get_object(*object_name);
    if (!object) {
        return std::nullopt;
    }

    Effect effect = it->second;
    effect.object = *object_name;
    effect.fact = {object->confidentiality, object->integrity};
    return effect;
}

std::string SecurityPolicy::format_categories(const std::set<std::string>& categories) const {
    std::ostringstream out;
    out << "{";
    bool first = true;
    for (const auto& category : categories) {
        if (!first) {
            out << ", ";
        }
        out << category;
        first = false;
    }
    out << "}";
    return out.str();
}

std::string SecurityPolicy::format_args(const std::vector<int>& args) const {
    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i != 0) {
            out << ", ";
        }
        out << args[i];
    }
    out << "]";
    return out.str();
}
