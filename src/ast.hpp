#pragma once
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "dim_expr.hpp"
#include "rix_type.hpp"


// Struct definitions for every node in the grammer


namespace rix {


class Expr;
class Statement;
struct Block;

// Expressions

struct IntLiteral    { int value; int line = 0; };
struct FloatLiteral  { double value; int line = 0; };
struct StringLiteral { std::string value; int line = 0; };
struct BoolLiteral   { bool value; int line = 0; };
struct Identifier    { std::string name; int line = 0; };

struct CallExpr {
    std::string funcName;
    std::vector<DimExpr> templateArgs;      // rolling<60>(...) -> {60}
    std::vector<std::shared_ptr<Expr>> args;
    int line = 0;
};

struct BinaryExpr {
    std::string op;
    std::shared_ptr<Expr> lhs;
    std::shared_ptr<Expr> rhs;
    int line = 0;
};

struct UnaryExpr {
    std::string op;
    std::shared_ptr<Expr> operand;
    int line = 0;
};


struct StructLiteral {
    std::string structName;
    std::vector<std::pair<std::string, std::shared_ptr<Expr>>> fields;
    int line = 0;
};


class Expr {
    public:
        using Variant = std::variant<
            IntLiteral,
            FloatLiteral,
            StringLiteral,
            BoolLiteral,
            Identifier,
            CallExpr,
            BinaryExpr,
            UnaryExpr,
            StructLiteral
        >;

        explicit Expr(Variant v) : value_(std::move(v)) {}

        template <typename T>
        bool is() const { return std::holds_alternative<T>(value_); }

        template <typename T>
        const T& as() const { return std::get<T>(value_); }

        int line() const;
        std::string toString() const;

    private:
        Variant value_;

};

struct LetStatement {
    std::string name;
    bool isMut = false;
    std::optional<RixType> declaredType;
    std::shared_ptr<Expr> value;
    int line = 0;
};

struct AssignStatement {
    std::string name;
    std::shared_ptr<Expr> value;
    int line = 0;
};

struct IfStatement {
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Block> thenBlock;
    std::shared_ptr<Block> elseBlock;
    int line = 0;
};

struct ForStatement {
    std::string varName;
    DimExpr start;
    DimExpr end;
    std::shared_ptr<Block> body;
    int line = 0;
};

struct ReturnStatement {
    std::shared_ptr<Expr> value;
    int line = 0;
};

struct ExprStatement {
    std::shared_ptr<Expr> expr;
    int line = 0;
};

class Statement {
    public:
        using Variant = std::variant<
            LetStatement,
            AssignStatement,
            IfStatement,
            ForStatement,
            ReturnStatement,
            ExprStatement
        >;

        explicit Statement(Variant v) : value_(std::move(v)) {}

        template <typename T>
        bool is() const { return std::holds_alternative<T>(value_); }

        template <typename T>
        const T& as() const { return std::get<T>(value_); }
        
        int line() const;
        std::string toString() const;
    private:
        Variant value_;
};

struct Block {
    std::vector<Statement> statements;
};

struct Param {
    std::string name;
    RixType type;
};

struct FuncDecl {
    std::string name;
    std::vector<Param> params;
    RixType returnType;
    Block body;
    int line = 0;
};

struct FieldDecl {
    std::string name;
    RixType type;
};

struct StructDecl {
    std::string name;
    std::vector<std::string> genericParams;  // e.g. ["N"] for RiskReport<N: Nat>
    std::vector<FieldDecl> fields;
    int line = 0;
};

struct ProgramNode {
    std::vector<StructDecl> structs;
    std::vector<FuncDecl> funcs;
};

}