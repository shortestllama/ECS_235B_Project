#ifndef CFG_HPP
#define CFG_HPP

#include <optional>

#include "basic_block.hpp"

class Function;

class CFG {
public:
    CFG(std::vector<BasicBlock> blocks, const std::vector<std::string>& variables);
    static CFG build(const Function& fn);
    void add_block(BasicBlock&& block);
    void add_variable(const std::string& var);
    void set_successors();
    std::optional<unsigned long> get_id_by_leader(const std::string& label) const;
    const std::vector<BasicBlock>& get_blocks() const { return blocks; }
    void print() const;
private:
    std::vector<BasicBlock> blocks;
    std::vector<std::string> variables;
};

#endif // CFG_HPP
