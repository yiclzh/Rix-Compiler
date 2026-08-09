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






}