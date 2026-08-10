#include "../src/parser.hpp"
#include <cassert>
#include <iostream>

using namespace rix;

static const char* SOURCE = R"(
func compute_var(returns: Matrix<Returns,T,N>, confidence: Scalar) -> Result<Matrix<Vector,1,N>, Error> {
    let vol: Matrix<Vector,1,N> = std(returns);
    if (confidence > 1.0) {
        return Err("confidence must be <= 1.0");
    } else {
        return Ok(vol * confidence);
    }
}

func main() -> Void {
    let mut count: Int = 0;
    for i in 0..5 {
        count = count + 1;
    }
    let rv: Matrix<Returns, 192, 10> = rolling<60>(returns, std);
    return;
}
)";

int main() {
    Tokenizer tok(SOURCE);
    Parser parser(tok);
    ProgramNode program = parser.parseProgram();

    std::cout << "parsed " << program.funcs.size() << " functions\n\n";
    assert(program.funcs.size() == 2);

    // --- compute_var ---
    const FuncDecl& cv = program.funcs[0];
    std::cout << "func " << cv.name << " -> " << cv.returnType.toString() << " {\n";
    for (const auto& stmt : cv.body.statements) {
        std::cout << "    [line " << stmt.line() << "] " << stmt.toString() << "\n";
    }
    std::cout << "}\n\n";

    assert(cv.name == "compute_var");
    assert(cv.params.size() == 2);
    assert(cv.params[0].name == "returns");
    assert(cv.params[0].type.is<MatrixType>());
    assert(cv.params[0].type.as<MatrixType>().tag.name == "Returns");
    assert(cv.params[1].type.is<ScalarType>());
    assert(cv.returnType.is<ResultType>());

    assert(cv.body.statements.size() == 2);
    assert(cv.body.statements[0].is<LetStatement>());
    assert(cv.body.statements[0].as<LetStatement>().name == "vol");

    const auto& ifNode = cv.body.statements[1].as<IfStatement>();
    assert(ifNode.condition->is<BinaryExpr>());
    assert(ifNode.condition->as<BinaryExpr>().op == ">");
    assert(ifNode.elseBlock != nullptr);
    assert(ifNode.elseBlock->statements[0].as<ReturnStatement>().value->as<CallExpr>().funcName == "Ok");

    // --- main: exercises mut/assign, for, and rolling<n> generic call ---
    const FuncDecl& mainFn = program.funcs[1];
    std::cout << "func " << mainFn.name << " -> " << mainFn.returnType.toString() << " {\n";
    for (const auto& stmt : mainFn.body.statements) {
        std::cout << "    [line " << stmt.line() << "] " << stmt.toString() << "\n";
    }
    std::cout << "}\n\n";

    assert(mainFn.body.statements.size() == 4);
    assert(mainFn.body.statements[0].as<LetStatement>().isMut == true);

    const auto& forNode = mainFn.body.statements[1].as<ForStatement>();
    assert(forNode.varName == "i");
    assert(forNode.start == DimExpr::constant(0));
    assert(forNode.end == DimExpr::constant(5));
    assert(forNode.body->statements.size() == 1);
    assert(forNode.body->statements[0].is<AssignStatement>());

    const auto& rollingLet = mainFn.body.statements[2].as<LetStatement>();
    assert(rollingLet.value->is<CallExpr>());
    const auto& rollingCall = rollingLet.value->as<CallExpr>();
    assert(rollingCall.funcName == "rolling");
    assert(rollingCall.templateArgs.size() == 1);
    assert(rollingCall.templateArgs[0] == DimExpr::constant(60));
    assert(rollingCall.args.size() == 2);

    assert(mainFn.body.statements[3].is<ReturnStatement>());
    assert(mainFn.body.statements[3].as<ReturnStatement>().value == nullptr);

    std::cout << "all checks passed\n";
    return 0;
}
