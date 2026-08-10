#include "cpp_writer.hpp"
#include "function_table.hpp"
#include "parser.hpp"
#include "symbol_table.hpp"
#include "rix_tokenizer.hpp"
#include "type_checker.hpp"

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

using namespace rix;

namespace {

    std::string readFile(const std::string&path) {
        std::ifstream file(path);
        if(!file) {
            throw std::runtime_error("could not open file.");

        }
        std::ostringstream contents;
        contents << file.rdbuf();
        return contents.str();
    }

    std::string tokenTypeName(TokenType t) {
        switch(t) {
             case TokenType::KEYWORD:      
                 return "KEYWORD";
             case TokenType::SYMBOL:
                 return "SYMBOL";
             case TokenType::INT_CONST:
                 return "INT_CONST";
             case TokenType::FLOAT_CONST:
                 return "FLOAT_CONST";
             case TokenType::STRING_CONST:
                 return "STRING_CONST";
             case TokenType::IDENTIFIER:
                 return "IDENTIFIER";
             case TokenType::END_OF_FILE:
                 return "EOF";
             }
             return "UNKNOWN";
    }

    void dumpTokens(const std::string& source) {
    Tokenizer tok(source);
        while (tok.hasMoreTokens()) {
            std::cout << "[line " << tok.currentLine() << "] "
                    << tokenTypeName(tok.tokenType()) << "  " << tok.identifier() << "\n";
            tok.advance();
        }
    }

    void registerStdlib(FunctionTable& functions) {
        DimExpr T = DimExpr::var("T"), N = DimExpr::var("N"), K = DimExpr::var("K");

        functions.declare("pct_change", FunctionSig{
                                            "pct_change", {Param{"p", RixType::matrix(Tag{"Prices"}, T, N)}}, RixType::matrix(Tag{"Returns"}, T - DimExpr::constant(1), N), {}});
        functions.declare("cov", FunctionSig{
                                     "cov", {Param{"r", RixType::matrix(Tag{"Returns"}, T, N)}}, RixType::matrix(Tag{"CovMatrix"}, N, N), {}});
        functions.declare("corr", FunctionSig{
                                      "corr", {Param{"r", RixType::matrix(Tag{"Returns"}, T, N)}}, RixType::matrix(Tag{"CorrMatrix"}, N, N), {}});
        functions.declare("std", FunctionSig{
                                     "std", {Param{"r", RixType::matrix(Tag{"Returns"}, T, N)}}, RixType::matrix(Tag{"Vector"}, DimExpr::constant(1), N), {}});
        functions.declare("var", FunctionSig{
                                     "var", {Param{"r", RixType::matrix(Tag{"Returns"}, T, N)}}, RixType::matrix(Tag{"Vector"}, DimExpr::constant(1), N), {}});
        functions.declare("mean", FunctionSig{
                                      "mean", {Param{"r", RixType::matrix(Tag{"Returns"}, T, N)}}, RixType::matrix(Tag{"Vector"}, DimExpr::constant(1), N), {}});
        functions.declare("sharpe", FunctionSig{
                                        "sharpe", {Param{"r", RixType::matrix(Tag{"Returns"}, T, N)}, Param{"rf", RixType::scalar()}}, RixType::matrix(Tag{"Vector"}, DimExpr::constant(1), N), {}});
        functions.declare("drawdown", FunctionSig{
                                          "drawdown", {Param{"p", RixType::matrix(Tag{"Prices"}, T, N)}}, RixType::matrix(Tag{"Drawdown"}, T, N), {}});
        functions.declare("ewma", FunctionSig{
                                      "ewma", {Param{"r", RixType::matrix(Tag{"Returns"}, T, N)}, Param{"lambda", RixType::scalar()}}, RixType::matrix(Tag{"Returns"}, T, N), {}});
        functions.declare("rolling", FunctionSig{
                                         "rolling", {Param{"m", RixType::matrix(Tag{"t", true}, T, N)}, Param{"f", RixType::string()}}, RixType::matrix(Tag{"Rolling_t", true}, T - DimExpr::var("n") + DimExpr::constant(1), N), {"n"}});
        functions.declare("inv", FunctionSig{
                                     "inv", {Param{"m", RixType::matrix(Tag{"t", true}, N, N)}}, RixType::matrix(Tag{"t", true}, N, N), {}});
        functions.declare("solve", FunctionSig{
                                       "solve", {Param{"a", RixType::matrix(Tag{"t", true}, N, N)}, Param{"b", RixType::matrix(Tag{"t2", true}, N, K)}}, RixType::matrix(Tag{"t2", true}, N, K), {}});
        functions.declare("cholesky", FunctionSig{
                                          "cholesky", {Param{"m", RixType::matrix(Tag{"PSD"}, N, N)}}, RixType::matrix(Tag{"LowerTri"}, N, N), {}});
        functions.declare("transpose", FunctionSig{
                                           "transpose", {Param{"m", RixType::matrix(Tag{"t", true}, T, N)}}, RixType::matrix(Tag{"t", true}, N, T), {}});
        functions.declare("min_variance_weights", FunctionSig{
                                                      "min_variance_weights", {Param{"cov", RixType::matrix(Tag{"CovMatrix"}, N, N)}}, RixType::matrix(Tag{"Weights"}, N, DimExpr::constant(1)), {}});
        functions.declare("concat_rows", FunctionSig{
                                             "concat_rows", {Param{"a", RixType::matrix(Tag{"t", true}, T, N)}, Param{"b", RixType::matrix(Tag{"t", true}, DimExpr::var("T2"), N)}}, RixType::matrix(Tag{"t", true}, T + DimExpr::var("T2"), N), {}});
    }

}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: rixc <source.rix> <output.cpp>\n"
                     "       rixc <source.rix> --tokens\n";
        return 1;
    }

    std::string source;
    try {
        source = readFile(argv[1]);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    if (argc >= 3 && std::string(argv[2]) == "--tokens") {
        dumpTokens(source);
        return 0;
    }

    if (argc < 3) {
        std::cerr << "error: missing output path\n"
                     "usage: rixc <source.rix> <output.cpp>\n";
        return 1;
    }
    std::string outputPath = argv[2];

    try {
        Tokenizer tok(source);
        Parser parser(tok);
        ProgramNode program = parser.parseProgram();

        FunctionTable functions;
        registerStdlib(functions);
        SymbolTable symbols;

        TypeChecker checker(symbols, functions);
        checker.checkProgram(program);

        std::set<std::string> stdlibNames = {
            "pct_change", "cov", "corr", "std", "var", "mean", "sharpe",
            "drawdown", "ewma", "rolling", "inv", "solve", "cholesky",
            "transpose", "min_variance_weights", "concat_rows"
        };

        std::ofstream out(outputPath);
        if (!out) {
            std::cerr << "error: could not open output file: " << outputPath << "\n";
            return 1;
        }
        CppWriter writer(out, stdlibNames);
        writer.writeProgram(program);

        std::cout << "compiled " << argv[1] << " -> " << outputPath << "\n";
        return 0;

    } catch (const ParseError& e) {
        std::cerr << argv[1] << ":" << e.line << ": parse error: " << e.what() << "\n";
        return 1;
    } catch (const TypeError& e) {
        std::cerr << argv[1] << ":" << e.line << ": type error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}

    







