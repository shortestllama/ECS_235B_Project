#ifndef BASIC_BLOCK_HPP
#define BASIC_BLOCK_HPP

#include <memory>
#include <vector>
#include <string>

#include "nodes.hpp"

class BasicBlock {
public:
    BasicBlock(unsigned long id, std::vector<std::unique_ptr<INode>> instructions, std::vector<unsigned long> successors);
    void add_instruction(std::unique_ptr<INode> instr);
    void add_successor(unsigned long succ);
    void set_leader(const LabelNode& label);
    void set_term(const BrNode& term);
    unsigned long get_id() const { return id; }
    const std::vector<std::unique_ptr<INode>>& get_instructions() const { return instructions; }
    const std::vector<unsigned long>& get_successors() const { return successors; }
    const std::optional<BrNode>& get_term() const { return term; }
    void print() const;
private:
    unsigned long id;
    std::vector<std::unique_ptr<INode>> instructions;
    std::vector<unsigned long> successors;
    std::optional<LabelNode> leader;
    std::optional<BrNode> term;
};

#endif // BASIC_BLOCK_HPP
