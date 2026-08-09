#pragma once
#include <string>
#include <unordered_map>

namespace rix {
    enum class TokenType {
        KEYWORD, 
        SYMBOL, 
        INT_CONST, 
        FLOAT_CONST, 
        STRING_CONST, 
        IDENTIFIER,
        END_OF_FILE
    };

    enum class Keyword {
        FUNC,
        STRUCT,
        LET,
        MUT,
        IF,
        ELSE,
        FOR,
        IN,
        RETURN,
        TRUE,
        FALSE,
        MATRIX,
        VOID,
        BOOL,
        INT,
        NAT,
        SCALAR,
        STRING,
        RESULT,
        OK,
        ERR,
        NONE
    };


    class Tokenizer {
        public:
            explicit Tokenizer(const std::string& source);

            bool hasMoreTokens() const;
            void advance();

            TokenType tokenType() const;
            Keyword keyword() const;
            char symbol() const;
            const std::string& identifier() const;
            int intVal() const;
            double floatVal() const;
            const std::string& stringVal() const;
            int currentLine() const;
        private:
            std::string source_;
            size_t pos = 0;
            int line = 1;

            TokenType currentType = TokenType::END_OF_FILE;
            Keyword currentKeyword = Keyword::NONE;
            char currentSymbol = '\0';
            std::string currentText;
            int currentIntVal = 0;
            double currentFloatVal = 0.0;

            static const std::unordered_map<std::string, Keyword> keywordTable;

            void skipWhitespaceAndComments();
            void readNumber();
            void readString();
            void readIdentifierOrKeyword();
            void readSymbol();

    };

}