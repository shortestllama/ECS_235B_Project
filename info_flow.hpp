#ifndef INFO_FLOW_HPP
#define INFO_FLOW_HPP

#include <optional>
#include <ostream>
#include <string>
#include <set>
#include <unordered_map>
#include <vector>

#include "CFG.hpp"
#include "function.hpp"
#include "security_policy.hpp"

struct FunctionSummary {
    std::set<int> return_args;
    std::optional<InfoFlowFact> return_fact;
    std::unordered_map<int, std::set<int>> pointer_writes;
    std::unordered_map<int, InfoFlowFact> pointer_write_facts;

    bool operator==(const FunctionSummary& other) const;
    bool operator!=(const FunctionSummary& other) const { return !(*this == other); }
};

class InfoFlowAnalysis {
public:
    using SummaryMap = std::unordered_map<std::string, FunctionSummary>;

    InfoFlowAnalysis(const CFG& cfg, const SecurityPolicy& policy);
    InfoFlowAnalysis(const CFG& cfg, const SecurityPolicy& policy, const SummaryMap& summaries);
    static SummaryMap build_summaries(const std::vector<Function>& functions, const SecurityPolicy& policy);
    static SummaryMap build_summaries(const std::vector<Function>& functions,
                                      const std::vector<CFG>& cfgs,
                                      const SecurityPolicy& policy);
    void run();
    const std::vector<std::string>& get_findings() const;
    void print_report() const;
    static void print_report(std::ostream& out, const std::vector<std::string>& findings);

private:
    using FactSet = std::unordered_map<std::string, InfoFlowFact>;

    struct AnalysisState {
        FactSet facts;
        std::unordered_map<unsigned long, InfoFlowFact> pc_contexts;
        InfoFlowFact pc;
    };

    const CFG& cfg;
    const SecurityPolicy& policy;
    const SummaryMap& summaries;
    std::vector<AnalysisState> in_states;
    std::vector<std::string> findings;

    AnalysisState initial_state() const;
    bool join_into(AnalysisState& target, const AnalysisState& source) const;
    bool join_fact_into(InfoFlowFact& target, const InfoFlowFact& source) const;

    InfoFlowFact public_fact() const;
    InfoFlowFact active_pc(const AnalysisState& state) const;
    InfoFlowFact get_fact(const AnalysisState& state, const std::string& value) const;
    InfoFlowFact object_fact(const std::string& object_name) const;
    InfoFlowFact with_pc(const AnalysisState& state, const InfoFlowFact& fact) const;

    void assign_fact(AnalysisState& state, const std::string& dest, const InfoFlowFact& fact) const;
    void drop_stopping_pc_contexts(AnalysisState& state, unsigned long block_id) const;
    void analyze_instruction(const INode& instr, AnalysisState& state);
    void analyze_call(const CallNode& call, AnalysisState& state);

    AnalysisState successor_state(const BasicBlock& block, const AnalysisState& out) const;
    std::optional<unsigned long> reconvergence_block(unsigned long block_id) const;

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
