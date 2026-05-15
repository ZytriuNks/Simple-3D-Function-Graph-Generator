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


namespace ExprEval {

    // Token 类型
    enum class ExprEval_TokenType {
        ExprEval_NUMBER,
        ExprEval_VARIABLE,
        ExprEval_FUNCTION,
        ExprEval_OPERATOR,
        ExprEval_LPAREN,
        ExprEval_RPAREN,
        ExprEval_END
    };

    // Token 结构体（成员变量重命名）
    struct ExprEval_Token {
        ExprEval_TokenType tokenType;
        double numericValue;       // 用于 ExprEval_NUMBER
        std::string identifierName; // 用于 ExprEval_VARIABLE 或 ExprEval_FUNCTION
        char operatorChar;         // 用于 ExprEval_OPERATOR

        ExprEval_Token(ExprEval_TokenType t) : tokenType(t), numericValue(0.0), operatorChar(0) {}
        ExprEval_Token(double v) : tokenType(ExprEval_TokenType::ExprEval_NUMBER), numericValue(v), operatorChar(0) {}
        ExprEval_Token(ExprEval_TokenType t, const std::string& n) : tokenType(t), identifierName(n), numericValue(0.0), operatorChar(0) {}
        ExprEval_Token(char o) : tokenType(ExprEval_TokenType::ExprEval_OPERATOR), operatorChar(o), numericValue(0.0) {}
    };

    // 词法分析器
    class ExprEval_Lexer {
    public:
        ExprEval_Lexer(const std::string& expr) : expr(expr), pos(0) {}

