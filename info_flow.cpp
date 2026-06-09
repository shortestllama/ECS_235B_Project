#include "info_flow.hpp"

#include <algorithm>
#include <iostream>
#include <queue>
#include <set>
#include <sstream>

namespace {

using DependencySet = std::set<int>;
using DependencyFacts = std::unordered_map<std::string, DependencySet>;
using SummaryMap = InfoFlowAnalysis::SummaryMap;

struct DependencyState {
    DependencyFacts facts;
    DependencySet pc;
    std::unordered_map<std::string, InfoFlowFact> label_facts;
    InfoFlowFact pc_fact;
};

const SummaryMap& empty_summaries() {
    static const SummaryMap summaries;
    return summaries;
}

bool merge_dependencies(DependencySet& target, const DependencySet& source) {
    size_t old_size = target.size();
    target.insert(source.begin(), source.end());
    return target.size() != old_size;
}

bool merge_fact(InfoFlowFact& target, const InfoFlowFact& source) {
    InfoFlowFact joined = InfoFlowFact::join(target, source);
    if (joined != target) {
        target = joined;
        return true;
    }
    return false;
}

bool merge_optional_fact(std::optional<InfoFlowFact>& target, const InfoFlowFact& source) {
    if (!target) {
        target = source;
        return true;
    }
    return merge_fact(*target, source);
}

InfoFlowFact object_fact(const SecurityPolicy& policy, const std::string& object_name) {
    auto object = policy.get_policy_object(object_name);
    if (!object) {
        return policy.public_input_fact();
    }
    return {object->confidentiality, object->integrity};
}

DependencySet get_dependencies(const DependencyFacts& facts, const std::string& value) {
    auto it = facts.find(value);
    if (it == facts.end()) {
        return {};
    }
    return it->second;
}

InfoFlowFact get_label_fact(const DependencyState& state,
                            const SecurityPolicy& policy,
                            const std::string& value) {
    auto it = state.label_facts.find(value);
    if (it == state.label_facts.end()) {
        return policy.public_input_fact();
    }
    return it->second;
}

void assign_summary_value(DependencyState& state,
                          const SecurityPolicy& policy,
                          const std::string& dest,
                          DependencySet deps,
                          InfoFlowFact fact) {
    if (dest.empty() || dest[0] != '%') {
        return;
    }

    merge_dependencies(deps, state.pc);
    state.facts[dest] = deps;
    state.label_facts[dest] = InfoFlowFact::join(fact, state.pc_fact);
}

bool join_dependency_state(DependencyState& target, const DependencyState& source) {
    bool changed = merge_dependencies(target.pc, source.pc);
    if (merge_fact(target.pc_fact, source.pc_fact)) {
        changed = true;
    }
    for (const auto& [value, deps] : source.facts) {
        if (merge_dependencies(target.facts[value], deps)) {
            changed = true;
        }
    }
    for (const auto& [value, fact] : source.label_facts) {
        auto it = target.label_facts.find(value);
        if (it == target.label_facts.end()) {
            target.label_facts[value] = fact;
            changed = true;
        } else if (merge_fact(it->second, fact)) {
            changed = true;
        }
    }
    return changed;
}

bool merge_summary(FunctionSummary& target, const FunctionSummary& source) {
    bool changed = merge_dependencies(target.return_args, source.return_args);
    if (source.return_fact && merge_optional_fact(target.return_fact, *source.return_fact)) {
        changed = true;
    }
    for (const auto& [arg_index, deps] : source.pointer_writes) {
        if (merge_dependencies(target.pointer_writes[arg_index], deps)) {
            changed = true;
        }
    }
    for (const auto& [arg_index, fact] : source.pointer_write_facts) {
        auto it = target.pointer_write_facts.find(arg_index);
        if (it == target.pointer_write_facts.end()) {
            target.pointer_write_facts[arg_index] = fact;
            changed = true;
        } else if (merge_fact(it->second, fact)) {
            changed = true;
        }
    }
    return changed;
}

void apply_summary_call_dependencies(const CallNode& call,
                                     const FunctionSummary& summary,
                                     const SecurityPolicy& policy,
                                     DependencyState& state) {
    DependencySet result;
    InfoFlowFact result_fact = summary.return_fact.value_or(policy.public_input_fact());
    for (int dep_arg : summary.return_args) {
        if (dep_arg >= 0 && static_cast<size_t>(dep_arg) < call.get_args().size()) {
            merge_dependencies(result, get_dependencies(state.facts, call.get_args()[dep_arg]));
            merge_fact(result_fact, get_label_fact(state, policy, call.get_args()[dep_arg]));
        }
    }
    assign_summary_value(state, policy, call.get_dest(), result, result_fact);

    for (const auto& [written_arg, dep_args] : summary.pointer_writes) {
        if (written_arg < 0 || static_cast<size_t>(written_arg) >= call.get_args().size()) {
            continue;
        }

        DependencySet written_deps;
        InfoFlowFact written_fact = policy.public_input_fact();
        if (auto fact_it = summary.pointer_write_facts.find(written_arg); fact_it != summary.pointer_write_facts.end()) {
            written_fact = fact_it->second;
        }

        for (int dep_arg : dep_args) {
            if (dep_arg >= 0 && static_cast<size_t>(dep_arg) < call.get_args().size()) {
                merge_dependencies(written_deps, get_dependencies(state.facts, call.get_args()[dep_arg]));
                merge_fact(written_fact, get_label_fact(state, policy, call.get_args()[dep_arg]));
            }
        }
        assign_summary_value(state, policy, call.get_args()[written_arg], written_deps, written_fact);
    }

    for (const auto& [written_arg, fact] : summary.pointer_write_facts) {
        if (summary.pointer_writes.find(written_arg) != summary.pointer_writes.end()
            || written_arg < 0
            || static_cast<size_t>(written_arg) >= call.get_args().size()) {
            continue;
        }
        assign_summary_value(state, policy, call.get_args()[written_arg], {}, fact);
    }
}

void record_pointer_write(const std::string& pointer,
                          const DependencySet& source,
                          const InfoFlowFact& source_fact,
                          const DependencyState& state,
                          const SecurityPolicy& policy,
                          FunctionSummary& summary) {
    DependencySet pointer_deps = get_dependencies(state.facts, pointer);
    DependencySet written_deps = source;
    merge_dependencies(written_deps, state.pc);
    InfoFlowFact written_fact = InfoFlowFact::join(source_fact, state.pc_fact);

    for (int pointer_arg : pointer_deps) {
        DependencySet deps = written_deps;
        deps.erase(pointer_arg);
        if (!deps.empty()) {
            merge_dependencies(summary.pointer_writes[pointer_arg], deps);
        }
        if (written_fact != policy.public_input_fact()) {
            auto it = summary.pointer_write_facts.find(pointer_arg);
            if (it == summary.pointer_write_facts.end()) {
                summary.pointer_write_facts[pointer_arg] = written_fact;
            } else {
                merge_fact(it->second, written_fact);
            }
        }
    }
}

DependencyState dependency_successor_state(const BasicBlock& block,
                                           const DependencyState& out,
                                           const SecurityPolicy& policy) {
    DependencyState next = out;

    if (const auto& term = block.get_term()) {
        if (!term->get_condition().empty()) {
            merge_dependencies(next.pc, get_dependencies(out.facts, term->get_condition()));
            merge_fact(next.pc_fact, get_label_fact(out, policy, term->get_condition()));
        }
    } else if (!block.get_instructions().empty()) {
        if (const auto* node = dynamic_cast<const SwitchNode*>(block.get_instructions().back().get())) {
            merge_dependencies(next.pc, get_dependencies(out.facts, node->get_condition()));
            merge_fact(next.pc_fact, get_label_fact(out, policy, node->get_condition()));
        }
    }

    return next;
}

FunctionSummary summarize_function(const Function& fn,
                                   const CFG& cfg,
                                   const SecurityPolicy& policy,
                                   const SummaryMap& summaries) {
    std::vector<DependencyState> in_states(cfg.get_blocks().size());
    FunctionSummary summary;

    const auto& parameters = fn.get_parameters();
    for (auto& state : in_states) {
        state.pc_fact = policy.public_input_fact();
        for (size_t i = 0; i < parameters.size(); ++i) {
            state.facts[parameters[i]] = {static_cast<int>(i)};
            state.label_facts[parameters[i]] = policy.public_input_fact();
        }
    }

    if (cfg.get_blocks().empty()) {
        return summary;
    }

    std::queue<unsigned long> worklist;
    std::vector<bool> queued(cfg.get_blocks().size(), false);
    worklist.push(0);
    queued[0] = true;

    while (!worklist.empty()) {
        unsigned long id = worklist.front();
        worklist.pop();
        queued[id] = false;

        DependencyState out = in_states[id];
        for (const auto& instr : cfg.get_blocks()[id].get_instructions()) {
            if (const auto* node = dynamic_cast<const AllocaNode*>(instr.get())) {
                assign_summary_value(out, policy, node->get_dest(), {}, policy.public_input_fact());
            } else if (const auto* node = dynamic_cast<const GEPNode*>(instr.get())) {
                assign_summary_value(out, policy, node->get_dest(),
                                     get_dependencies(out.facts, node->get_src()),
                                     get_label_fact(out, policy, node->get_src()));
            } else if (const auto* node = dynamic_cast<const LoadNode*>(instr.get())) {
                assign_summary_value(out, policy, node->get_dest(),
                                     get_dependencies(out.facts, node->get_src()),
                                     get_label_fact(out, policy, node->get_src()));
            } else if (const auto* node = dynamic_cast<const StoreNode*>(instr.get())) {
                DependencySet deps = get_dependencies(out.facts, node->get_src());
                InfoFlowFact fact = get_label_fact(out, policy, node->get_src());
                record_pointer_write(node->get_dest(), deps, fact, out, policy, summary);
                assign_summary_value(out, policy, node->get_dest(), deps, fact);
            } else if (const auto* node = dynamic_cast<const CmpNode*>(instr.get())) {
                DependencySet deps = get_dependencies(out.facts, node->get_src1());
                merge_dependencies(deps, get_dependencies(out.facts, node->get_src2()));
                InfoFlowFact fact = get_label_fact(out, policy, node->get_src1());
                merge_fact(fact, get_label_fact(out, policy, node->get_src2()));
                assign_summary_value(out, policy, node->get_dest(), deps, fact);
            } else if (const auto* node = dynamic_cast<const CastNode*>(instr.get())) {
                assign_summary_value(out, policy, node->get_dest(),
                                     get_dependencies(out.facts, node->get_src()),
                                     get_label_fact(out, policy, node->get_src()));
            } else if (const auto* node = dynamic_cast<const SelectNode*>(instr.get())) {
                DependencySet deps = get_dependencies(out.facts, node->get_condition());
                merge_dependencies(deps, get_dependencies(out.facts, node->get_true_value()));
                merge_dependencies(deps, get_dependencies(out.facts, node->get_false_value()));
                InfoFlowFact fact = get_label_fact(out, policy, node->get_condition());
                merge_fact(fact, get_label_fact(out, policy, node->get_true_value()));
                merge_fact(fact, get_label_fact(out, policy, node->get_false_value()));
                assign_summary_value(out, policy, node->get_dest(), deps, fact);
            } else if (const auto* node = dynamic_cast<const AddNode*>(instr.get())) {
                DependencySet deps = get_dependencies(out.facts, node->get_src1());
                merge_dependencies(deps, get_dependencies(out.facts, node->get_src2()));
                InfoFlowFact fact = get_label_fact(out, policy, node->get_src1());
                merge_fact(fact, get_label_fact(out, policy, node->get_src2()));
                assign_summary_value(out, policy, node->get_dest(), deps, fact);
            } else if (const auto* node = dynamic_cast<const SubNode*>(instr.get())) {
                DependencySet deps = get_dependencies(out.facts, node->get_src1());
                merge_dependencies(deps, get_dependencies(out.facts, node->get_src2()));
                InfoFlowFact fact = get_label_fact(out, policy, node->get_src1());
                merge_fact(fact, get_label_fact(out, policy, node->get_src2()));
                assign_summary_value(out, policy, node->get_dest(), deps, fact);
            } else if (const auto* node = dynamic_cast<const MulNode*>(instr.get())) {
                DependencySet deps = get_dependencies(out.facts, node->get_src1());
                merge_dependencies(deps, get_dependencies(out.facts, node->get_src2()));
                InfoFlowFact fact = get_label_fact(out, policy, node->get_src1());
                merge_fact(fact, get_label_fact(out, policy, node->get_src2()));
                assign_summary_value(out, policy, node->get_dest(), deps, fact);
            } else if (const auto* node = dynamic_cast<const DivNode*>(instr.get())) {
                DependencySet deps = get_dependencies(out.facts, node->get_src1());
                merge_dependencies(deps, get_dependencies(out.facts, node->get_src2()));
                InfoFlowFact fact = get_label_fact(out, policy, node->get_src1());
                merge_fact(fact, get_label_fact(out, policy, node->get_src2()));
                assign_summary_value(out, policy, node->get_dest(), deps, fact);
            } else if (const auto* node = dynamic_cast<const BinaryOpNode*>(instr.get())) {
                DependencySet deps = get_dependencies(out.facts, node->get_src1());
                merge_dependencies(deps, get_dependencies(out.facts, node->get_src2()));
                InfoFlowFact fact = get_label_fact(out, policy, node->get_src1());
                merge_fact(fact, get_label_fact(out, policy, node->get_src2()));
                assign_summary_value(out, policy, node->get_dest(), deps, fact);
            } else if (const auto* node = dynamic_cast<const PhiNode*>(instr.get())) {
                DependencySet deps;
                InfoFlowFact fact = policy.public_input_fact();
                for (const auto& [value, label] : node->get_incoming()) {
                    (void)label;
                    merge_dependencies(deps, get_dependencies(out.facts, value));
                    merge_fact(fact, get_label_fact(out, policy, value));
                }
                assign_summary_value(out, policy, node->get_dest(), deps, fact);
            } else if (const auto* node = dynamic_cast<const CallNode*>(instr.get())) {
                if (auto effect = policy.get_effect(node->get_fn_name())) {
                    for (int written_arg : effect->writes_to_args) {
                        if (written_arg >= 0 && static_cast<size_t>(written_arg) < node->get_args().size()) {
                            InfoFlowFact source = effect->reads ? object_fact(policy, *effect->reads) : policy.public_input_fact();
                            assign_summary_value(out, policy, node->get_args()[written_arg], {}, source);
                        }
                    }
                    if (effect->returns && effect->reads) {
                        assign_summary_value(out, policy, node->get_dest(), {}, object_fact(policy, *effect->reads));
                    } else {
                        assign_summary_value(out, policy, node->get_dest(), {}, policy.public_input_fact());
                    }
                } else if (auto summary_it = summaries.find(node->get_fn_name()); summary_it != summaries.end()) {
                    apply_summary_call_dependencies(*node, summary_it->second, policy, out);
                } else {
                    DependencySet deps;
                    InfoFlowFact fact = policy.public_input_fact();
                    for (const auto& arg : node->get_args()) {
                        merge_dependencies(deps, get_dependencies(out.facts, arg));
                        merge_fact(fact, get_label_fact(out, policy, arg));
                    }
                    assign_summary_value(out, policy, node->get_dest(), deps, fact);
                }
            } else if (const auto* node = dynamic_cast<const RetNode*>(instr.get())) {
                DependencySet deps = get_dependencies(out.facts, node->get_src());
                merge_dependencies(deps, out.pc);
                merge_dependencies(summary.return_args, deps);
                InfoFlowFact fact = get_label_fact(out, policy, node->get_src());
                merge_fact(fact, out.pc_fact);
                if (fact != policy.public_input_fact()) {
                    merge_optional_fact(summary.return_fact, fact);
                }
            }
        }

        DependencyState next = dependency_successor_state(cfg.get_blocks()[id], out, policy);
        for (unsigned long succ : cfg.get_blocks()[id].get_successors()) {
            if (succ >= in_states.size()) {
                continue;
            }
            if (join_dependency_state(in_states[succ], next) && !queued[succ]) {
                worklist.push(succ);
                queued[succ] = true;
            }
        }
    }

    return summary;
}

} // namespace

