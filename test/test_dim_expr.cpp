#include "../src/dim_expr.hpp"
#include "../src/tag.hpp"
#include <cassert>
#include <iostream>

using namespace rix;

int main() {
    // T - 1  (pct_change's output row count)
    DimExpr T = DimExpr::var("T");
    DimExpr TMinus1 = T - DimExpr::constant(1);
    std::cout << "T - 1            = " << TMinus1.toString() << "\n";
    assert(!TMinus1.isConstant()); // Should not be constant (Free variable)
    assert(TMinus1.freeVars().count("T") == 1); // Free Variable should be T

    // T - n + 1  (rolling<n>'s output row count)
    DimExpr n = DimExpr::var("n");
    DimExpr rollingShape = T - n + DimExpr::constant(1);
    std::cout << "T - n + 1         = " << rollingShape.toString() << "\n";

    // Resolve with T=251, n=60 -> should collapse to 192
    std::map<std::string, DimExpr> bindings = {
        {"T", DimExpr::constant(251)},
        {"n", DimExpr::constant(60)},
    };
    DimExpr resolved = rollingShape.substitute(bindings);
    std::cout << "substituted       = " << resolved.toString() << "\n";
    assert(resolved.isConstant());
    assert(resolved.constantTerm() == 192);

    // Partial substitution: only T known, n still free
    DimExpr partial = rollingShape.substitute({{"T", DimExpr::constant(251)}});
    std::cout << "partial substitute = " << partial.toString() << "\n";
    assert(!partial.isConstant());
    assert(partial.freeVars().count("n") == 1);
    assert(partial.freeVars().count("T") == 0);

    // Equality
    assert(DimExpr::constant(192) == DimExpr::constant(192));
    assert(DimExpr::var("T") != DimExpr::var("N"));
    assert((T - DimExpr::constant(1)) == (T - DimExpr::constant(1)));

    // Tag basics
    Tag prices{"Prices", false};
    Tag returns{"Returns", false};
    Tag t{"t", true};
    assert(prices != returns);
    Tag pricesAgain{"Prices", false};
    assert(prices == pricesAgain);
    assert(t.isVar);
    assert(UNTAGGED.name == "Untagged");

    std::cout << "\nall checks passed\n";
    return 0;
}