        std::vector<ExprEval_Token> tokenize() {
            std::vector<ExprEval_Token> tokens;
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
                    tokens.push_back(ExprEval_Token(ch));
                    ++pos;
                }
                else if (ch == '(') {
                    tokens.push_back(ExprEval_Token(ExprEval_TokenType::ExprEval_LPAREN));
                    ++pos;
                }
                else if (ch == ')') {
                    tokens.push_back(ExprEval_Token(ExprEval_TokenType::ExprEval_RPAREN));
                    ++pos;
                }
                else {
                    throw std::runtime_error("未知字符: " + std::string(1, ch));
                }
            }
            tokens.push_back(ExprEval_Token(ExprEval_TokenType::ExprEval_END));
            return tokens;
        }

    private:
        std::string expr;
        size_t pos;

        ExprEval_Token parseNumber() {
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
            return ExprEval_Token(val);
        }

        ExprEval_Token parseIdentifier() {
            size_t start = pos;
            while (pos < expr.length() && isalnum(expr[pos])) {
                ++pos;
            }
            std::string name = expr.substr(start, pos - start);
            static const std::vector<std::string> functions = {
                "sin", "cos", "tan", "asin", "acos", "atan",
                "sqrt", "ln", "log10", "exp", "abs"
            };
            if (std::find(functions.begin(), functions.end(), name) != functions.end()) {
                return ExprEval_Token(ExprEval_TokenType::ExprEval_FUNCTION, name);
            }
            else if (name == "x" || name == "y") {
                return ExprEval_Token(ExprEval_TokenType::ExprEval_VARIABLE, name);
            }
            else {
                throw std::runtime_error("未知标识符: " + name);
            }
        }
    };

    // 抽象语法树节点
    class ExprEval_Node {
    public:
        virtual ~ExprEval_Node() = default;
        virtual double evaluate(const std::map<std::string, double>& vars) const = 0;
    };

    class ExprEval_NumberNode : public ExprEval_Node {
        double val;
    public:
        ExprEval_NumberNode(double v) : val(v) {}
        double evaluate(const std::map<std::string, double>&) const override {
            return val;
        }
    };

    class ExprEval_VariableNode : public ExprEval_Node {
        std::string varName;
    public:
        ExprEval_VariableNode(const std::string& n) : varName(n) {}
        double evaluate(const std::map<std::string, double>& vars) const override {
            auto it = vars.find(varName);
            if (it == vars.end())
                throw std::runtime_error("变量未定义: " + varName);
            return it->second;
        }
    };

    class ExprEval_BinaryOpNode : public ExprEval_Node {
        char op;
        std::unique_ptr<ExprEval_Node> left;
        std::unique_ptr<ExprEval_Node> right;
    public:
        ExprEval_BinaryOpNode(char o, std::unique_ptr<ExprEval_Node> l, std::unique_ptr<ExprEval_Node> r)
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

    class ExprEval_UnaryNode : public ExprEval_Node {
        char op;
        std::unique_ptr<ExprEval_Node> child;
    public:
        ExprEval_UnaryNode(char o, std::unique_ptr<ExprEval_Node> c) : op(o), child(std::move(c)) {}
        double evaluate(const std::map<std::string, double>& vars) const override {
            double v = child->evaluate(vars);
            if (op == '-') return -v;
            throw std::runtime_error("未知一元运算符");
        }
    };

    class ExprEval_FunctionNode : public ExprEval_Node {
        std::string funcName;
        std::unique_ptr<ExprEval_Node> arg;
    public:
        ExprEval_FunctionNode(const std::string& name, std::unique_ptr<ExprEval_Node> a)
            : funcName(name), arg(std::move(a)) {
        }
        double evaluate(const std::map<std::string, double>& vars) const override {
            double v = arg->evaluate(vars);
            if (funcName == "sin") return std::sin(v);
            if (funcName == "cos") return std::cos(v);
            if (funcName == "tan") return std::tan(v);
            if (funcName == "asin") return std::asin(v);
            if (funcName == "acos") return std::acos(v);
            if (funcName == "atan") return std::atan(v);
            if (funcName == "sqrt") return std::sqrt(v);
            if (funcName == "ln") return std::log(v);
            if (funcName == "log10") return std::log10(v);
            if (funcName == "exp") return std::exp(v);
            if (funcName == "abs") return std::fabs(v);
            throw std::runtime_error("未知函数: " + funcName);
        }
    };

    // 递归下降解析器
    class ExprEval_Parser {
    public:
        ExprEval_Parser(const std::vector<ExprEval_Token>& tokens) : tokens(tokens), index(0) {}

        std::unique_ptr<ExprEval_Node> parse() {
            return parseExpression();
        }

    private:
        const std::vector<ExprEval_Token>& tokens;
        size_t index;

        ExprEval_Token current() const {
            if (index < tokens.size()) return tokens[index];
            return ExprEval_Token(ExprEval_TokenType::ExprEval_END);
        }

        void consume(ExprEval_TokenType expected) {
            if (current().tokenType == expected) {
                ++index;
            }
            else {
                throw std::runtime_error("语法错误: 期望 " + tokenTypeToString(expected));
            }
        }

        std::string tokenTypeToString(ExprEval_TokenType type) const {
            switch (type) {
            case ExprEval_TokenType::ExprEval_NUMBER: return "数字";
            case ExprEval_TokenType::ExprEval_VARIABLE: return "变量";
            case ExprEval_TokenType::ExprEval_FUNCTION: return "函数";
            case ExprEval_TokenType::ExprEval_OPERATOR: return "运算符";
            case ExprEval_TokenType::ExprEval_LPAREN: return "'('";
            case ExprEval_TokenType::ExprEval_RPAREN: return "')'";
            case ExprEval_TokenType::ExprEval_END: return "结束";
            default: return "未知";
            }
        }

        std::unique_ptr<ExprEval_Node> parseExpression() {
            auto node = parseTerm();
            while (current().tokenType == ExprEval_TokenType::ExprEval_OPERATOR &&
                (current().operatorChar == '+' || current().operatorChar == '-')) {
                char op = current().operatorChar;
                ++index;
                auto right = parseTerm();
                node = std::make_unique<ExprEval_BinaryOpNode>(op, std::move(node), std::move(right));
            }
            return node;
        }

        std::unique_ptr<ExprEval_Node> parseTerm() {
            auto node = parsePower();
            while (current().tokenType == ExprEval_TokenType::ExprEval_OPERATOR &&
                (current().operatorChar == '*' || current().operatorChar == '/')) {
                char op = current().operatorChar;
                ++index;
                auto right = parsePower();
                node = std::make_unique<ExprEval_BinaryOpNode>(op, std::move(node), std::move(right));
            }
            return node;
        }

        std::unique_ptr<ExprEval_Node> parsePower() {
            auto node = parseUnary();
            if (current().tokenType == ExprEval_TokenType::ExprEval_OPERATOR && current().operatorChar == '^') {
                ++index;
                auto right = parsePower();
                node = std::make_unique<ExprEval_BinaryOpNode>('^', std::move(node), std::move(right));
            }
            return node;
        }

        std::unique_ptr<ExprEval_Node> parseUnary() {
            if (current().tokenType == ExprEval_TokenType::ExprEval_OPERATOR && current().operatorChar == '-') {
                ++index;
                auto node = parsePrimary();
                return std::make_unique<ExprEval_UnaryNode>('-', std::move(node));
            }
            return parsePrimary();
        }

        std::unique_ptr<ExprEval_Node> parsePrimary() {
            if (current().tokenType == ExprEval_TokenType::ExprEval_NUMBER) {
                double val = current().numericValue;
                ++index;
                return std::make_unique<ExprEval_NumberNode>(val);
            }
            else if (current().tokenType == ExprEval_TokenType::ExprEval_VARIABLE) {
                std::string name = current().identifierName;
                ++index;
                return std::make_unique<ExprEval_VariableNode>(name);
            }
            else if (current().tokenType == ExprEval_TokenType::ExprEval_FUNCTION) {
                std::string funcName = current().identifierName;
                ++index;
                if (current().tokenType != ExprEval_TokenType::ExprEval_LPAREN) {
                    throw std::runtime_error("函数 '" + funcName + "' 后需要 '('");
                }
                ++index;
                auto arg = parseExpression();
                if (current().tokenType != ExprEval_TokenType::ExprEval_RPAREN) {
                    throw std::runtime_error("缺少 ')'");
                }
                ++index;
                return std::make_unique<ExprEval_FunctionNode>(funcName, std::move(arg));
            }
            else if (current().tokenType == ExprEval_TokenType::ExprEval_LPAREN) {
                ++index;
                auto node = parseExpression();
                if (current().tokenType != ExprEval_TokenType::ExprEval_RPAREN) {
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

} // namespace ExprEval

// 对外接口（简洁封装）
class Expression {
public:
    Expression(const std::string& expr) {
        ExprEval::ExprEval_Lexer lexer(expr);
        auto tokens = lexer.tokenize();
        ExprEval::ExprEval_Parser parser(tokens);
        ast = parser.parse();
    }

    double evaluate(double x, double y) const {
        std::map<std::string, double> vars = { {"x", x}, {"y", y} };
        return ast->evaluate(vars);
    }

private:
    std::unique_ptr<ExprEval::ExprEval_Node> ast;
};

#endif // EXPRESSION_EVALUATOR_H