bool FunctionSummary::operator==(const FunctionSummary& other) const {
    return return_args == other.return_args
        && return_fact == other.return_fact
        && pointer_writes == other.pointer_writes
        && pointer_write_facts == other.pointer_write_facts;
}

InfoFlowAnalysis::InfoFlowAnalysis(const CFG& cfg, const SecurityPolicy& policy)
    : InfoFlowAnalysis(cfg, policy, empty_summaries()) {
}

InfoFlowAnalysis::InfoFlowAnalysis(const CFG& cfg, const SecurityPolicy& policy, const SummaryMap& summaries)
    : cfg(cfg), policy(policy), summaries(summaries) {
    in_states.resize(cfg.get_blocks().size(), initial_state());
}

InfoFlowAnalysis::SummaryMap InfoFlowAnalysis::build_summaries(const std::vector<Function>& functions,
                                                               const SecurityPolicy& policy) {
    std::vector<CFG> cfgs;
    cfgs.reserve(functions.size());
    for (const auto& fn : functions) {
        cfgs.push_back(CFG::build(fn));
    }
    return build_summaries(functions, cfgs, policy);
}

InfoFlowAnalysis::SummaryMap InfoFlowAnalysis::build_summaries(const std::vector<Function>& functions,
                                                               const std::vector<CFG>& cfgs,
                                                               const SecurityPolicy& policy) {
    SummaryMap summaries;
    bool changed = true;

    while (changed) {
        changed = false;
        for (size_t i = 0; i < functions.size() && i < cfgs.size(); ++i) {
            FunctionSummary next = summarize_function(functions[i], cfgs[i], policy, summaries);
            if (merge_summary(summaries[functions[i].get_name()], next)) {
                changed = true;
            }
        }
    }

    return summaries;
}

