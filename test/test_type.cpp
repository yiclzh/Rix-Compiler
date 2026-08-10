#include "../src/function_table.hpp"
#include "../src/parser.hpp"
#include "../src/symbol_table.hpp"
#include "../src/type_checker.hpp"
#include <cassert>
#include <iostream>

using namespace rix;

static void registerMiniStdlib(FunctionTable& functions) {
    DimExpr T = DimExpr::var("T"), N = DimExpr::var("N"), n = DimExpr::var("n");

    functions.declare("pct_change", FunctionSig{
        "pct_change",
        {Param{"p", RixType::matrix(Tag{"Prices"}, T, N)}},
        RixType::matrix(Tag{"Returns"}, T - DimExpr::constant(1), N),
        {}
    });
    functions.declare("cov", FunctionSig{
        "cov",
        {Param{"r", RixType::matrix(Tag{"Returns"}, T, N)}},
        RixType::matrix(Tag{"CovMatrix"}, N, N),
        {}
    });
    functions.declare("std", FunctionSig{
        "std",
        {Param{"r", RixType::matrix(Tag{"Returns"}, T, N)}},
        RixType::matrix(Tag{"Vector"}, DimExpr::constant(1), N),
        {}
    });
    functions.declare("sharpe", FunctionSig{
        "sharpe",
        {Param{"r", RixType::matrix(Tag{"Returns"}, T, N)}, Param{"rf", RixType::scalar()}},
        RixType::matrix(Tag{"Vector"}, DimExpr::constant(1), N),
        {}
    });
    functions.declare("rolling", FunctionSig{
        "rolling",
        {Param{"m", RixType::matrix(Tag{"t", true}, T, N)}, Param{"f", RixType::string()}},  // f's real type TBD; String as a stand-in
        RixType::matrix(Tag{"Rolling_t", true}, T - n + DimExpr::constant(1), N),
        {"n"}
    });
}

static ProgramNode parseSource(const std::string& src) {
    Tokenizer tok(src);
    Parser parser(tok);
    return parser.parseProgram();
}

int main() {
    // ============ Valid program ============
    std::cout << "=== valid program ===\n";
    {
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
        // load_csv used here as a bare identifier standing in for a real call --
        // define it directly as a symbol for this test rather than modeling
        // the full load_csv/Dyn design from earlier.
        ProgramNode program = parseSource(src);

        FunctionTable functions;
        registerMiniStdlib(functions);
        SymbolTable symbols;
        symbols.define("load_csv", RixType::matrix(Tag{"Prices"}, DimExpr::constant(252), DimExpr::constant(3)), false);

        TypeChecker checker(symbols, functions);
        try {
            checker.checkProgram(program);
            std::cout << "checked OK -- no errors\n";
        } catch (const TypeError& e) {
            std::cout << "UNEXPECTED error at line " << e.line << ": " << e.what() << "\n";
            assert(false);
        }
    }

    // ============ Invalid program: cov(prices) directly ============
    std::cout << "\n=== cov(prices) should be rejected ===\n";
    {
        const char* src = R"(
            func main() -> Void {
                let prices: Matrix<Prices,252,3> = load_csv;
                let bad: Matrix<CovMatrix,3,3> = cov(prices);
                return;
            }
        )";
        ProgramNode program = parseSource(src);

        FunctionTable functions;
        registerMiniStdlib(functions);
        SymbolTable symbols;
        symbols.define("load_csv", RixType::matrix(Tag{"Prices"}, DimExpr::constant(252), DimExpr::constant(3)), false);

        TypeChecker checker(symbols, functions);
        bool threw = false;
        try {
            checker.checkProgram(program);
        } catch (const TypeError& e) {
            threw = true;
            std::cout << "correctly rejected at line " << e.line << ":\n" << e.what() << "\n";
            assert(std::string(e.what()).find("Prices") != std::string::npos);
            assert(std::string(e.what()).find("pct_change") != std::string::npos);
        }
        assert(threw);
    }

    // ============ Invalid program: assigning to a non-mut variable ============
    std::cout << "\n=== assigning to non-mut variable should be rejected ===\n";
    {
        const char* src = R"(
            func main() -> Void {
                let count: Int = 0;
                count = count + 1;
                return;
            }
        )";
        ProgramNode program = parseSource(src);
        FunctionTable functions;
        SymbolTable symbols;
        TypeChecker checker(symbols, functions);
        bool threw = false;
        try {
            checker.checkProgram(program);
        } catch (const TypeError& e) {
            threw = true;
            std::cout << "correctly rejected: " << e.what() << "\n";
        }
        assert(threw);
    }

    std::cout << "\nall checks passed\n";
    return 0;
}
