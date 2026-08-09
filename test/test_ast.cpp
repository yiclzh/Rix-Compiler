#include "../src/ast.hpp"
#include <cassert>
#include <iostream>
#include <memory>

using namespace rix;

// Builds the AST for, roughly:
//
//   func compute_var(returns: Matrix<Returns,T,N>, confidence: Scalar)
//       -> Result<Matrix<Vector,1,N>, Error>
//   {
//       let vol: Matrix<Vector,1,N> = std(returns);
//       if (confidence > 1.0) {
//           return Err("confidence must be <= 1.0");
//       } else {
//           return Ok(vol * confidence);
//       }
//   }
int main() {
    DimExpr T = DimExpr::var("T");
    DimExpr N = DimExpr::var("N");

    // let vol: Matrix<Vector,1,N> = std(returns);
    auto returnsArg = std::make_shared<Expr>(Expr::Variant{Identifier{"returns", 3}});
    auto stdCall = std::make_shared<Expr>(Expr::Variant{
        CallExpr{"std", {}, {returnsArg}, 3}
    });
    Statement letVol{Statement::Variant{
        LetStatement{
            "vol",
            /*isMut=*/false,
            RixType::matrix(Tag{"Vector"}, DimExpr::constant(1), N),
            stdCall,
            /*line=*/3
        }
    }};

    // if (confidence > 1.0) { return Err(...); } else { return Ok(...); }
    auto confidenceId = std::make_shared<Expr>(Expr::Variant{Identifier{"confidence", 4}});
    auto oneLiteral = std::make_shared<Expr>(Expr::Variant{FloatLiteral{1.0, 4}});
    auto condition = std::make_shared<Expr>(Expr::Variant{
        BinaryExpr{">", confidenceId, oneLiteral, 4}
    });

    auto errMsg = std::make_shared<Expr>(Expr::Variant{
        StringLiteral{"confidence must be <= 1.0", 5}
    });
    auto errCall = std::make_shared<Expr>(Expr::Variant{
        CallExpr{"Err", {}, {errMsg}, 5}
    });
    auto thenBlock = std::make_shared<Block>();
    thenBlock->statements.push_back(Statement{Statement::Variant{
        ReturnStatement{errCall, 5}
    }});

    auto volId = std::make_shared<Expr>(Expr::Variant{Identifier{"vol", 7}});
    auto confidenceId2 = std::make_shared<Expr>(Expr::Variant{Identifier{"confidence", 7}});
    auto scaled = std::make_shared<Expr>(Expr::Variant{
        BinaryExpr{"*", volId, confidenceId2, 7}
    });
    auto okCall = std::make_shared<Expr>(Expr::Variant{
        CallExpr{"Ok", {}, {scaled}, 7}
    });
    auto elseBlock = std::make_shared<Block>();
    elseBlock->statements.push_back(Statement{Statement::Variant{
        ReturnStatement{okCall, 7}
    }});

    Statement ifStmt{Statement::Variant{
        IfStatement{condition, thenBlock, elseBlock, 4}
    }};

    // Assemble the function
    FuncDecl compute_var;
    compute_var.name = "compute_var";
    compute_var.params = {
        Param{"returns", RixType::matrix(Tag{"Returns"}, T, N)},
        Param{"confidence", RixType::scalar()},
    };
    compute_var.returnType = RixType::result(RixType::matrix(Tag{"Vector"}, DimExpr::constant(1), N));
    compute_var.body.statements = {letVol, ifStmt};
    compute_var.line = 1;

    // --- Checks ---
    std::cout << "func " << compute_var.name << "(...) -> "
              << compute_var.returnType.toString() << " {\n";
    for (const auto& stmt : compute_var.body.statements) {
        std::cout << "    [line " << stmt.line() << "] " << stmt.toString() << "\n";
    }
    std::cout << "}\n\n";

    assert(compute_var.params.size() == 2);
    assert(compute_var.params[0].type.is<MatrixType>());
    assert(compute_var.params[0].type.as<MatrixType>().tag.name == "Returns");
    assert(compute_var.params[1].type.is<ScalarType>());
    assert(compute_var.returnType.is<ResultType>());

    assert(compute_var.body.statements.size() == 2);
    assert(compute_var.body.statements[0].is<LetStatement>());
    assert(compute_var.body.statements[0].as<LetStatement>().name == "vol");
    assert(compute_var.body.statements[0].line() == 3);

    const auto& ifNode = compute_var.body.statements[1].as<IfStatement>();
    assert(ifNode.condition->is<BinaryExpr>());
    assert(ifNode.condition->as<BinaryExpr>().op == ">");
    assert(ifNode.thenBlock->statements.size() == 1);
    assert(ifNode.elseBlock != nullptr);
    assert(ifNode.elseBlock->statements[0].as<ReturnStatement>().value->as<CallExpr>().funcName == "Ok");

    std::cout << "all checks passed\n";
    return 0;
}
