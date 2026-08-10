#include "../src/unify.hpp"
#include <cassert>
#include <iostream>

using namespace rix;

int main() {
    DimExpr T = DimExpr::var("T");
    DimExpr N = DimExpr::var("N");

    std::cout << "=== cov(prices) ===\n";
    {
        MatrixType covParam{Tag{"Returns"}, T, N};
        MatrixType pricesArg{Tag{"Prices"}, DimExpr::constant(252), DimExpr::constant(3)};
        Subst subst;
        bool threw = false;
        try {
            unifyMatrixType(covParam, pricesArg, subst);
        } catch (const UnifyError& e) {
            threw = true;
            std::cout << "error: " << e.what() << "\n";
        }
        assert(threw);
    }

    // --- pct_change(prices) then cov(returns) should succeed end to end ---
    std::cout << "\n=== pct_change(prices) then cov(...) ===\n";
    MatrixType returnsResult{Tag{"Returns"}, DimExpr::constant(0), DimExpr::constant(0)};  // placeholder, filled below
    {
        MatrixType pctChangeParam{Tag{"Prices"}, T, N};
        MatrixType pricesArg{Tag{"Prices"}, DimExpr::constant(252), DimExpr::constant(3)};
        Subst subst;
        unifyMatrixType(pctChangeParam, pricesArg, subst);

        // return type is Matrix<Returns, T-1, N> -- resolve now that T,N are bound
        DimExpr resultRows = subst.resolveDim(T - DimExpr::constant(1));
        DimExpr resultCols = subst.resolveDim(N);
        returnsResult = MatrixType{Tag{"Returns"}, resultRows, resultCols};
        std::cout << "pct_change(prices) : Matrix<" << returnsResult.tag.name << ", "
                  << returnsResult.rows.toString() << ", " << returnsResult.cols.toString() << ">\n";
        assert(resultRows == DimExpr::constant(251));
        assert(resultCols == DimExpr::constant(3));
    }
    {
        MatrixType covParam{Tag{"Returns"}, T, N};
        Subst subst;
        unifyMatrixType(covParam, returnsResult, subst);
        DimExpr n = subst.resolveDim(N);
        std::cout << "cov(returns)        : Matrix<CovMatrix, " << n.toString() << ", " << n.toString() << ">\n";
        assert(n == DimExpr::constant(3));
    }

    // --- rolling<60>(returns, std): T - n + 1 with n baked to 60 ---
    std::cout << "\n=== rolling<60>(returns) ===\n";
    {
        MatrixType rollingParam{Tag{"t", true}, T, N};
        Subst subst;
        unifyMatrixType(rollingParam, returnsResult, subst);
        DimExpr resultRows = subst.resolveDim(T - DimExpr::constant(59));  // n=60 folded in: T-n+1 == T-59
        std::cout << "rolling<60>(returns) rows : " << resultRows.toString() << "\n";
        assert(resultRows == DimExpr::constant(192));
    }

    // --- solve(A, b): shared N across params, mismatched arg should throw ---
    std::cout << "\n=== solve(A, b) with mismatched N ===\n";
    {
        MatrixType solveParamA{Tag{"t", true}, N, N};
        MatrixType solveParamB{Tag{"t2", true}, N, DimExpr::var("K")};

        MatrixType covArg{Tag{"CovMatrix"}, DimExpr::constant(3), DimExpr::constant(3)};
        MatrixType badWeightsArg{Tag{"Weights"}, DimExpr::constant(4), DimExpr::constant(1)};  // wrong N

        Subst subst;
        unifyMatrixType(solveParamA, covArg, subst);   // binds N=3, t=CovMatrix
        bool threw = false;
        try {
            unifyMatrixType(solveParamB, badWeightsArg, subst);  // N already 3, arg says 4
        } catch (const UnifyError& e) {
            threw = true;
            std::cout << "correctly rejected: " << e.what() << "\n";
        }
        assert(threw);
    }

    std::cout << "\nall checks passed\n";
    return 0;
}
