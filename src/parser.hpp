#pragma once
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "ast.hpp"
#include "rix_tokenizer.hpp"

namespace rix {

struct ParseError : std::runtime_error {
    int line;
    ParseError(const std::string& msg, int line_)
        : std::runtime_error(msg), line(line_) {}
};

class Parser {
public:
    explicit Parser(Tokenizer& tok) : tok_(tok) {}

    ProgramNode parseProgram();

private:
    Tokenizer& tok_;

    // --- token helpers ---
    bool checkSymbol(const std::string& s) const;
    bool checkKeyword(Keyword k) const;
    void expectSymbol(const std::string& s);
    void expectKeyword(Keyword k);
    std::string expectIdentifierText();  

    // --- top level ---
    StructDecl parseStructDecl();
    FuncDecl parseFuncDecl();
    std::vector<Param> parseParamList();

    // --- types ---
    RixType parseType();
    RixType parseMatrixType();
    DimExpr parseDimExpr();

    // --- statements ---
    Block parseBlockValue();
    std::shared_ptr<Block> parseBlockPtr();
    Statement parseStatement();
    Statement parseLetStatement();
    Statement parseAssignStatement();
    Statement parseIfStatement();
    Statement parseForStatement();
    Statement parseReturnStatement();
    Statement parseExprStatement();
    bool peekIsAssign();   

    std::shared_ptr<Expr> parseExpr();
    std::shared_ptr<Expr> parseComparison();
    std::shared_ptr<Expr> parseAdditive();
    std::shared_ptr<Expr> parseMultiplicative();
    std::shared_ptr<Expr> parseUnary();
    std::shared_ptr<Expr> parsePrimary();
    std::shared_ptr<Expr> parseNameLike(const std::string& name, int line);  // call / generic call / struct literal / bare identifier
    std::vector<std::shared_ptr<Expr>> parseExprList();
};

} 