InfoFlowAnalysis::AnalysisState InfoFlowAnalysis::initial_state() const {
    AnalysisState state;
    state.pc = public_fact();
    for (const auto& var : cfg.get_variables()) {
        state.facts[var] = public_fact();
    }
    return state;
}

bool InfoFlowAnalysis::join_fact_into(InfoFlowFact& target, const InfoFlowFact& source) const {
    InfoFlowFact joined = InfoFlowFact::join(target, source);
    if (joined != target) {
        target = joined;
        return true;
    }
    return false;
}

bool InfoFlowAnalysis::join_into(AnalysisState& target, const AnalysisState& source) const {
    bool changed = join_fact_into(target.pc, source.pc);

    for (const auto& [var, fact] : source.facts) {
        auto it = target.facts.find(var);
        if (it == target.facts.end()) {
            target.facts[var] = fact;
            changed = true;
        } else if (join_fact_into(it->second, fact)) {
            changed = true;
        }
    }

    return changed;
}

InfoFlowFact InfoFlowAnalysis::public_fact() const {
    return policy.public_input_fact();
}

InfoFlowFact InfoFlowAnalysis::get_fact(const AnalysisState& state, const std::string& value) const {
    auto it = state.facts.find(value);
    if (it == state.facts.end()) {
        return public_fact();
    }
    return it->second;
}

