#include <iostream>
#include <regex>

#include "nodes.hpp"

INode::INode() {
}

AllocaNode::AllocaNode(const std::string& dest) : dest(dest) {
}

std::optional<std::unique_ptr<INode>> AllocaNode::parse(const std::string& line) {
    std::regex re(R"(^(%[\w]+)\s*=\s*alloca\s+[\w\*]+$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<AllocaNode>(match[1].str()));
    }
    return std::nullopt;
}

void AllocaNode::print() const {
    std::cout << "AllocaNode { dest: " << dest << " }" << std::endl;
}

GEPNode::GEPNode(const std::string& dest, const std::string& src, const std::vector<std::string>& indices)
    : dest(dest), src(src), indices(indices) {
}

std::optional<std::unique_ptr<INode>> GEPNode::parse(const std::string& line) {
    std::regex re(R"(^(%[\w]+)\s*=\s*getelementptr\s+[\w\*]+\s*,\s*[\w\*]+\s+(%[\w]+)\s*,\s*[\w\*]+\s+((?:%)?[\w]+)(?:\s*,\s*[\w\*]+\s+(?:%)?[\w]+)*$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        std::string dest = match[1].str();
        std::string src = match[2].str();
        std::vector<std::string> indices;
        std::string indices_str = match[3].str();
        std::regex index_re(R"((?:%?[\w]+))");
        auto indices_begin = std::sregex_iterator(indices_str.begin(), indices_str.end(), index_re);
        auto indices_end = std::sregex_iterator();
        for (std::sregex_iterator i = indices_begin; i != indices_end; ++i) {
            indices.push_back((*i).str());
        }
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<GEPNode>(dest, src, indices));
    }
    return std::nullopt;
}

void GEPNode::print() const {
    std::cout << "GEPNode { dest: " << dest << ", src: " << src << ", indices: [";
    for (const auto& index : indices) {
        std::cout << index;
        if (&index != &indices.back()) {
            std::cout << ", ";
        }
    }
    std::cout << "] }" << std::endl;
}

LoadNode::LoadNode(const std::string& dest, const std::string& src) : dest(dest), src(src) {
}

std::optional<std::unique_ptr<INode>> LoadNode::parse(const std::string& line) {
    std::regex re(R"(^(%[\w]+)\s*=\s*load\s+[\w\*]+\s*,\s*ptr\s+(%[\w]+)$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<LoadNode>(match[1].str(), match[2].str()));
    }
    return std::nullopt;
}

void LoadNode::print() const {
    std::cout << "LoadNode { dest: " << dest << ", src: " << src << " }" << std::endl;
}

StoreNode::StoreNode(const std::string& src, const std::string& dest) : src(src), dest(dest) {
}

std::optional<std::unique_ptr<INode>> StoreNode::parse(const std::string& line) {
    std::regex re(R"(^store\s+[\w\*]+\s+((?:%)?[\w]+)\s*,\s*ptr\s+(%[\w]+)$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<StoreNode>(match[1].str(), match[2].str()));
    }
    return std::nullopt;
}

void StoreNode::print() const {
    std::cout << "StoreNode { dest: " << dest << ", src: " << src << " }" << std::endl;
}

RetNode::RetNode(const std::string& src) : src(src) {
}

std::optional<std::unique_ptr<INode>> RetNode::parse(const std::string& line) {
    std::regex re(R"(^ret\s+[\w\*]+\s+((?:%)?[\w]+)$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<RetNode>(match[1].str()));
    }
    return std::nullopt;
}

void RetNode::print() const {
    std::cout << "RetNode" << std::endl;
}

CmpNode::CmpNode(const std::string& dest, const std::string& op, const std::string& src1, const std::string& src2)
    : dest(dest), op(op), src1(src1), src2(src2) {
}

std::optional<std::unique_ptr<INode>> CmpNode::parse(const std::string& line) {
    std::regex re(R"(^(%[\w]+)\s*=\s*icmp\s+([\w]+)\s+[\w\*]+\s+((?:%)?[\w]+)\s*,\s*((?:%)?[\w]+)$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<CmpNode>(match[1].str(), match[2].str(), match[3].str(), match[4].str()));
    }
    return std::nullopt;
}

void CmpNode::print() const {
    std::cout << "CmpNode { dest: " << dest << ", op: " << op << ", src1: " << src1 << ", src2: " << src2 << " }" << std::endl;
}

BrNode::BrNode(const std::string& condition, const std::string& true_label, const std::string& false_label)
    : condition(condition), true_label(true_label), false_label(false_label) {
}

std::optional<std::unique_ptr<INode>> BrNode::parse(const std::string& line) {
    std::regex re(R"(^br\s+(?:[\w\*]+\s+((?:%)?[\w]+)\s*,\s*label\s+%([\w]+)\s*,\s*)?label\s+%([\w]+)$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<BrNode>(match[1].str(), match[2].str(), match[3].str()));
    }
    return std::nullopt;
}

void BrNode::print() const {
    std::cout << "BrNode { condition: " << condition << ", true_label: " << true_label << ", false_label: " << false_label << " }" << std::endl;
}

AddNode::AddNode(const std::string& dest, const std::string& src1, const std::string& src2)
    : dest(dest), src1(src1), src2(src2) {
}

std::optional<std::unique_ptr<INode>> AddNode::parse(const std::string& line) {
    std::regex re(R"(^(%[\w]+)\s*=\s*add\s+[\w\*]+\s+((?:%)?[\w]+)\s*,\s*((?:%)?[\w]+)$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<AddNode>(match[1].str(), match[2].str(), match[3].str()));
    }
    return std::nullopt;
}

