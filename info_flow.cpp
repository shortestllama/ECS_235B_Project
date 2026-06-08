#include "info_flow.hpp"

#include <algorithm>
#include <iostream>
#include <queue>
#include <sstream>

InfoFlowAnalysis::InfoFlowAnalysis(const CFG& cfg, const SecurityPolicy& policy)
    : cfg(cfg), policy(policy) {
    in_states.resize(cfg.get_blocks().size(), initial_state());
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
    if (findings.empty()) {
        std::cout << "INFO FLOW: no violations found" << std::endl;
        return;
    }

    std::cout << "INFO FLOW: " << findings.size() << " violation(s)" << std::endl;
    for (const auto& finding : findings) {
        std::cout << "  - " << finding << std::endl;
    }
}