InfoFlowFact InfoFlowAnalysis::object_fact(const std::string& object_name) const {
    auto object = policy.get_policy_object(object_name);
    if (!object) {
        return public_fact();
    }
    return {object->confidentiality, object->integrity};
}

InfoFlowFact InfoFlowAnalysis::with_pc(const AnalysisState& state, const InfoFlowFact& fact) const {
    return InfoFlowFact::join(fact, state.pc);
}

void InfoFlowAnalysis::assign_fact(AnalysisState& state, const std::string& dest, const InfoFlowFact& fact) const {
    if (!dest.empty() && dest[0] == '%') {
        state.facts[dest] = with_pc(state, fact);
    }
}

void InfoFlowAnalysis::analyze_instruction(const INode& instr, AnalysisState& state) {
    if (const auto* node = dynamic_cast<const AllocaNode*>(&instr)) {
        assign_fact(state, node->get_dest(), public_fact());
    } else if (const auto* node = dynamic_cast<const GEPNode*>(&instr)) {
        assign_fact(state, node->get_dest(), get_fact(state, node->get_src()));
    } else if (const auto* node = dynamic_cast<const LoadNode*>(&instr)) {
        assign_fact(state, node->get_dest(), get_fact(state, node->get_src()));
    } else if (const auto* node = dynamic_cast<const StoreNode*>(&instr)) {
        assign_fact(state, node->get_dest(), get_fact(state, node->get_src()));
    } else if (const auto* node = dynamic_cast<const CmpNode*>(&instr)) {
        assign_fact(state, node->get_dest(), InfoFlowFact::join(get_fact(state, node->get_src1()), get_fact(state, node->get_src2())));
    } else if (const auto* node = dynamic_cast<const CastNode*>(&instr)) {
        assign_fact(state, node->get_dest(), get_fact(state, node->get_src()));
    } else if (const auto* node = dynamic_cast<const SelectNode*>(&instr)) {
        InfoFlowFact values = InfoFlowFact::join(get_fact(state, node->get_true_value()), get_fact(state, node->get_false_value()));
        InfoFlowFact selected = InfoFlowFact::join(get_fact(state, node->get_condition()), values);
        assign_fact(state, node->get_dest(), selected);
    } else if (const auto* node = dynamic_cast<const AddNode*>(&instr)) {
        assign_fact(state, node->get_dest(), InfoFlowFact::join(get_fact(state, node->get_src1()), get_fact(state, node->get_src2())));
    } else if (const auto* node = dynamic_cast<const SubNode*>(&instr)) {
        assign_fact(state, node->get_dest(), InfoFlowFact::join(get_fact(state, node->get_src1()), get_fact(state, node->get_src2())));
    } else if (const auto* node = dynamic_cast<const MulNode*>(&instr)) {
        assign_fact(state, node->get_dest(), InfoFlowFact::join(get_fact(state, node->get_src1()), get_fact(state, node->get_src2())));
    } else if (const auto* node = dynamic_cast<const DivNode*>(&instr)) {
        assign_fact(state, node->get_dest(), InfoFlowFact::join(get_fact(state, node->get_src1()), get_fact(state, node->get_src2())));
    } else if (const auto* node = dynamic_cast<const BinaryOpNode*>(&instr)) {
        assign_fact(state, node->get_dest(), InfoFlowFact::join(get_fact(state, node->get_src1()), get_fact(state, node->get_src2())));
    } else if (const auto* node = dynamic_cast<const PhiNode*>(&instr)) {
        InfoFlowFact joined = public_fact();
        for (const auto& [value, label] : node->get_incoming()) {
            (void)label;
            joined = InfoFlowFact::join(joined, get_fact(state, value));
        }
        assign_fact(state, node->get_dest(), joined);
    } else if (const auto* node = dynamic_cast<const CallNode*>(&instr)) {
        analyze_call(*node, state);
    }
}

