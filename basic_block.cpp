#include <iostream>

#include "basic_block.hpp"
#include "nodes.hpp"

BasicBlock::BasicBlock(unsigned long id, std::vector<std::unique_ptr<INode>> instructions, std::vector<unsigned long> successors)
    : id(id), instructions(std::move(instructions)), successors(successors), leader(std::nullopt), term(std::nullopt) {
}

void BasicBlock::add_instruction(std::unique_ptr<INode> instr) {
    instructions.push_back(std::move(instr));
}

void BasicBlock::add_successor(unsigned long succ) {
    successors.push_back(succ);
}

void BasicBlock::set_leader(const LabelNode& label) {
    this->leader = label;
}

void BasicBlock::set_term(const BrNode& term) {
    this->term = term;
}

void BasicBlock::print() const {
    std::cout << "--------------" << std::endl;
    std::cout << "BASIC BLOCK " << id << std::endl;
    std::cout << "--------------" << std::endl;
    for (const auto& instr : instructions) {
        instr->print();
    }
}