void AddNode::print() const {
    std::cout << "AddNode { dest: " << dest << ", src1: " << src1 << ", src2: " << src2 << " }" << std::endl;
}

SubNode::SubNode(const std::string& dest, const std::string& src1, const std::string& src2)
    : dest(dest), src1(src1), src2(src2) {
}

std::optional<std::unique_ptr<INode>> SubNode::parse(const std::string& line) {
    std::regex re(R"(^(%[\w]+)\s*=\s*sub\s+[\w\*]+\s+((?:%)?[\w]+)\s*,\s*((?:%)?[\w]+)$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<SubNode>(match[1].str(), match[2].str(), match[3].str()));
    }
    return std::nullopt;
}

void SubNode::print() const {
    std::cout << "SubNode { dest: " << dest << ", src1: " << src1 << ", src2: " << src2 << " }" << std::endl;
}

MulNode::MulNode(const std::string& dest, const std::string& src1, const std::string& src2)
    : dest(dest), src1(src1), src2(src2) {
}

std::optional<std::unique_ptr<INode>> MulNode::parse(const std::string& line) {
    std::regex re(R"(^(%[\w]+)\s*=\s*mul\s+[\w\*]+\s+((?:%)?[\w]+)\s*,\s*((?:%)?[\w]+)$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<MulNode>(match[1].str(), match[2].str(), match[3].str()));
    }
    return std::nullopt;
}

void MulNode::print() const {
    std::cout << "MulNode { dest: " << dest << ", src1: " << src1 << ", src2: " << src2 << " }" << std::endl;
}

DivNode::DivNode(const std::string& dest, const std::string& src1, const std::string& src2)
    : dest(dest), src1(src1), src2(src2) {
}

std::optional<std::unique_ptr<INode>> DivNode::parse(const std::string& line) {
    std::regex re(R"(^(%[\w]+)\s*=\s*div\s+[\w\*]+\s+((?:%)?[\w]+)\s*,\s*((?:%)?[\w]+)$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<DivNode>(match[1].str(), match[2].str(), match[3].str()));
    }
    return std::nullopt;
}

void DivNode::print() const {
    std::cout << "DivNode { dest: " << dest << ", src1: " << src1 << ", src2: " << src2 << " }" << std::endl;
}

PhiNode::PhiNode(const std::string& dest, const std::vector<std::pair<std::string, std::string>>& incoming)
    : dest(dest), incoming(incoming) {
}

std::optional<std::unique_ptr<INode>> PhiNode::parse(const std::string& line) {
    std::regex re(R"(^(%[\w]+)\s*=\s*phi\s+[\w]+\s*(\[\s*(%[\w]+)\s*,\s*%([\w]+)\s*\]\s*(?:,\s*\[(%[\w]+),\s+%([\w]+)\])*)$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        std::string dest = match[1].str();
        std::vector<std::pair<std::string, std::string>> incoming;
        std::string incoming_str = match[2].str();
        std::regex pair_re(R"(\[\s*(%[\w]+)\s*,\s*(%[\w]+)\s*\])");
        auto pairs_begin = std::sregex_iterator(incoming_str.begin(), incoming_str.end(), pair_re);
        auto pairs_end = std::sregex_iterator();
        for (std::sregex_iterator i = pairs_begin; i != pairs_end; ++i) {
            incoming.emplace_back((*i)[1].str(), (*i)[2].str());
        }
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<PhiNode>(dest, incoming));
    }
    return std::nullopt;
}

void PhiNode::print() const {
    std::cout << "PhiNode { dest: " << dest << ", incoming: [";
    for (const auto& pair : incoming) {
        std::cout << "[" << pair.first << ", " << pair.second << "]";
        if (&pair != &incoming.back()) {
            std::cout << ", ";
        }
    }
    std::cout << "] }" << std::endl;
}

CallNode::CallNode(const std::string& dest, const std::string& fn_name, const std::vector<std::string>& args)
    : dest(dest), fn_name(fn_name), args(args) {
}

std::optional<std::unique_ptr<INode>> CallNode::parse(const std::string& line) {
    std::regex re(R"(^(?:(%[\w]+)\s*=\s*)?\s*call\s+[\w\*]+\s*(?:\(\s*(?:\.\.\.|[\w]+(?:,\s*(?:\.\.\.|[\w\*]+))*)?\s*\))?\s+@([\w]+)\s*\(\s*(?:[\w\*]+\s+((?:%)?[\w]+)(?:,\s*[\w\*]+\s+((?:%)?[\w]+))*)?\s*\)$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        std::string dest = match[1].str();
        std::string fn_name = match[2].str();
        std::vector<std::string> args;
        std::string args_str = match[3].str();
        std::regex arg_re(R"((?:%?[\w]+))");
        auto args_begin = std::sregex_iterator(args_str.begin(), args_str.end(), arg_re);
        auto args_end = std::sregex_iterator();
        for (std::sregex_iterator i = args_begin; i != args_end; ++i) {
            args.push_back((*i).str());
        }
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<CallNode>(dest, fn_name, args));
    }
    return std::nullopt;
}

void CallNode::print() const {
    std::cout << "CallNode { name: " << fn_name << ", dest: " << dest << ", args: [";
    for (const auto& arg : args) {
        std::cout << arg;
        if (&arg != &args.back()) {
            std::cout << ", ";
        }
    }
    std::cout << "] }" << std::endl;
}

LabelNode::LabelNode(const std::string& label) : label(label) {
}

std::optional<std::unique_ptr<INode>> LabelNode::parse(const std::string& line) {
    std::regex re(R"(^([\w]+):$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<LabelNode>(match[1].str()));
    }
    return std::nullopt;
}

void LabelNode::print() const {
    std::cout << "LabelNode { label: " << label << " }" << std::endl;
}
