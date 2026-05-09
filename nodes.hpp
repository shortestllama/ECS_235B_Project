#ifndef NODES_HPP
#define NODES_HPP

#include <memory>
#include <vector>
#include <optional>
#include <string>

class INode {
public:
    INode();
    virtual ~INode() = default;
    virtual void print() const = 0;
};

class AllocaNode : public INode {
public:
    AllocaNode(const std::string& dest);
    static std::optional<std::unique_ptr<INode>> parse(const std::string& line);
    std::string get_dest() const { return dest; }
    void print() const override;
private:
    std::string dest;
};

class GEPNode : public INode {
public:
    GEPNode(const std::string& dest, const std::string& src, const std::vector<std::string>& indices);
    static std::optional<std::unique_ptr<INode>> parse(const std::string& line);
    std::string get_dest() const { return dest; }
    void print() const override;
private:
    std::string dest;
    std::string src;
    std::vector<std::string> indices;
};

class LoadNode : public INode {
public:
    LoadNode(const std::string& dest, const std::string& src);
    static std::optional<std::unique_ptr<INode>> parse(const std::string& line);
    std::string get_dest() const { return dest; }
    void print() const override;
private:
    std::string dest;
    std::string src;
};

class StoreNode : public INode {
public:
    StoreNode(const std::string& src, const std::string& dest);
    static std::optional<std::unique_ptr<INode>> parse(const std::string& line);
    void print() const override;
private:
    std::string src;
    std::string dest;
};

class RetNode : public INode {
public:
    RetNode(const std::string& src);
    static std::optional<std::unique_ptr<INode>> parse(const std::string& line);
    void print() const override;
private:
    std::string src;
};

class CmpNode : public INode {
public:
    CmpNode(const std::string& dest, const std::string& op, const std::string& src1, const std::string& src2);
    static std::optional<std::unique_ptr<INode>> parse(const std::string& line);
    std::string get_dest() const { return dest; }
    void print() const override;
private:
    std::string dest;
    std::string op;
    std::string src1;
    std::string src2;
};

class BrNode : public INode {
public:
    BrNode(const std::string& condition, const std::string& true_label, const std::string& false_label);
    static std::optional<std::unique_ptr<INode>> parse(const std::string& line);
    std::string get_true_label() const { return true_label; }
    std::string get_false_label() const { return false_label; }
    void print() const override;
private:
    std::string condition;
    std::string true_label;
    std::string false_label;
};

class AddNode : public INode {
public:
    AddNode(const std::string& dest, const std::string& src1, const std::string& src2);
    static std::optional<std::unique_ptr<INode>> parse(const std::string& line);
    std::string get_dest() const { return dest; }
    void print() const override;
private:
    std::string dest;
    std::string src1;
    std::string src2;
};

class SubNode : public INode {
public:
    SubNode(const std::string& dest, const std::string& src1, const std::string& src2);
    static std::optional<std::unique_ptr<INode>> parse(const std::string& line);
    std::string get_dest() const { return dest; }
    void print() const override;
private:
    std::string dest;
    std::string src1;
    std::string src2;
};

class MulNode : public INode {
public:
    MulNode(const std::string& dest, const std::string& src1, const std::string& src2);
    static std::optional<std::unique_ptr<INode>> parse(const std::string& line);
    std::string get_dest() const { return dest; }
    void print() const override;
private:
    std::string dest;
    std::string src1;
    std::string src2;
};

class DivNode : public INode {
public:
    DivNode(const std::string& dest, const std::string& src1, const std::string& src2);
    static std::optional<std::unique_ptr<INode>> parse(const std::string& line);
    std::string get_dest() const { return dest; }
    void print() const override;
private:
    std::string dest;
    std::string src1;
    std::string src2;
};

class PhiNode : public INode {
public:
    PhiNode(const std::string& dest, const std::vector<std::pair<std::string, std::string>>& incoming);
    static std::optional<std::unique_ptr<INode>> parse(const std::string& line);
    std::string get_dest() const { return dest; }
    void print() const override;
private:
    std::string dest;
    std::vector<std::pair<std::string, std::string>> incoming; // pair of (value, label)
};

class CallNode : public INode {
public:
    CallNode(const std::string& dest, const std::string& fn_name, const std::vector<std::string>& args);
    static std::optional<std::unique_ptr<INode>> parse(const std::string& line);
    std::string get_dest() const { return dest; }
    void print() const override;
private:
    std::string dest;
    std::string fn_name;
    std::vector<std::string> args;
};

class LabelNode : public INode {
public:
    LabelNode(const std::string& label);
    static std::optional<std::unique_ptr<INode>> parse(const std::string& line);
    std::string get_label() const { return label; }
    void print() const override;
private:
    std::string label;
};

#endif // NODES_HPP
