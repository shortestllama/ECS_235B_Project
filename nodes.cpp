#include <iostream>
#include <regex>
#include <sstream>

#include "nodes.hpp"

namespace {

std::vector<std::string> extract_typed_operands(const std::string& text) {
    std::vector<std::string> values;
    std::regex value_re(R"([%@][A-Za-z0-9_.$-]+|-?\d+)");
    std::stringstream stream(text);
    std::string part;

    while (std::getline(stream, part, ',')) {
        std::string value;
        auto begin = std::sregex_iterator(part.begin(), part.end(), value_re);
        auto end = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) {
            value = (*it).str();
        }
        if (!value.empty()) {
            values.push_back(value);
        }
    }

    return values;
}

const char* value_token = R"([%@]?[\w.$+-]+|-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)";

std::string capture_value() {
    return "(" + std::string(value_token) + ")";
}

} // namespace

INode::INode() {
}

AllocaNode::AllocaNode(const std::string& dest) : dest(dest) {
}

std::optional<std::unique_ptr<INode>> AllocaNode::parse(const std::string& line) {
    std::regex re(R"(^(%[\w.]+)\s*=\s*alloca\s+.+?(?:\s*,.*)?$)");
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
    std::regex re(R"(^(%[\w.]+)\s*=\s*getelementptr(?:\s+inbounds)?\s+.+,\s*(?:ptr|[^,]+\*)\s+([%@][\w.$-]+)(?:\s*,\s*(.*))?$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        std::string dest = match[1].str();
        std::string src = match[2].str();
        std::vector<std::string> indices;
        std::string indices_str = match[3].str();
        indices = extract_typed_operands(indices_str);
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
    std::regex re(R"(^(%[\w.]+)\s*=\s*load\s+.+,\s*ptr\s+([%@][\w.$-]+)(?:\s*,.*)?$)");
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
    std::regex re(R"(^store\s+.+?\s+([%@]?[\w.$-]+|-?\d+)\s*,\s*ptr\s+([%@][\w.$-]+)(?:\s*,.*)?$)");
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
    std::regex re(R"(^ret\s+(?:void|.+?\s+([%@]?[\w.$-]+|-?\d+))(?:\s*,.*)?$)");
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
    std::regex re(std::string(R"(^(%[\w.]+)\s*=\s*(?:icmp|fcmp)\s+([\w]+)\s+.+?\s+)") + capture_value() + R"(\s*,\s*)" + capture_value() + R"((?:\s*,.*)?$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<CmpNode>(match[1].str(), match[2].str(), match[3].str(), match[4].str()));
    }
    return std::nullopt;
}

void CmpNode::print() const {
    std::cout << "CmpNode { dest: " << dest << ", op: " << op << ", src1: " << src1 << ", src2: " << src2 << " }" << std::endl;
}

CastNode::CastNode(const std::string& dest, const std::string& op, const std::string& src)
    : dest(dest), op(op), src(src) {
}

std::optional<std::unique_ptr<INode>> CastNode::parse(const std::string& line) {
    std::regex re(std::string(R"(^(%[\w.]+)\s*=\s*(zext|sext|trunc|bitcast|ptrtoint|inttoptr|fptrunc|fpext|fptoui|fptosi|uitofp|sitofp)\s+.+?\s+)") + capture_value() + R"(\s+to\s+.+$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<CastNode>(match[1].str(), match[2].str(), match[3].str()));
    }
    return std::nullopt;
}

void CastNode::print() const {
    std::cout << "CastNode { dest: " << dest << ", op: " << op << ", src: " << src << " }" << std::endl;
}

SelectNode::SelectNode(const std::string& dest, const std::string& condition, const std::string& true_value, const std::string& false_value)
    : dest(dest), condition(condition), true_value(true_value), false_value(false_value) {
}

std::optional<std::unique_ptr<INode>> SelectNode::parse(const std::string& line) {
    std::regex re(std::string(R"(^(%[\w.]+)\s*=\s*select\s+.+?\s+)") + capture_value() + R"(\s*,\s+.+?\s+)" + capture_value() + R"(\s*,\s+.+?\s+)" + capture_value() + R"((?:\s*,.*)?$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<SelectNode>(match[1].str(), match[2].str(), match[3].str(), match[4].str()));
    }
    return std::nullopt;
}

void SelectNode::print() const {
    std::cout << "SelectNode { dest: " << dest << ", condition: " << condition
              << ", true: " << true_value << ", false: " << false_value << " }" << std::endl;
}

BrNode::BrNode(const std::string& condition, const std::string& true_label, const std::string& false_label)
    : condition(condition), true_label(true_label), false_label(false_label) {
}

std::optional<std::unique_ptr<INode>> BrNode::parse(const std::string& line) {
    std::regex re(R"(^br\s+(?:.+?\s+([%@]?[\w.$-]+|-?\d+)\s*,\s*label\s+%([\w.]+)\s*,\s*)?label\s+%([\w.]+)$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<BrNode>(match[1].str(), match[2].str(), match[3].str()));
    }
    return std::nullopt;
}

void BrNode::print() const {
    std::cout << "BrNode { condition: " << condition << ", true_label: " << true_label << ", false_label: " << false_label << " }" << std::endl;
}

SwitchNode::SwitchNode(const std::string& condition, const std::string& default_label, const std::vector<std::string>& case_labels)
    : condition(condition), default_label(default_label), case_labels(case_labels) {
}

std::optional<std::unique_ptr<INode>> SwitchNode::parse(const std::string& line) {
    std::regex re(std::string(R"(^switch\s+.+?\s+)") + capture_value() + R"(\s*,\s*label\s+%([\w.]+)\s*\[(.*)\]$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        std::string cases = match[3].str();
        std::vector<std::string> labels;
        std::regex label_re(R"(label\s+%([\w.]+))");
        auto begin = std::sregex_iterator(cases.begin(), cases.end(), label_re);
        auto end = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) {
            labels.push_back((*it)[1].str());
        }
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<SwitchNode>(match[1].str(), match[2].str(), labels));
    }
    return std::nullopt;
}

