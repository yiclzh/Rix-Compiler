#include "type_checker.hpp"
#include "unify.hpp"

namespace rix {

namespace {

// Mirrors renameType's traversal (function_table.cpp) but substitutes bound
// values via a Subst instead of renaming -- this is how a call's return
// type (e.g. Matrix<CovMatrix, N@3, N@3>) becomes concrete (Matrix<CovMatrix,
// 3, 3>) once unification has bound N@3.
RixType resolveFully(const RixType& t, const Subst& subst) {
    if (t.is<MatrixType>()) {
        const auto& m = t.as<MatrixType>();
        return RixType::matrix(subst.resolveTag(m.tag), subst.resolveDim(m.rows), subst.resolveDim(m.cols));
    }
    if (t.is<ResultType>()) {
        const auto& r = t.as<ResultType>();
        return RixType::result(resolveFully(*r.okType, subst));
    }
    if (t.is<StructType>()) {
        const auto& s = t.as<StructType>();
        std::vector<DimExpr> args;
        for (const auto& a : s.genericArgs) args.push_back(subst.resolveDim(a));
        return RixType::structType(s.name, args);
    }
    return t;
}

} // namespace

void TypeChecker::checkProgram(const ProgramNode& program) {
    // First pass: register every user function's signature so calls can
    // reference functions regardless of declaration order (and so mutually
    // recursive functions work).
    for (const auto& f : program.funcs) {
        FunctionSig sig;
        sig.name = f.name;
        sig.params = f.params;
        sig.returnType = f.returnType;
        functions_.declare(f.name, sig);
    }

    for (const auto& s : program.structs) checkStructDecl(s);

    // Second pass: check each body now that every function is known.
    for (const auto& f : program.funcs) checkFuncDecl(f);
}

void TypeChecker::checkStructDecl(const StructDecl& /*decl*/) {
    // Not yet validated against a registered struct definition -- that needs
    // a StructTable mirroring FunctionTable (field name -> type, generic Nat
    // params). Struct *literals* are still checked field-by-field in
    // checkStructLiteral; this is specifically about validating the
    // declaration itself. Left as a known gap for the next pass.
}

void TypeChecker::checkFuncDecl(const FuncDecl& decl) {
    symbols_.enterScope();
    for (const auto& p : decl.params) {
        symbols_.define(p.name, p.type, /*isMut=*/false);
    }

    RixType savedReturn = currentReturnType_;
    currentReturnType_ = decl.returnType;

    checkBlock(decl.body);

    currentReturnType_ = savedReturn;
    symbols_.exitScope();
}

void TypeChecker::checkBlock(const Block& block) {
    symbols_.enterScope();
    for (const auto& stmt : block.statements) checkStatement(stmt);
    symbols_.exitScope();
}

void TypeChecker::checkStatement(const Statement& stmt) {
    if (stmt.is<LetStatement>())    { checkLet(stmt.as<LetStatement>()); return; }
    if (stmt.is<AssignStatement>()) { checkAssign(stmt.as<AssignStatement>()); return; }
    if (stmt.is<IfStatement>())     { checkIf(stmt.as<IfStatement>()); return; }
    if (stmt.is<ForStatement>())    { checkFor(stmt.as<ForStatement>()); return; }
    if (stmt.is<ReturnStatement>()) { checkReturn(stmt.as<ReturnStatement>()); return; }
    if (stmt.is<ExprStatement>())   { checkExprStatement(stmt.as<ExprStatement>()); return; }
}

void TypeChecker::checkLet(const LetStatement& s) {
    RixType valueType = checkExpr(*s.value);
    if (s.declaredType) {
        checkAssignable(*s.declaredType, valueType, s.line);
        symbols_.define(s.name, *s.declaredType, s.isMut);
    } else {
        symbols_.define(s.name, valueType, s.isMut);
    }
}

void TypeChecker::checkAssign(const AssignStatement& s) {
    if (!symbols_.isDefined(s.name)) {
        throw TypeError("undefined variable '" + s.name + "'", s.line);
    }
    if (!symbols_.isMutable(s.name)) {
        throw TypeError("cannot assign to '" + s.name + "' -- not declared 'mut'", s.line);
    }
    RixType valueType = checkExpr(*s.value);
    checkAssignable(symbols_.typeOf(s.name), valueType, s.line);
}

void TypeChecker::checkIf(const IfStatement& s) {
    RixType condType = checkExpr(*s.condition);
    if (!condType.is<BoolType>()) {
        throw TypeError("if condition must be Bool, got " + condType.toString(), s.line);
    }
    checkBlock(*s.thenBlock);
    if (s.elseBlock) checkBlock(*s.elseBlock);
}

void TypeChecker::checkFor(const ForStatement& s) {
    symbols_.enterScope();
    symbols_.define(s.varName, RixType::integer(), /*isMut=*/false);
    checkBlock(*s.body);
    symbols_.exitScope();
}

void TypeChecker::checkReturn(const ReturnStatement& s) {
    if (s.value) {
        RixType valueType = checkExpr(*s.value);
        checkAssignable(currentReturnType_, valueType, s.line);
    } else if (!currentReturnType_.is<VoidType>()) {
        throw TypeError("missing return value -- function returns " + currentReturnType_.toString(), s.line);
    }
}

void TypeChecker::checkExprStatement(const ExprStatement& s) {
    checkExpr(*s.expr);   // result discarded -- called for side effects, e.g. report(...)
}

RixType TypeChecker::checkExpr(const Expr& expr) {
    if (expr.is<IntLiteral>())    return RixType::integer();
    if (expr.is<FloatLiteral>())  return RixType::scalar();
    if (expr.is<StringLiteral>()) return RixType::string();
    if (expr.is<BoolLiteral>())   return RixType::boolean();

    if (expr.is<Identifier>()) {
        const auto& id = expr.as<Identifier>();
        if (!symbols_.isDefined(id.name)) {
            throw TypeError("undefined variable '" + id.name + "'", id.line);
        }
        return symbols_.typeOf(id.name);
    }

    if (expr.is<CallExpr>())      return checkCall(expr.as<CallExpr>(), expr.line());
    if (expr.is<BinaryExpr>())    return checkBinary(expr.as<BinaryExpr>(), expr.line());
    if (expr.is<UnaryExpr>())     return checkUnary(expr.as<UnaryExpr>(), expr.line());
    if (expr.is<StructLiteral>()) return checkStructLiteral(expr.as<StructLiteral>(), expr.line());

    throw TypeError("internal error: unrecognized expression kind", expr.line());
}

RixType TypeChecker::checkCall(const CallExpr& call, int line) {
    if (!functions_.exists(call.funcName)) {
        throw TypeError("unknown function '" + call.funcName + "'", line);
    }
    FunctionSig sig = functions_.instantiate(call.funcName, nextCallId_++);
    Subst subst;

    // Const-generic args (rolling<60>'s "60") bind directly -- they aren't
    // discovered through unification the way T/N are, since no *parameter*
    // ever mentions n by itself.
    if (sig.constGenericParams.size() != call.templateArgs.size()) {
        throw TypeError(call.funcName + "() expects " + std::to_string(sig.constGenericParams.size()) +
                         " template argument(s), got " + std::to_string(call.templateArgs.size()), line);
    }
    for (size_t i = 0; i < sig.constGenericParams.size(); ++i) {
        subst.bindDim(sig.constGenericParams[i], call.templateArgs[i]);
    }

    if (sig.params.size() != call.args.size()) {
        throw TypeError(call.funcName + "() expects " + std::to_string(sig.params.size()) +
                         " argument(s), got " + std::to_string(call.args.size()), line);
    }

    for (size_t i = 0; i < sig.params.size(); ++i) {
        RixType argType = checkExpr(*call.args[i]);
        const RixType& paramType = sig.params[i].type;

        if (paramType.is<MatrixType>() && argType.is<MatrixType>()) {
            try {
                unifyMatrixType(paramType.as<MatrixType>(), argType.as<MatrixType>(), subst);
            } catch (const UnifyError& e) {
                std::string msg = call.funcName + "(): argument " + std::to_string(i + 1) + ": " + e.what();

                const std::string& paramTag = paramType.as<MatrixType>().tag.name;
                const std::string& argTag = argType.as<MatrixType>().tag.name;
                if (paramTag != argTag) {
                    auto fix = functions_.suggestFix(argTag, paramTag);
                    if (fix) {
                        msg += "\nnote: " + argTag + " and " + paramTag +
                               " are distinct tags -- did you mean " + *fix + "(" +
                               call.args[i]->toString() + ")?";
                    }
                }
                throw TypeError(msg, line);
            }
        } else if (paramType != argType) {
            throw TypeError(call.funcName + "(): argument " + std::to_string(i + 1) +
                             ": expected " + paramType.toString() + ", got " + argType.toString(), line);
        }
    }

    return resolveFully(sig.returnType, subst);
}

RixType TypeChecker::checkBinary(const BinaryExpr& bin, int line) {
    RixType lhs = checkExpr(*bin.lhs);
    RixType rhs = checkExpr(*bin.rhs);

    if (bin.op == "==" || bin.op == "<" || bin.op == ">") {
        if (lhs != rhs) {
            throw TypeError("cannot compare " + lhs.toString() + " and " + rhs.toString(), line);
        }
        return RixType::boolean();
    }

    if (bin.op == "+" || bin.op == "-") {
        if (lhs.is<MatrixType>() && rhs.is<MatrixType>()) {
            Subst subst;
            try {
                unifyMatrixType(lhs.as<MatrixType>(), rhs.as<MatrixType>(), subst);
            } catch (const UnifyError& e) {
                throw TypeError(std::string("cannot apply '") + bin.op + "': " + e.what(), line);
            }
            return lhs;
        }
        if (lhs.is<ScalarType>() && rhs.is<ScalarType>()) return RixType::scalar();
        if (lhs.is<IntType>() && rhs.is<IntType>())       return RixType::integer();
        throw TypeError("cannot apply '" + bin.op + "' to " + lhs.toString() + " and " + rhs.toString(), line);
    }

    if (bin.op == "*" || bin.op == "/") {
        // Scalar broadcast is unambiguous in either order.
        if (lhs.is<MatrixType>() && rhs.is<ScalarType>()) return lhs;
        if (lhs.is<ScalarType>() && rhs.is<MatrixType>()) return rhs;
        if (lhs.is<ScalarType>() && rhs.is<ScalarType>()) return RixType::scalar();

        if (lhs.is<MatrixType>() && rhs.is<MatrixType>()) {
            // Matrix multiply: inner dimensions must agree (lhs.cols == rhs.rows).
            const auto& lm = lhs.as<MatrixType>();
            const auto& rm = rhs.as<MatrixType>();
            Subst subst;
            try {
                unifyDim(lm.cols, rm.rows, subst);
            } catch (const UnifyError& e) {
                throw TypeError("matrix multiply: inner dimensions don't match: " + std::string(e.what()), line);
            }
            // Result tag left Untagged for now -- matmul's output tag isn't
            // formalized yet (e.g. what a portfolio-weights-times-simulated-
            // returns product should be called); revisit once real stdlib
            // functions that produce one exist.
            return RixType::matrix(Tag{"Untagged"}, subst.resolveDim(lm.rows), subst.resolveDim(rm.cols));
        }
        throw TypeError("cannot apply '" + bin.op + "' to " + lhs.toString() + " and " + rhs.toString(), line);
    }

    if (bin.op == ".*" || bin.op == "./") {
        if (lhs.is<MatrixType>() && rhs.is<MatrixType>()) {
            Subst subst;
            try {
                unifyMatrixType(lhs.as<MatrixType>(), rhs.as<MatrixType>(), subst);
            } catch (const UnifyError& e) {
                throw TypeError("elementwise '" + bin.op + "' requires identical shapes: " + std::string(e.what()), line);
            }
            return lhs;
        }
        throw TypeError("'" + bin.op + "' requires two matrices of identical shape", line);
    }

    throw TypeError("unknown operator '" + bin.op + "'", line);
}

RixType TypeChecker::checkUnary(const UnaryExpr& un, int line) {
    RixType operandType = checkExpr(*un.operand);
    if (operandType.is<ScalarType>() || operandType.is<IntType>() || operandType.is<MatrixType>()) {
        return operandType;
    }
    throw TypeError("cannot apply unary '" + un.op + "' to " + operandType.toString(), line);
}

RixType TypeChecker::checkStructLiteral(const StructLiteral& lit, int /*line*/) {
    for (const auto& field : lit.fields) {
        checkExpr(*field.second);   // at least catch errors inside field expressions
    }
    // Not yet validated against a registered struct definition -- see the
    // note in checkStructDecl. Returns an unvalidated struct type for now.
    return RixType::structType(lit.structName, {});
}

void TypeChecker::checkAssignable(const RixType& declared, const RixType& actual, int line) {
    if (declared.is<MatrixType>() && actual.is<MatrixType>()) {
        Subst subst;
        try {
            unifyMatrixType(declared.as<MatrixType>(), actual.as<MatrixType>(), subst);
        } catch (const UnifyError& e) {
            throw TypeError(e.what(), line);
        }
        return;
    }
    if (declared != actual) {
        throw TypeError("expected " + declared.toString() + ", got " + actual.toString(), line);
    }
}

} 
