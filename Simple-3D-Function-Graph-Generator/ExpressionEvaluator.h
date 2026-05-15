// ExpressionEvaluator.h
#ifndef EXPRESSION_EVALUATOR_H
#define EXPRESSION_EVALUATOR_H

#include <string>
#include <vector>
#include <cctype>
#include <cmath>
#include <memory>
#include <map>
#include <stdexcept>
#include <algorithm>

// Token 类型
enum class TokenType {
    NUMBER,
    VARIABLE,
    FUNCTION,
    OPERATOR,
    LPAREN,
    RPAREN,
    END
};

// Token 结构
struct Token {
    TokenType type;
    double value;       // 用于 NUMBER
    std::string name;   // 用于 VARIABLE 或 FUNCTION
    char op;            // 用于 OPERATOR

    Token(TokenType t) : type(t), value(0.0), op(0) {}
    Token(double v) : type(TokenType::NUMBER), value(v), op(0) {}
    Token(TokenType t, const std::string& n) : type(t), name(n), value(0.0), op(0) {}
    Token(char o) : type(TokenType::OPERATOR), op(o), value(0.0) {}
};

// 词法分析器
class Lexer {
public:
    Lexer(const std::string& expr) : expr(expr), pos(0) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (pos < expr.length()) {
            char ch = expr[pos];
            if (isspace(ch)) {
                ++pos;
                continue;
            }
            if (isdigit(ch) || ch == '.') {
                tokens.push_back(parseNumber());
            }
            else if (isalpha(ch)) {
                tokens.push_back(parseIdentifier());
            }
            else if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^') {
                tokens.push_back(Token(ch));
                ++pos;
            }
            else if (ch == '(') {
                tokens.push_back(Token(TokenType::LPAREN));
                ++pos;
            }
            else if (ch == ')') {
                tokens.push_back(Token(TokenType::RPAREN));
                ++pos;
            }
            else {
                throw std::runtime_error("未知字符: " + std::string(1, ch));
            }
        }
        tokens.push_back(Token(TokenType::END));
        return tokens;
    }

private:
    std::string expr;
    size_t pos;

    Token parseNumber() {
        size_t start = pos;
        bool hasDot = false;
        while (pos < expr.length() && (isdigit(expr[pos]) || expr[pos] == '.')) {
            if (expr[pos] == '.') {
                if (hasDot) throw std::runtime_error("数字格式错误: 多个小数点");
                hasDot = true;
            }
            ++pos;
        }
        double val = std::stod(expr.substr(start, pos - start));
        return Token(val);
    }

    Token parseIdentifier() {
        size_t start = pos;
        while (pos < expr.length() && isalnum(expr[pos])) {
            ++pos;
        }
        std::string name = expr.substr(start, pos - start);
        // 已知函数列表
        static const std::vector<std::string> functions = {
            "sin", "cos", "tan", "asin", "acos", "atan",
            "sqrt", "ln", "log10", "exp", "abs"
        };
        if (std::find(functions.begin(), functions.end(), name) != functions.end()) {
            return Token(TokenType::FUNCTION, name);
        }
        else if (name == "x" || name == "y") {
            return Token(TokenType::VARIABLE, name);
        }
        else {
            throw std::runtime_error("未知标识符: " + name);
        }
    }
};

// 抽象语法树节点
class ExprNode {
public:
    virtual ~ExprNode() = default;
    virtual double evaluate(const std::map<std::string, double>& vars) const = 0;
};

class NumberNode : public ExprNode {
    double value;
public:
    NumberNode(double val) : value(val) {}
    double evaluate(const std::map<std::string, double>&) const override {
        return value;
    }
};

class VariableNode : public ExprNode {
    std::string name;
public:
    VariableNode(const std::string& n) : name(n) {}
    double evaluate(const std::map<std::string, double>& vars) const override {
        auto it = vars.find(name);
        if (it == vars.end())
            throw std::runtime_error("变量未定义: " + name);
        return it->second;
    }
};

class BinaryOpNode : public ExprNode {
    char op;
    std::unique_ptr<ExprNode> left;
    std::unique_ptr<ExprNode> right;
public:
    BinaryOpNode(char o, std::unique_ptr<ExprNode> l, std::unique_ptr<ExprNode> r)
        : op(o), left(std::move(l)), right(std::move(r)) {
    }
    double evaluate(const std::map<std::string, double>& vars) const override {
        double lv = left->evaluate(vars);
        double rv = right->evaluate(vars);
        switch (op) {
        case '+': return lv + rv;
        case '-': return lv - rv;
        case '*': return lv * rv;
        case '/':
            if (rv == 0) throw std::runtime_error("除零错误");
            return lv / rv;
        case '^': return std::pow(lv, rv);
        default: throw std::runtime_error("未知运算符");
        }
    }
};

class UnaryNode : public ExprNode {
    char op; // 仅支持 '-'
    std::unique_ptr<ExprNode> child;
public:
    UnaryNode(char o, std::unique_ptr<ExprNode> c) : op(o), child(std::move(c)) {}
    double evaluate(const std::map<std::string, double>& vars) const override {
        double val = child->evaluate(vars);
        if (op == '-') return -val;
        throw std::runtime_error("未知一元运算符");
    }
};

