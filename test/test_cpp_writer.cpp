#include "../src/cpp_writer.hpp"
#include "../src/function_table.hpp"
#include "../src/parser.hpp"
#include "../src/symbol_table.hpp"
#include "../src/type_checker.hpp"
#include <fstream>
#include <iostream>
#include <set>

using namespace rix;

static void registerMiniStdlib(FunctionTable& functions) {
    DimExpr T = DimExpr::var("T"), N = DimExpr::var("N"), n = DimExpr::var("n");

    functions.declare("pct_change", FunctionSig{
        "pct_change", {Param{"p", RixType::matrix(Tag{"Prices"}, T, N)}},
        RixType::matrix(Tag{"Returns"}, T - DimExpr::constant(1), N), {}
    });
    functions.declare("cov", FunctionSig{
        "cov", {Param{"r", RixType::matrix(Tag{"Returns"}, T, N)}},
        RixType::matrix(Tag{"CovMatrix"}, N, N), {}
    });
    functions.declare("std", FunctionSig{
        "std", {Param{"r", RixType::matrix(Tag{"Returns"}, T, N)}},
        RixType::matrix(Tag{"Vector"}, DimExpr::constant(1), N), {}
    });
    functions.declare("sharpe", FunctionSig{
        "sharpe", {Param{"r", RixType::matrix(Tag{"Returns"}, T, N)}, Param{"rf", RixType::scalar()}},
        RixType::matrix(Tag{"Vector"}, DimExpr::constant(1), N), {}
    });
    functions.declare("rolling", FunctionSig{
        "rolling", {Param{"m", RixType::matrix(Tag{"t", true}, T, N)}, Param{"f", RixType::string()}},
        RixType::matrix(Tag{"Rolling_t", true}, T - n + DimExpr::constant(1), N), {"n"}
    });
}

int main() {
    const char* src = R"(
        func main() -> Void {
            let prices: Matrix<Prices,252,3> = load_csv;
            let returns: Matrix<Returns,251,3> = pct_change(prices);
            let vol: Matrix<Vector,1,3> = std(returns);
            let ratios: Matrix<Vector,1,3> = sharpe(returns, 0.045);
            let smoothed: Matrix<Returns,251,3> = returns;
            let combined: Matrix<Returns,251,3> = returns + smoothed;
            let window: Matrix<Rolling_t,192,3> = rolling<60>(returns, "std");
            return;
        }
    )";

    Tokenizer tok(src);
    Parser parser(tok);
    ProgramNode program = parser.parseProgram();

    FunctionTable functions;
    registerMiniStdlib(functions);
    SymbolTable symbols;
    symbols.define("load_csv", RixType::matrix(Tag{"Prices"}, DimExpr::constant(252), DimExpr::constant(3)), false);

    TypeChecker checker(symbols, functions);
    checker.checkProgram(program);   
    std::cout << "type check passed\n\n";

    std::set<std::string> stdlibNames = {"pct_change", "cov", "std", "sharpe", "rolling"};

    std::ofstream outFile("/tmp/generated.cpp");
    CppWriter writer(outFile, stdlibNames);
    writer.writeProgram(program);
    outFile.close();

    std::ifstream check("/tmp/generated.cpp");
    std::cout << check.rdbuf();
    return 0;
}