void InfoFlowAnalysis::analyze_call(const CallNode& call, AnalysisState& state) {
    auto effect = policy.get_effect(call.get_fn_name());

    if (!effect) {
        auto summary_it = summaries.find(call.get_fn_name());
        if (summary_it != summaries.end()) {
            InfoFlowFact result = summary_it->second.return_fact.value_or(public_fact());
            for (int dep_arg : summary_it->second.return_args) {
                if (dep_arg >= 0 && static_cast<size_t>(dep_arg) < call.get_args().size()) {
                    result = InfoFlowFact::join(result, get_fact(state, call.get_args()[dep_arg]));
                }
            }
            assign_fact(state, call.get_dest(), result);

            for (const auto& [written_arg, dep_args] : summary_it->second.pointer_writes) {
                if (written_arg < 0 || static_cast<size_t>(written_arg) >= call.get_args().size()) {
                    continue;
                }

                InfoFlowFact written_fact = public_fact();
                if (auto fact_it = summary_it->second.pointer_write_facts.find(written_arg);
                    fact_it != summary_it->second.pointer_write_facts.end()) {
                    written_fact = fact_it->second;
                }
                for (int dep_arg : dep_args) {
                    if (dep_arg >= 0 && static_cast<size_t>(dep_arg) < call.get_args().size()) {
                        written_fact = InfoFlowFact::join(written_fact, get_fact(state, call.get_args()[dep_arg]));
                    }
                }
                assign_fact(state, call.get_args()[written_arg], written_fact);
            }

            for (const auto& [written_arg, fact] : summary_it->second.pointer_write_facts) {
                if (summary_it->second.pointer_writes.find(written_arg) != summary_it->second.pointer_writes.end()
                    || written_arg < 0
                    || static_cast<size_t>(written_arg) >= call.get_args().size()) {
                    continue;
                }
                assign_fact(state, call.get_args()[written_arg], fact);
            }
            return;
        }

        InfoFlowFact result = public_fact();
        for (const auto& arg : call.get_args()) {
            result = InfoFlowFact::join(result, get_fact(state, arg));
        }
        assign_fact(state, call.get_dest(), result);
        return;
    }

    if (effect->writes) {
        InfoFlowFact destination = object_fact(*effect->writes);
        for (int arg_index : effect->reads_from_args) {
            check_write_arg(call, arg_index, *effect->writes, destination, state);
        }
    }

    if (effect->reads) {
        InfoFlowFact source = object_fact(*effect->reads);
        for (int arg_index : effect->writes_to_args) {
            if (arg_index >= 0 && static_cast<size_t>(arg_index) < call.get_args().size()) {
                assign_fact(state, call.get_args()[arg_index], source);
            }
        }

        if (effect->returns) {
            assign_fact(state, call.get_dest(), source);
        }
    }

    if (!effect->returns && !call.get_dest().empty()) {
        InfoFlowFact result = public_fact();
        for (int arg_index : effect->reads_from_args) {
            if (arg_index >= 0 && static_cast<size_t>(arg_index) < call.get_args().size()) {
                result = InfoFlowFact::join(result, get_fact(state, call.get_args()[arg_index]));
            }
        }
        assign_fact(state, call.get_dest(), result);
    }
}