class FunctionNode : public ExprNode {
    std::string funcName;
    std::unique_ptr<ExprNode> arg;
public:
    FunctionNode(const std::string& name, std::unique_ptr<ExprNode> a)
        : funcName(name), arg(std::move(a)) {
    }
    double evaluate(const std::map<std::string, double>& vars) const override {
        double val = arg->evaluate(vars);
        if (funcName == "sin") return std::sin(val);
        if (funcName == "cos") return std::cos(val);
        if (funcName == "tan") return std::tan(val);
        if (funcName == "asin") return std::asin(val);
        if (funcName == "acos") return std::acos(val);
        if (funcName == "atan") return std::atan(val);
        if (funcName == "sqrt") return std::sqrt(val);
        if (funcName == "ln") return std::log(val);
        if (funcName == "log10") return std::log10(val);
        if (funcName == "exp") return std::exp(val);
        if (funcName == "abs") return std::fabs(val);
        throw std::runtime_error("未知函数: " + funcName);
    }
};

// 递归下降解析器
class Parser {
public:
    Parser(const std::vector<Token>& tokens) : tokens(tokens), index(0) {}

    std::unique_ptr<ExprNode> parse() {
        return parseExpression();
    }

private:
    const std::vector<Token>& tokens;
    size_t index;

    Token current() const {
        if (index < tokens.size()) return tokens[index];
        return Token(TokenType::END);
    }

    void consume(TokenType expected) {
        if (current().type == expected) {
            ++index;
        }
        else {
            throw std::runtime_error("语法错误: 期望 " + tokenTypeToString(expected));
        }
    }

    std::string tokenTypeToString(TokenType type) const {
        switch (type) {
        case TokenType::NUMBER: return "数字";
        case TokenType::VARIABLE: return "变量";
        case TokenType::FUNCTION: return "函数";
        case TokenType::OPERATOR: return "运算符";
        case TokenType::LPAREN: return "'('";
        case TokenType::RPAREN: return "')'";
        case TokenType::END: return "结束";
        default: return "未知";
        }
    }

    std::unique_ptr<ExprNode> parseExpression() {
        auto node = parseTerm();
        while (current().type == TokenType::OPERATOR && (current().op == '+' || current().op == '-')) {
            char op = current().op;
            ++index;
            auto right = parseTerm();
            node = std::make_unique<BinaryOpNode>(op, std::move(node), std::move(right));
        }
        return node;
    }

    std::unique_ptr<ExprNode> parseTerm() {
        auto node = parsePower();
        while (current().type == TokenType::OPERATOR && (current().op == '*' || current().op == '/')) {
            char op = current().op;
            ++index;
            auto right = parsePower();
            node = std::make_unique<BinaryOpNode>(op, std::move(node), std::move(right));
        }
        return node;
    }

    std::unique_ptr<ExprNode> parsePower() {
        auto node = parseUnary();
        if (current().type == TokenType::OPERATOR && current().op == '^') {
            ++index;
            auto right = parsePower();
            node = std::make_unique<BinaryOpNode>('^', std::move(node), std::move(right));
        }
        return node;
    }

    std::unique_ptr<ExprNode> parseUnary() {
        if (current().type == TokenType::OPERATOR && current().op == '-') {
            ++index;
            auto node = parsePrimary();
            return std::make_unique<UnaryNode>('-', std::move(node));
        }
        return parsePrimary();
    }

    std::unique_ptr<ExprNode> parsePrimary() {
        if (current().type == TokenType::NUMBER) {
            double val = current().value;
            ++index;
            return std::make_unique<NumberNode>(val);
        }
        else if (current().type == TokenType::VARIABLE) {
            std::string name = current().name;
            ++index;
            return std::make_unique<VariableNode>(name);
        }
        else if (current().type == TokenType::FUNCTION) {
            std::string funcName = current().name;
            ++index;
            if (current().type != TokenType::LPAREN) {
                throw std::runtime_error("函数 '" + funcName + "' 后需要 '('");
            }
            ++index;
            auto arg = parseExpression();
            if (current().type != TokenType::RPAREN) {
                throw std::runtime_error("缺少 ')'");
            }
            ++index;
            return std::make_unique<FunctionNode>(funcName, std::move(arg));
        }
        else if (current().type == TokenType::LPAREN) {
            ++index;
            auto node = parseExpression();
            if (current().type != TokenType::RPAREN) {
                throw std::runtime_error("缺少 ')'");
            }
            ++index;
            return node;
        }
        else {
            throw std::runtime_error("语法错误: 意外的 token");
        }
    }
};

// 表达式求值器封装（对外接口）
class Expression {
public:
    Expression(const std::string& expr) {
        Lexer lexer(expr);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        ast = parser.parse();
    }

    double evaluate(double x, double y) const {
        std::map<std::string, double> vars = { {"x", x}, {"y", y} };
        return ast->evaluate(vars);
    }

private:
    std::unique_ptr<ExprNode> ast;
};

#endif // EXPRESSION_EVALUATOR_H