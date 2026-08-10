#include "parser.hpp"

namespace rix {

bool Parser::checkSymbol(const std::string& s) const {
    return tok_.tokenType() == TokenType::SYMBOL && tok_.identifier() == s;
}

bool Parser::checkKeyword(Keyword k) const {
    return tok_.tokenType() == TokenType::KEYWORD && tok_.keyword() == k;
}

void Parser::expectSymbol(const std::string& s) {
    if (!checkSymbol(s)) {
        throw ParseError("expected '" + s + "', got '" + tok_.identifier() + "'", tok_.currentLine());
    }
    tok_.advance();
}

void Parser::expectKeyword(Keyword k) {
    if (!checkKeyword(k)) {
        throw ParseError("unexpected token '" + tok_.identifier() + "'", tok_.currentLine());
    }
    tok_.advance();
}

std::string Parser::expectIdentifierText() {
    // Accepts plain identifiers, and the handful of keywords (Ok, Err) that
    // are also used as callable names in expression position.
    if (tok_.tokenType() != TokenType::IDENTIFIER &&
        !(tok_.tokenType() == TokenType::KEYWORD &&
          (tok_.keyword() == Keyword::OK || tok_.keyword() == Keyword::ERR))) {
        throw ParseError("expected an identifier, got '" + tok_.identifier() + "'", tok_.currentLine());
    }
    std::string name = tok_.identifier();
    tok_.advance();
    return name;
}

ProgramNode Parser::parseProgram() {
    ProgramNode program;
    while (tok_.hasMoreTokens()) {
        if (checkKeyword(Keyword::STRUCT)) {
            program.structs.push_back(parseStructDecl());
        } else if (checkKeyword(Keyword::FUNC)) {
            program.funcs.push_back(parseFuncDecl());
        } else {
            throw ParseError("Expected struct or func but got '" + tok_.identifier() + "'", tok_.currentLine());
        }
    }
    return program;
}

StructDecl Parser::parseStructDecl() {
    int line = tok_.currentLine();
    expectKeyword(Keyword::STRUCT);
    StructDecl decl;
    decl.line = line;
    decl.name = expectIdentifierText();

    if (checkSymbol("<")) {
        tok_.advance();
        while (true) {
            decl.genericParams.push_back(expectIdentifierText());
            expectSymbol(":");
            expectKeyword(Keyword::NAT);
            if (checkSymbol(",")) {
                tok_.advance();
                continue;
            } break;
        }
        expectSymbol(">");
    }

    expectSymbol("{");
    while (!checkSymbol("}")) {
        FieldDecl field;
        field.name = expectIdentifierText();
        expectSymbol(":");
        field.type = parseType();
        if (checkSymbol(",")) tok_.advance();
        decl.fields.push_back(std::move(field));
    }
    expectSymbol("}");
    return decl;

}

FuncDecl Parser::parseFuncDecl() {
    int line = tok_.currentLine();
    expectKeyword(Keyword::FUNC);
    FuncDecl decl;
    decl.line = line;
    decl.name = expectIdentifierText();
    expectSymbol("(");
    decl.params = parseParamList();
    expectSymbol(")");
    expectSymbol("->");
    decl.returnType = parseType();
    decl.body = parseBlockValue();
    return decl;

}

std::vector<Param> Parser::parseParamList() {
    std::vector<Param> params;
    while (!checkSymbol(")")) {
        Param p;
        p.name = expectIdentifierText();
        expectSymbol(":");
        p.type = parseType();
        params.push_back(std::move(p));
        if (checkSymbol(",")) { tok_.advance(); continue; }
        break;
    }
    return params;
}

RixType Parser::parseType() {
    if (checkKeyword(Keyword::MATRIX))  return parseMatrixType();
    if (checkKeyword(Keyword::SCALAR))  { tok_.advance(); return RixType::scalar(); }
    if (checkKeyword(Keyword::BOOL))    { tok_.advance(); return RixType::boolean(); }
    if (checkKeyword(Keyword::INT))     { tok_.advance(); return RixType::integer(); }
    if (checkKeyword(Keyword::NAT))     { tok_.advance(); return RixType::nat(); }
    if (checkKeyword(Keyword::STRING))  { tok_.advance(); return RixType::string(); }
    if (checkKeyword(Keyword::VOID))    { tok_.advance(); return RixType::voidType(); }

    if (checkKeyword(Keyword::RESULT)) {
        tok_.advance();
        expectSymbol("<");
        RixType okType = parseType();
        expectSymbol(",");
        std::string errName = expectIdentifierText();
        if (errName != "Error") {
            throw ParseError("expected 'Error' in Result<T, Error>, got '" + errName + "'", tok_.currentLine());
        }
        expectSymbol(">");
        return RixType::result(std::move(okType));
    }

    if (tok_.tokenType() == TokenType::IDENTIFIER) {
        std::string name = expectIdentifierText();
        std::vector<DimExpr> genericArgs;
        if (checkSymbol("<")) {
            tok_.advance();
            genericArgs.push_back(parseDimExpr());
            while (checkSymbol(",")) { tok_.advance(); genericArgs.push_back(parseDimExpr()); }
            expectSymbol(">");
        }
        return RixType::structType(name, genericArgs);
    }

    throw ParseError("expected a type, got '" + tok_.identifier() + "'", tok_.currentLine());
}

RixType Parser::parseMatrixType() {
    expectKeyword(Keyword::MATRIX);
    expectSymbol("<");
    std::string tagName = expectIdentifierText();
    expectSymbol(",");
    DimExpr rows = parseDimExpr();
    expectSymbol(",");
    DimExpr cols = parseDimExpr();
    expectSymbol(">");
    return RixType::matrix(Tag{tagName, /*isVar=*/false}, rows, cols);
}


DimExpr Parser::parseDimExpr() {
    DimExpr result = [&]() -> DimExpr {
        if (tok_.tokenType() == TokenType::INT_CONST) {
            int v = tok_.intVal();
            tok_.advance();
            return DimExpr::constant(v);
        }
        if (tok_.tokenType() == TokenType::IDENTIFIER) {
            std::string name = tok_.identifier();
            tok_.advance();
            return DimExpr::var(name);
        }
        throw ParseError("expected a dimension (number or name), got '" + tok_.identifier() + "'", tok_.currentLine());
    }();

    while (checkSymbol("+") || checkSymbol("-")) {
        bool isPlus = checkSymbol("+");
        tok_.advance();
        DimExpr rhs = [&]() -> DimExpr {
            if (tok_.tokenType() == TokenType::INT_CONST) {
                int v = tok_.intVal(); tok_.advance(); return DimExpr::constant(v);
            }
            if (tok_.tokenType() == TokenType::IDENTIFIER) {
                std::string name = tok_.identifier(); tok_.advance(); return DimExpr::var(name);
            }
            throw ParseError("expected a dimension term", tok_.currentLine());
        }();
        result = isPlus ? (result + rhs) : (result - rhs);
    }
    return result;
}

Block Parser::parseBlockValue() {
    expectSymbol("{");
    Block block;
    while (!checkSymbol("}")) {
        block.statements.push_back(parseStatement());
    }
    expectSymbol("}");
    return block;
}

std::shared_ptr<Block> Parser::parseBlockPtr() {
    return std::make_shared<Block>(parseBlockValue());
}

bool Parser::peekIsAssign() {
    Tokenizer snapshot = tok_;   // Tokenizer is a cheap-to-copy value type,
                                 // so this is a genuine, safe rewind point.
    tok_.advance();              // step past the identifier
    bool isAssign = checkSymbol("=");
    tok_ = snapshot;             // rewind -- caller re-parses from scratch
    return isAssign;
}

Statement Parser::parseStatement() {
    if (checkKeyword(Keyword::LET))    return parseLetStatement();
    if (checkKeyword(Keyword::IF))     return parseIfStatement();
    if (checkKeyword(Keyword::FOR))    return parseForStatement();
    if (checkKeyword(Keyword::RETURN)) return parseReturnStatement();

    if (tok_.tokenType() == TokenType::IDENTIFIER && peekIsAssign()) {
        return parseAssignStatement();
    }
    return parseExprStatement();
}

Statement Parser::parseLetStatement() {
    int line = tok_.currentLine();
    expectKeyword(Keyword::LET);
    bool isMut = false;
    if (checkKeyword(Keyword::MUT)) { isMut = true; tok_.advance(); }

    std::string name = expectIdentifierText();
    std::optional<RixType> declaredType;
    if (checkSymbol(":")) { tok_.advance(); declaredType = parseType(); }

    expectSymbol("=");
    auto value = parseExpr();
    expectSymbol(";");
    return Statement{Statement::Variant{
        LetStatement{name, isMut, declaredType, value, line}
    }};
}

Statement Parser::parseAssignStatement() {
    int line = tok_.currentLine();
    std::string name = expectIdentifierText();
    expectSymbol("=");
    auto value = parseExpr();
    expectSymbol(";");
    return Statement{Statement::Variant{AssignStatement{name, value, line}}};
}

Statement Parser::parseIfStatement() {
    int line = tok_.currentLine();
    expectKeyword(Keyword::IF);
    expectSymbol("(");
    auto cond = parseExpr();
    expectSymbol(")");
    auto thenBlk = parseBlockPtr();
    std::shared_ptr<Block> elseBlk = nullptr;
    if (checkKeyword(Keyword::ELSE)) {
        tok_.advance();
        elseBlk = parseBlockPtr();
    }
    return Statement{Statement::Variant{IfStatement{cond, thenBlk, elseBlk, line}}};
}

Statement Parser::parseForStatement() {
    int line = tok_.currentLine();
    expectKeyword(Keyword::FOR);
    std::string varName = expectIdentifierText();
    expectKeyword(Keyword::IN);
    DimExpr start = parseDimExpr();
    expectSymbol("..");
    DimExpr end = parseDimExpr();
    auto body = parseBlockPtr();
    return Statement{Statement::Variant{ForStatement{varName, start, end, body, line}}};
}

Statement Parser::parseReturnStatement() {
    int line = tok_.currentLine();
    expectKeyword(Keyword::RETURN);
    std::shared_ptr<Expr> value = nullptr;
    if (!checkSymbol(";")) value = parseExpr();
    expectSymbol(";");
    return Statement{Statement::Variant{ReturnStatement{value, line}}};
}

Statement Parser::parseExprStatement() {
    int line = tok_.currentLine();
    auto expr = parseExpr();
    expectSymbol(";");
    return Statement{Statement::Variant{ExprStatement{expr, line}}};
}

std::shared_ptr<Expr> Parser::parseExpr() { return parseComparison(); }

std::shared_ptr<Expr> Parser::parseComparison() {
    auto left = parseAdditive();
    while (checkSymbol("==") || checkSymbol("<") || checkSymbol(">")) {
        int line = tok_.currentLine();
        std::string op = tok_.identifier();
        tok_.advance();
        auto right = parseAdditive();
        left = std::make_shared<Expr>(Expr::Variant{BinaryExpr{op, left, right, line}});
    }
    return left;
}

std::shared_ptr<Expr> Parser::parseAdditive() {
    auto left = parseMultiplicative();
    while (checkSymbol("+") || checkSymbol("-")) {
        int line = tok_.currentLine();
        std::string op = tok_.identifier();
        tok_.advance();
        auto right = parseMultiplicative();
        left = std::make_shared<Expr>(Expr::Variant{BinaryExpr{op, left, right, line}});
    }
    return left;
}

std::shared_ptr<Expr> Parser::parseMultiplicative() {
    auto left = parseUnary();
    while (checkSymbol("*") || checkSymbol(".*") || checkSymbol("/") || checkSymbol("./")) {
        int line = tok_.currentLine();
        std::string op = tok_.identifier();
        tok_.advance();
        auto right = parseUnary();
        left = std::make_shared<Expr>(Expr::Variant{BinaryExpr{op, left, right, line}});
    }
    return left;
}

std::shared_ptr<Expr> Parser::parseUnary() {
    if (checkSymbol("-")) {
        int line = tok_.currentLine();
        tok_.advance();
        auto operand = parseUnary();
        return std::make_shared<Expr>(Expr::Variant{UnaryExpr{"-", operand, line}});
    }
    return parsePrimary();
}

std::shared_ptr<Expr> Parser::parsePrimary() {
    int line = tok_.currentLine();

    if (tok_.tokenType() == TokenType::INT_CONST) {
        int v = tok_.intVal(); tok_.advance();
        return std::make_shared<Expr>(Expr::Variant{IntLiteral{v, line}});
    }
    if (tok_.tokenType() == TokenType::FLOAT_CONST) {
        double v = tok_.floatVal(); tok_.advance();
        return std::make_shared<Expr>(Expr::Variant{FloatLiteral{v, line}});
    }
    if (tok_.tokenType() == TokenType::STRING_CONST) {
        std::string v = tok_.stringVal(); tok_.advance();
        return std::make_shared<Expr>(Expr::Variant{StringLiteral{v, line}});
    }
    if (checkKeyword(Keyword::TRUE))  { tok_.advance(); return std::make_shared<Expr>(Expr::Variant{BoolLiteral{true, line}}); }
    if (checkKeyword(Keyword::FALSE)) { tok_.advance(); return std::make_shared<Expr>(Expr::Variant{BoolLiteral{false, line}}); }

    if (checkSymbol("(")) {
        tok_.advance();
        auto e = parseExpr();
        expectSymbol(")");
        return e;
    }

    if (tok_.tokenType() == TokenType::IDENTIFIER ||
        checkKeyword(Keyword::OK) || checkKeyword(Keyword::ERR)) {
        std::string name = tok_.identifier();
        tok_.advance();
        return parseNameLike(name, line);
    }

    throw ParseError("unexpected token '" + tok_.identifier() + "' in expression", line);
}

std::shared_ptr<Expr> Parser::parseNameLike(const std::string& name, int line) {
    if (checkSymbol("(")) {
        tok_.advance();
        auto args = parseExprList();
        expectSymbol(")");
        return std::make_shared<Expr>(Expr::Variant{CallExpr{name, {}, args, line}});
    }

    if (checkSymbol("<")) {
        Tokenizer snapshot = tok_;
        try {
            tok_.advance();   // consume '<'
            std::vector<DimExpr> templateArgs;
            templateArgs.push_back(parseDimExpr());
            while (checkSymbol(",")) { tok_.advance(); templateArgs.push_back(parseDimExpr()); }
            expectSymbol(">");
            expectSymbol("(");
            auto args = parseExprList();
            expectSymbol(")");
            return std::make_shared<Expr>(Expr::Variant{CallExpr{name, templateArgs, args, line}});
        } catch (const ParseError&) {
            tok_ = snapshot;   
        }
    }

    // Struct literal: Name { field: expr, ... }
    if (checkSymbol("{")) {
        tok_.advance();
        std::vector<std::pair<std::string, std::shared_ptr<Expr>>> fields;
        while (!checkSymbol("}")) {
            std::string fieldName = expectIdentifierText();
            expectSymbol(":");
            auto fieldValue = parseExpr();
            fields.emplace_back(fieldName, fieldValue);
            if (checkSymbol(",")) tok_.advance();
        }
        expectSymbol("}");
        return std::make_shared<Expr>(Expr::Variant{StructLiteral{name, fields, line}});
    }

    
    return std::make_shared<Expr>(Expr::Variant{Identifier{name, line}});
}

std::vector<std::shared_ptr<Expr>> Parser::parseExprList() {
    std::vector<std::shared_ptr<Expr>> args;
    if (checkSymbol(")")) return args;
    args.push_back(parseExpr());
    while (checkSymbol(",")) { tok_.advance(); args.push_back(parseExpr()); }
    return args;
}


}