#pragma once
#include <stdexcept>
#include <string>

#include "ast.hpp"
#include "function_table.hpp"
#include "symbol_table.hpp"

namespace rix {

struct TypeError : std::runtime_error {
    int line;
    TypeError(const std::string& msg, int line_) : std::runtime_error(msg), line(line_) {}
};

class TypeChecker {
public:
    TypeChecker(SymbolTable& symbols, FunctionTable& functions)
        : symbols_(symbols), functions_(functions) {}

    void checkProgram(const ProgramNode& program);

private:
    SymbolTable& symbols_;
    FunctionTable& functions_;

    RixType currentReturnType_ = RixType::voidType();
    int nextCallId_ = 1;

    void checkStructDecl(const StructDecl& decl);
    void checkFuncDecl(const FuncDecl& decl);
    void checkBlock(const Block& block);
    void checkStatement(const Statement& stmt);

    void checkLet(const LetStatement& s);
    void checkAssign(const AssignStatement& s);
    void checkIf(const IfStatement& s);
    void checkFor(const ForStatement& s);
    void checkReturn(const ReturnStatement& s);
    void checkExprStatement(const ExprStatement& s);

    RixType checkExpr(const Expr& expr);
    RixType checkCall(const CallExpr& call, int line);
    RixType checkBinary(const BinaryExpr& bin, int line);
    RixType checkUnary(const UnaryExpr& un, int line);
    RixType checkStructLiteral(const StructLiteral& lit, int line);

    // Throws TypeError; for two MatrixTypes, routes through unifyMatrixType
    // so the error names the specific mismatch (tag vs shape) rather than a
    // blanket "types don't match".
    void checkAssignable(const RixType& declared, const RixType& actual, int line);
};

} 
