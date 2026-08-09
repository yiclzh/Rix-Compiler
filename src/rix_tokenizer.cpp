#include "rix_tokenizer.hpp"
#include <cctype>
#include <stdexcept>

namespace rix {

const std::unordered_map<std::string, Keyword> Tokenizer::keywordTable = {
    {"func", Keyword::FUNC}, 
    {"struct", Keyword::STRUCT}, 
    {"let", Keyword::LET},
    {"mut", Keyword::MUT}, 
    {"if", Keyword::IF}, 
    {"else", Keyword::ELSE},
    {"for", Keyword::FOR}, 
    {"in", Keyword::IN}, 
    {"return", Keyword::RETURN},
    {"true", Keyword::TRUE}, 
    {"false", Keyword::FALSE}, 
    {"Matrix", Keyword::MATRIX},
    {"Void", Keyword::VOID}, 
    {"Bool", Keyword::BOOL}, 
    {"Int", Keyword::INT},
    {"Nat", Keyword::NAT}, 
    {"Scalar", Keyword::SCALAR}, 
    {"String", Keyword::STRING},
    {"Result", Keyword::RESULT}, 
    {"Ok", Keyword::OK}, 
    {"Err", Keyword::ERR},
};

Tokenizer::Tokenizer(const std::string& source) : source_(source) {
    advance();  
}

bool Tokenizer::hasMoreTokens() const {
    return currentType != TokenType::END_OF_FILE;
}

void Tokenizer::advance() {
    skipWhitespaceAndComments();
    if (pos >= source_.size()) { currentType = TokenType::END_OF_FILE; return; }

    char c = source_[pos];
    if (std::isdigit(static_cast<unsigned char>(c))) { 
        readNumber(); 
        return; 
    }
    if (c == '"') { 
        readString(); 
        return; 
    }
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') { 
        readIdentifierOrKeyword(); 
        return; 
    }
    readSymbol();
}

TokenType Tokenizer::tokenType() const { return currentType; }
Keyword Tokenizer::keyword() const { return currentKeyword; }
char Tokenizer::symbol() const { return currentSymbol; }
const std::string& Tokenizer::identifier() const { return currentText; }
int Tokenizer::intVal() const { return currentIntVal; }
double Tokenizer::floatVal() const { return currentFloatVal; }
const std::string& Tokenizer::stringVal() const { return currentText; }
int Tokenizer::currentLine() const { return line; }

void Tokenizer::skipWhitespaceAndComments() {
    while (pos < source_.size()) {
        char c = source_[pos];
        if (c == '\n') { ++line; ++pos; }
        else if (std::isspace(static_cast<unsigned char>(c))) { ++pos; }
        else if (c == '/' && pos + 1 < source_.size() && source_[pos+1] == '/') {
            while (pos < source_.size() && source_[pos] != '\n') ++pos;
        }
        else break;
    }
}

void Tokenizer::readNumber() {
    size_t start = pos;
    bool isFloat = false;

    while (pos < source_.size() && std::isdigit(static_cast<unsigned char>(source_[pos])))
        ++pos;

    bool nextIsDigit = pos + 1 < source_.size() &&
                        std::isdigit(static_cast<unsigned char>(source_[pos + 1]));
    if (pos < source_.size() && source_[pos] == '.' && nextIsDigit) {
        isFloat = true;
        ++pos;
        while (pos < source_.size() && std::isdigit(static_cast<unsigned char>(source_[pos])))
            ++pos;
    }

    currentText = source_.substr(start, pos - start);
    if (isFloat) {
        currentType = TokenType::FLOAT_CONST;
        currentFloatVal = std::stod(currentText);
    } else {
        currentType = TokenType::INT_CONST;
        currentIntVal = std::stoi(currentText);
    }
}

void Tokenizer::readString() {
    ++pos;  
    size_t start = pos;
    while (pos < source_.size() && source_[pos] != '"') ++pos;
    currentText = source_.substr(start, pos - start);
    ++pos;  
    currentType = TokenType::STRING_CONST;
}

void Tokenizer::readIdentifierOrKeyword() {
    size_t start = pos;
    while (pos < source_.size() &&
           (std::isalnum(static_cast<unsigned char>(source_[pos])) || source_[pos] == '_')) ++pos;
    currentText = source_.substr(start, pos - start);

    auto it = keywordTable.find(currentText);
    if (it != keywordTable.end()) { 
        currentType = TokenType::KEYWORD; 
        currentKeyword = it->second; 
    }
    else { 
        currentType = TokenType::IDENTIFIER; 
        currentKeyword = Keyword::NONE; 
    }
}

void Tokenizer::readSymbol() {
    currentSymbol = source_[pos];
    if (currentSymbol == '.' && pos + 1 < source_.size() && source_[pos+1] == '*') {
        currentText = ".*"; 
        pos += 2;
    } else if (currentSymbol == '.' && pos + 1 < source_.size() && source_[pos+1] == '/') {
        currentText = "./"; 
        pos += 2;
    } else if (currentSymbol == '-' && pos + 1 < source_.size() && source_[pos+1] == '>') {
        currentText = "->"; 
        pos += 2;
    } else if (currentSymbol == '.' && pos + 1 < source_.size() && source_[pos+1] == '.') {
        currentText = ".."; 
        pos += 2;
    } else if (currentSymbol == '=' && pos + 1 < source_.size() && source_[pos+1] == '=') {
        currentText = "=="; 
        pos += 2;
    } else {
        currentText = std::string(1, currentSymbol);
        ++pos;
    }
    currentType = TokenType::SYMBOL;
}

}