InfoFlowAnalysis::AnalysisState InfoFlowAnalysis::successor_state(const BasicBlock& block, const AnalysisState& out) const {
    AnalysisState next = out;

    if (const auto& term = block.get_term()) {
        if (!term->get_condition().empty()) {
            next.pc = InfoFlowFact::join(next.pc, get_fact(out, term->get_condition()));
        }
    } else if (!block.get_instructions().empty()) {
        if (const auto* node = dynamic_cast<const SwitchNode*>(block.get_instructions().back().get())) {
            next.pc = InfoFlowFact::join(next.pc, get_fact(out, node->get_condition()));
        }
    }

    return next;
}

void InfoFlowAnalysis::check_write_arg(const CallNode& call,
                                       int arg_index,
                                       const std::string& object_name,
                                       const InfoFlowFact& destination,
                                       const AnalysisState& state) {
    if (arg_index < 0 || static_cast<size_t>(arg_index) >= call.get_args().size()) {
        return;
    }

    const std::string& value = call.get_args()[arg_index];
    InfoFlowFact source = with_pc(state, get_fact(state, value));

    if (policy.has_blp() && !destination.confidentiality.dominates(source.confidentiality)) {
        add_blp_finding(call, value, source, object_name, destination);
    }

    if (policy.has_biba() && !source.integrity.dominates(destination.integrity)) {
        add_biba_finding(call, value, source, object_name, destination);
    }
}

