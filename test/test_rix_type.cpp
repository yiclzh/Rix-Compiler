#include "../src/rix_type.hpp"
#include <cassert>
#include <iostream>

using namespace rix;

int main() {
    DimExpr T = DimExpr::var("T");
    DimExpr N = DimExpr::var("N");

    // cov(r): Matrix<Returns,T,N> -> Matrix<CovMatrix,N,N>
    RixType returnsParam = RixType::matrix(Tag{"Returns", false}, T, N);
    RixType covResult    = RixType::matrix(Tag{"CovMatrix", false}, N, N);
    std::cout << "cov param  : " << returnsParam.toString() << "\n";
    std::cout << "cov return : " << covResult.toString() << "\n";
    assert(returnsParam.is<MatrixType>());
    assert(returnsParam.as<MatrixType>().tag.name == "Returns");

    // Tag distinctness: cov(prices) should build a *different* RixType
    RixType pricesParam = RixType::matrix(Tag{"Prices", false}, T, N);
    assert(pricesParam != returnsParam);
    std::cout << "Prices != Returns as RixType: confirmed\n";

    // solve(a, b): Matrix<t,N,N>, Matrix<t2,N,K> -> Matrix<t2,N,K>
    DimExpr K = DimExpr::var("K");
    RixType solveParamA = RixType::matrix(Tag{"t", true}, N, N);
    RixType solveParamB = RixType::matrix(Tag{"t2", true}, N, K);
    assert(solveParamA.as<MatrixType>().tag.isVar);
    std::cout << "solve param A: " << solveParamA.toString() << "\n";
    std::cout << "solve param B: " << solveParamB.toString() << "\n";

    // compute_var's return type: Result<Matrix<Vector,1,N>, Error>
    RixType vectorType = RixType::matrix(Tag{"Vector", false}, DimExpr::constant(1), N);
    RixType resultType = RixType::result(vectorType);
    std::cout << "Result type   : " << resultType.toString() << "\n";
    assert(resultType.is<ResultType>());
    assert(*resultType.as<ResultType>().okType == vectorType);

    // Scalar, Bool, Int, Nat, String, Void
    assert(RixType::scalar().toString() == "Scalar");
    assert(RixType::boolean().toString() == "Bool");
    assert(RixType::integer().toString() == "Int");
    assert(RixType::nat().toString() == "Nat");
    assert(RixType::string().toString() == "String");
    assert(RixType::voidType().toString() == "Void");
    assert(RixType::scalar() == RixType::scalar());
    assert(RixType::scalar() != RixType::boolean());

    // RiskReport<10> struct type
    RixType riskReport = RixType::structType("RiskReport", {DimExpr::constant(10)});
    std::cout << "struct type   : " << riskReport.toString() << "\n";
    assert(riskReport.is<StructType>());

    // Result equality: two separately-built equivalent Results should be equal
    RixType resultType2 = RixType::result(RixType::matrix(Tag{"Vector", false}, DimExpr::constant(1), N));
    assert(resultType == resultType2);

    std::cout << "\nall checks passed\n";
    return 0;
}
