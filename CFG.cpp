#include <iostream>
#include <memory>
#include <vector>

#include "function.hpp"
#include "CFG.hpp"
#include "nodes.hpp"

CFG::CFG(std::vector<BasicBlock> blocks, const std::vector<std::string>& variables)
    : blocks(std::move(blocks)), variables(variables) {
}

CFG CFG::build(const Function& fn) {
    CFG cfg{{}, {}};
    auto current_block = std::make_unique<BasicBlock>(cfg.get_blocks().size(), std::vector<std::unique_ptr<INode>>{}, std::vector<unsigned long>{});

    for (const auto& line : fn.get_body()) {
        bool valid = false;

        if (auto node = LabelNode::parse(line)) {
            if (!current_block->get_successors().empty() || !current_block->get_instructions().empty()) {
                cfg.add_block(std::move(*current_block));
                current_block = std::make_unique<BasicBlock>(cfg.get_blocks().size(), std::vector<std::unique_ptr<INode>>{}, std::vector<unsigned long>{});
            }
            current_block->set_leader(*static_cast<LabelNode*>(node->get()));
            current_block->add_instruction(std::move(*node));
            valid = true;
        }

        else {
            if (auto node = AllocaNode::parse(line)) {
                cfg.add_variable(static_cast<AllocaNode*>(node->get())->get_dest());
                current_block->add_instruction(std::move(*node));
                valid = true;
            }
            if (auto node = GEPNode::parse(line)) {
                cfg.add_variable(static_cast<GEPNode*>(node->get())->get_dest());
                current_block->add_instruction(std::move(*node));
                valid = true;
            }
            if (auto node = LoadNode::parse(line)) {
                cfg.add_variable(static_cast<LoadNode*>(node->get())->get_dest());
                current_block->add_instruction(std::move(*node));
                valid = true;
            }
            if (auto node = StoreNode::parse(line)) {
                current_block->add_instruction(std::move(*node));
                valid = true;
            }
            if (auto node = RetNode::parse(line)) {
                current_block->add_instruction(std::move(*node));
                valid = true;
            }
            if (auto node = CmpNode::parse(line)) {
                cfg.add_variable(static_cast<CmpNode*>(node->get())->get_dest());
                current_block->add_instruction(std::move(*node));
                valid = true;
            }
            if (auto node = BrNode::parse(line)) {
                current_block->set_term(*static_cast<BrNode*>(node->get()));
                current_block->add_instruction(std::move(*node));
                valid = true;
            }
            if (auto node = AddNode::parse(line)) {
                cfg.add_variable(static_cast<AddNode*>(node->get())->get_dest());
                current_block->add_instruction(std::move(*node));
                valid = true;
            }
            if (auto node = SubNode::parse(line)) {
                cfg.add_variable(static_cast<SubNode*>(node->get())->get_dest());
                current_block->add_instruction(std::move(*node));
                valid = true;
            }
            if (auto node = MulNode::parse(line)) {
                cfg.add_variable(static_cast<MulNode*>(node->get())->get_dest());
                current_block->add_instruction(std::move(*node));
                valid = true;
            }
            if (auto node = DivNode::parse(line)) {
                cfg.add_variable(static_cast<DivNode*>(node->get())->get_dest());
                current_block->add_instruction(std::move(*node));
                valid = true;
            }
            if (auto node = PhiNode::parse(line)) {
                cfg.add_variable(static_cast<PhiNode*>(node->get())->get_dest());
                current_block->add_instruction(std::move(*node));
                valid = true;
            }
            if (auto node = CallNode::parse(line)) {
                if (!static_cast<CallNode*>(node->get())->get_dest().empty()) {
                    cfg.add_variable(static_cast<CallNode*>(node->get())->get_dest());
                }
                current_block->add_instruction(std::move(*node));
                valid = true;
            }
        }

        if (!valid) {
            std::cerr << "Unrecognized instruction: " << line << std::endl;
        }
    }

    cfg.add_block(std::move(*current_block));
    cfg.set_successors();
    return cfg;
}

void CFG::add_block(BasicBlock&& block) {
    blocks.push_back(std::move(block));
}

void CFG::add_variable(const std::string& var) {
    variables.push_back(var);
}

void CFG::set_successors() {
    for (auto& block : blocks) {
        if (auto node = block.get_term()) {
            if (auto true_label = node->get_true_label(); !true_label.empty()) {
                if (auto new_id = get_id_by_leader(true_label)) {
                    block.add_successor(*new_id);
                }
            }

            if (auto false_label = node->get_false_label(); !false_label.empty()) {
                if (auto new_id = get_id_by_leader(false_label)) {
                    block.add_successor(*new_id);
                }
            }
        }
    }
}

std::optional<unsigned long> CFG::get_id_by_leader(const std::string& label) const {
    for (const auto& block : blocks) {
        if (block.get_instructions().empty()) {
            continue;
        }

        if (auto node = dynamic_cast<LabelNode*>(block.get_instructions().front().get())) {
            if (node->get_label() == label) {
                return block.get_id();
            }
        }
    }

    return {};
}

void CFG::print() const {
    std::cout << "--------------" << std::endl;
    std::cout << "SSAs: [";
    for (const auto& var : variables) {
        std::cout << var;
        if (&var != &variables.back()) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;
    for (const auto& block : blocks) {
        block.print();
        std::cout << "--------------" << std::endl;
        std::cout << "Successors: [";
        for (const auto& succ : block.get_successors()) {
            std::cout << succ;
            if (&succ != &block.get_successors().back()) {
                std::cout << ", ";
            }
        }
        std::cout << "]" << std::endl;
    }
}