void InfoFlowAnalysis::add_blp_finding(const CallNode& call,
                                       const std::string& value,
                                       const InfoFlowFact& source,
                                       const std::string& object_name,
                                       const InfoFlowFact& destination) {
    std::ostringstream out;
    out << "BLP violation: " << value << " flows to " << call.get_fn_name()
        << " writing " << object_name
        << "; destination confidentiality " << policy.describe_confidentiality(destination.confidentiality)
        << " does not dominate source confidentiality " << policy.describe_confidentiality(source.confidentiality);
    add_finding(out.str());
}

void InfoFlowAnalysis::add_biba_finding(const CallNode& call,
                                        const std::string& value,
                                        const InfoFlowFact& source,
                                        const std::string& object_name,
                                        const InfoFlowFact& destination) {
    std::ostringstream out;
    out << "Biba violation: " << value << " flows to " << call.get_fn_name()
        << " writing " << object_name
        << "; source integrity " << policy.describe_integrity(source.integrity)
        << " does not dominate destination integrity " << policy.describe_integrity(destination.integrity);
    add_finding(out.str());
}

void InfoFlowAnalysis::add_finding(const std::string& finding) {
    if (std::find(findings.begin(), findings.end(), finding) == findings.end()) {
        findings.push_back(finding);
    }
}

