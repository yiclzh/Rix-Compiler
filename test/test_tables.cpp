#include "../src/function_table.hpp"
#include "../src/symbol_table.hpp"
#include "../src/unify.hpp"
#include <cassert>
#include <iostream>

using namespace rix;

int main() {
    // ===================== SymbolTable =====================
    std::cout << "=== SymbolTable ===\n";
    {
        SymbolTable symbols;
        symbols.define("returns", RixType::matrix(Tag{"Returns"}, DimExpr::constant(251), DimExpr::constant(3)), false);
        assert(symbols.isDefined("returns"));
        assert(!symbols.isMutable("returns"));

        symbols.enterScope();
        symbols.define("i", RixType::integer(), true);
        assert(symbols.isDefined("i"));
        assert(symbols.isDefined("returns"));   // outer scope still visible
        assert(symbols.isMutable("i"));

        symbols.exitScope();
        assert(!symbols.isDefined("i"));        // out of scope now
        assert(symbols.isDefined("returns"));   // outer survives

        bool threw = false;
        try { symbols.typeOf("nonexistent"); }
        catch (const SymbolError& e) { threw = true; std::cout << "correctly threw: " << e.what() << "\n"; }
        assert(threw);
    }
    std::cout << "SymbolTable checks passed\n\n";

    // ===================== FunctionTable =====================
    std::cout << "=== FunctionTable ===\n";
    FunctionTable functions;
    {
        DimExpr T = DimExpr::var("T"), N = DimExpr::var("N");

        functions.declare("pct_change", FunctionSig{
            "pct_change",
            {Param{"p", RixType::matrix(Tag{"Prices"}, T, N)}},
            RixType::matrix(Tag{"Returns"}, T - DimExpr::constant(1), N)
        });
        functions.declare("cov", FunctionSig{
            "cov",
            {Param{"r", RixType::matrix(Tag{"Returns"}, T, N)}},
            RixType::matrix(Tag{"CovMatrix"}, N, N)
        });
    }

    // instantiate() called twice for "cov" -- prove the two calls get
    // independent variable names, so their bindings can't collide.
    FunctionSig cov1 = functions.instantiate("cov", 1);
    FunctionSig cov2 = functions.instantiate("cov", 2);
    std::string cov1Var = *cov1.params[0].type.as<MatrixType>().rows.freeVars().begin();
    std::string cov2Var = *cov2.params[0].type.as<MatrixType>().rows.freeVars().begin();
    std::cout << "cov call 1 uses '" << cov1Var << "', cov call 2 uses '" << cov2Var << "'\n";
    assert(cov1Var == "T@1");
    assert(cov2Var == "T@2");
    assert(cov1Var != cov2Var);

    // Full round trip: instantiate cov for a real call, unify against a real
    // argument, resolve the return type.
    MatrixType returnsArg{Tag{"Returns"}, DimExpr::constant(251), DimExpr::constant(3)};
    Subst subst;
    unifyMatrixType(cov1.params[0].type.as<MatrixType>(), returnsArg, subst);
    RixType resolvedReturn = RixType::matrix(
        Tag{"CovMatrix"},
        subst.resolveDim(cov1.returnType.as<MatrixType>().rows),
        subst.resolveDim(cov1.returnType.as<MatrixType>().cols)
    );
    std::cout << "cov(returns) resolved to: " << resolvedReturn.toString() << "\n";
    assert(resolvedReturn == RixType::matrix(Tag{"CovMatrix"}, DimExpr::constant(3), DimExpr::constant(3)));

    // cov(prices) should still fail -- tag mismatch survives instantiation
    MatrixType pricesArg{Tag{"Prices"}, DimExpr::constant(252), DimExpr::constant(3)};
    Subst subst2;
    bool threw = false;
    try {
        unifyMatrixType(cov2.params[0].type.as<MatrixType>(), pricesArg, subst2);
    } catch (const UnifyError& e) {
        threw = true;
        std::cout << "cov(prices) correctly rejected: " << e.what() << "\n";
        auto fix = functions.suggestFix("Prices", "Returns");
        assert(fix.has_value());
        std::cout << "suggested fix: " << *fix << "(prices)\n";
        assert(*fix == "pct_change");
    }
    assert(threw);

    std::cout << "\nall checks passed\n";
    return 0;
}
