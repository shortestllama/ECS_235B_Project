#ifndef INFO_FLOW_HPP
#define INFO_FLOW_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include "CFG.hpp"
#include "security_policy.hpp"

class InfoFlowAnalysis {
public:
    InfoFlowAnalysis(const CFG& cfg, const SecurityPolicy& policy);
    void run();
    void print_report() const;

private:
    using FactSet = std::unordered_map<std::string, InfoFlowFact>;

    struct AnalysisState {
        FactSet facts;
        InfoFlowFact pc;
    };

    const CFG& cfg;
    const SecurityPolicy& policy;
    std::vector<AnalysisState> in_states;
    std::vector<std::string> findings;

    AnalysisState initial_state() const;
    bool join_into(AnalysisState& target, const AnalysisState& source) const;
    bool join_fact_into(InfoFlowFact& target, const InfoFlowFact& source) const;

    InfoFlowFact public_fact() const;
    InfoFlowFact get_fact(const AnalysisState& state, const std::string& value) const;
    InfoFlowFact object_fact(const std::string& object_name) const;
    InfoFlowFact with_pc(const AnalysisState& state, const InfoFlowFact& fact) const;

    void assign_fact(AnalysisState& state, const std::string& dest, const InfoFlowFact& fact) const;
    void analyze_instruction(const INode& instr, AnalysisState& state);
    void analyze_call(const CallNode& call, AnalysisState& state);

    AnalysisState successor_state(const BasicBlock& block, const AnalysisState& out) const;

    void check_write_arg(const CallNode& call,
                         int arg_index,
                         const std::string& object_name,
                         const InfoFlowFact& destination,
                         const AnalysisState& state);
    void add_blp_finding(const CallNode& call,
                         const std::string& value,
                         const InfoFlowFact& source,
                         const std::string& object_name,
                         const InfoFlowFact& destination);
    void add_biba_finding(const CallNode& call,
                          const std::string& value,
                          const InfoFlowFact& source,
                          const std::string& object_name,
                          const InfoFlowFact& destination);
    void add_finding(const std::string& finding);
};

#endif // INFO_FLOW_HPP
