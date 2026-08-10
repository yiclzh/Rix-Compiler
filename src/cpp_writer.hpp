#pragma once
#include <ostream>
#include <set>
#include <string>

#include "ast.hpp"
#include "rix_type.hpp"

namespace rix {

// Walks a type-checked ProgramNode and emits real C++ (Eigen-backed) source.
// Runs as a separate pass AFTER TypeChecker, on the same AST -- by this
// point every type is already known to be valid.
//
// A user function like `main` needs no C++ template machinery: Rix has no
// user-defined generics, so every RixType reaching CppWriter is already
// fully concrete (rows/cols are literal ints). Calls into the stdlib don't
// need explicit template arguments either
class CppWriter {
public:
    CppWriter(std::ostream& out, std::set<std::string> stdlibFuncNames)
        : out_(out), stdlibFuncNames_(std::move(stdlibFuncNames)) {}

    void writeProgram(const ProgramNode& program);

private:
    std::ostream& out_;
    std::set<std::string> stdlibFuncNames_;   
    int indent_ = 0;
    bool inMainFunc_ = false;

    void writeIndent();
    void writeIncludes();
    void writeStructDecl(const StructDecl& decl);
    void writeFuncForwardDecl(const FuncDecl& decl);
    void writeFuncDecl(const FuncDecl& decl);
    void writeBlock(const Block& block);
    void writeStatement(const Statement& stmt);

    std::string emitType(const RixType& t);
    std::string emitExpr(const Expr& expr);
    std::string emitCall(const CallExpr& call);
    std::string emitBinary(const BinaryExpr& bin);
};

}