void InfoFlowAnalysis::run() {
    if (cfg.get_blocks().empty()) {
        return;
    }

    std::queue<unsigned long> worklist;
    std::vector<bool> queued(cfg.get_blocks().size(), false);
    worklist.push(0);
    queued[0] = true;

    while (!worklist.empty()) {
        unsigned long id = worklist.front();
        worklist.pop();
        queued[id] = false;

        AnalysisState out = in_states[id];
        for (const auto& instr : cfg.get_blocks()[id].get_instructions()) {
            analyze_instruction(*instr, out);
        }

        AnalysisState next = successor_state(cfg.get_blocks()[id], out);
        for (unsigned long succ : cfg.get_blocks()[id].get_successors()) {
            if (succ >= in_states.size()) {
                continue;
            }
            if (join_into(in_states[succ], next) && !queued[succ]) {
                worklist.push(succ);
                queued[succ] = true;
            }
        }
    }
}

void InfoFlowAnalysis::print_report() const {
    print_report(std::cout, findings);
}

const std::vector<std::string>& InfoFlowAnalysis::get_findings() const {
    return findings;
}

void InfoFlowAnalysis::print_report(std::ostream& out, const std::vector<std::string>& findings) {
    if (findings.empty()) {
        out << "INFO FLOW: no violations found" << std::endl;
        return;
    }

    out << "INFO FLOW: " << findings.size() << " violation(s)" << std::endl;
    for (const auto& finding : findings) {
        out << "  - " << finding << std::endl;
    }
}
