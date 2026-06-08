#ifndef SECURITY_POLICY_HPP
#define SECURITY_POLICY_HPP

#include <iosfwd>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

struct SecurityLabel {
    int level = 0;
    std::set<std::string> categories;

    bool dominates(const SecurityLabel& other) const;
    static SecurityLabel join(const SecurityLabel& lhs, const SecurityLabel& rhs);
    static SecurityLabel integrity_join(const SecurityLabel& lhs, const SecurityLabel& rhs);
};

struct InfoFlowFact {
    SecurityLabel confidentiality;
    SecurityLabel integrity;

    bool operator==(const InfoFlowFact& other) const;
    bool operator!=(const InfoFlowFact& other) const { return !(*this == other); }
    static InfoFlowFact join(const InfoFlowFact& lhs, const InfoFlowFact& rhs);
};

struct PolicyObject {
    SecurityLabel confidentiality;
    SecurityLabel integrity;
};

struct Effect {
    std::string object;
    InfoFlowFact fact;
    std::optional<std::string> reads;
    std::optional<std::string> writes;
    bool returns = false;
    std::vector<int> reads_from_args;
    std::vector<int> writes_to_args;
};

class SecurityPolicy {
public:
    static SecurityPolicy default_policy();
    static SecurityPolicy parse_file(const std::string& path);

    bool has_blp() const;
    bool has_biba() const;
    InfoFlowFact public_input_fact() const;
    std::optional<Effect> get_effect(const std::string& function) const;
    std::optional<PolicyObject> get_policy_object(const std::string& object) const;
    std::optional<Effect> read_effect(const std::string& function) const;
    std::optional<Effect> write_effect(const std::string& function) const;
    std::string describe_confidentiality(const SecurityLabel& label) const;
    std::string describe_integrity(const SecurityLabel& label) const;
    std::string describe_fact(const InfoFlowFact& fact) const;
    void print(std::ostream& out) const;

private:
    std::vector<std::string> models;
    std::vector<std::string> confidentiality_levels;
    std::vector<std::string> integrity_levels;
    std::unordered_map<std::string, PolicyObject> objects;
    std::unordered_map<std::string, Effect> effects;

    int confidentiality_index(const std::string& level) const;
    int integrity_index(const std::string& level) const;
    std::optional<PolicyObject> get_object(const std::string& object) const;
    std::optional<Effect> resolve_effect_object(const std::string& function, bool is_read) const;
    std::string format_categories(const std::set<std::string>& categories) const;
    std::string format_args(const std::vector<int>& args) const;
};

#endif // SECURITY_POLICY_HPP