void SwitchNode::print() const {
    std::cout << "SwitchNode { condition: " << condition << ", default_label: " << default_label << ", case_labels: [";
    for (const auto& label : case_labels) {
        std::cout << label;
        if (&label != &case_labels.back()) {
            std::cout << ", ";
        }
    }
    std::cout << "] }" << std::endl;
}

AddNode::AddNode(const std::string& dest, const std::string& src1, const std::string& src2)
    : dest(dest), src1(src1), src2(src2) {
}

std::optional<std::unique_ptr<INode>> AddNode::parse(const std::string& line) {
    std::regex re(std::string(R"(^(%[\w.]+)\s*=\s*add(?:\s+\w+)*\s+.+?\s+)") + capture_value() + R"(\s*,\s*)" + capture_value() + R"((?:\s*,.*)?$)");
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
    std::regex re(std::string(R"(^(%[\w.]+)\s*=\s*sub(?:\s+\w+)*\s+.+?\s+)") + capture_value() + R"(\s*,\s*)" + capture_value() + R"((?:\s*,.*)?$)");
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
    std::regex re(std::string(R"(^(%[\w.]+)\s*=\s*mul(?:\s+\w+)*\s+.+?\s+)") + capture_value() + R"(\s*,\s*)" + capture_value() + R"((?:\s*,.*)?$)");
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
    std::regex re(std::string(R"(^(%[\w.]+)\s*=\s*(?:[suf]?div)(?:\s+\w+)*\s+.+?\s+)") + capture_value() + R"(\s*,\s*)" + capture_value() + R"((?:\s*,.*)?$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<DivNode>(match[1].str(), match[2].str(), match[3].str()));
    }
    return std::nullopt;
}

void DivNode::print() const {
    std::cout << "DivNode { dest: " << dest << ", src1: " << src1 << ", src2: " << src2 << " }" << std::endl;
}

BinaryOpNode::BinaryOpNode(const std::string& dest, const std::string& op, const std::string& src1, const std::string& src2)
    : dest(dest), op(op), src1(src1), src2(src2) {
}

std::optional<std::unique_ptr<INode>> BinaryOpNode::parse(const std::string& line) {
    std::regex re(std::string(R"(^(%[\w.]+)\s*=\s*(and|or|xor|shl|lshr|ashr|srem|urem|fadd|fsub|fmul|frem)\s+(?:\w+\s+)*.+?\s+)") + capture_value() + R"(\s*,\s*)" + capture_value() + R"((?:\s*,.*)?$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<BinaryOpNode>(match[1].str(), match[2].str(), match[3].str(), match[4].str()));
    }
    return std::nullopt;
}

void BinaryOpNode::print() const {
    std::cout << "BinaryOpNode { dest: " << dest << ", op: " << op << ", src1: " << src1 << ", src2: " << src2 << " }" << std::endl;
}

PhiNode::PhiNode(const std::string& dest, const std::vector<std::pair<std::string, std::string>>& incoming)
    : dest(dest), incoming(incoming) {
}

std::optional<std::unique_ptr<INode>> PhiNode::parse(const std::string& line) {
    std::regex re(R"(^(%[\w.]+)\s*=\s*phi\s+.+?\s+(.+)$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        std::string dest = match[1].str();
        std::vector<std::pair<std::string, std::string>> incoming;
        std::string incoming_str = match[2].str();
        std::regex pair_re(R"(\[\s*([%@]?[\w.$-]+|-?\d+)\s*,\s*%([\w.]+)\s*\])");
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
    std::regex re(R"(^(?:(%[\w.]+)\s*=\s*)?\s*call\s+.*?@([\w.$]+)\s*\((.*)\)(?:\s*#[0-9]+)?$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        std::string dest = match[1].str();
        std::string fn_name = match[2].str();
        std::string args_str = match[3].str();
        std::vector<std::string> args = extract_typed_operands(args_str);
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
    std::regex re(R"(^([\w.]+):$)");
    std::smatch match;
    if (std::regex_search(line, match, re)) {
        return std::make_optional<std::unique_ptr<INode>>(std::make_unique<LabelNode>(match[1].str()));
    }
    return std::nullopt;
}

void LabelNode::print() const {
    std::cout << "LabelNode { label: " << label << " }" << std::endl;
}
