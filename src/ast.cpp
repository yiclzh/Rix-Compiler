#include "ast.hpp"
#include <sstream>
#include <type_traits>

namespace rix {

int Expr::line() const {
    return std::visit([](const auto& e) { return e.line; }, value_);
}

std::string Expr::toString() const {
    return std::visit([](const auto& e) -> std::string {
        using T = std::decay_t<decltype(e)>;

        if constexpr (std::is_same_v<T, IntLiteral>) {
            return std::to_string(e.value);
        } else if constexpr (std::is_same_v<T, FloatLiteral>) {
            return std::to_string(e.value);
        } else if constexpr (std::is_same_v<T, StringLiteral>) {
            return "\"" + e.value + "\"";
        } else if constexpr (std::is_same_v<T, BoolLiteral>) {
            return e.value ? "true" : "false";
        } else if constexpr (std::is_same_v<T, Identifier>) {
            return e.name;
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            std::string s = e.funcName;
            if (!e.templateArgs.empty()) {
                s += "<";
                for (size_t i = 0; i < e.templateArgs.size(); ++i) {
                    if (i) s += ", ";
                    s += e.templateArgs[i].toString();
                }
                s += ">";
            }
            s += "(";
            for (size_t i = 0; i < e.args.size(); ++i) {
                if (i) s += ", ";
                s += e.args[i]->toString();
            }
            s += ")";
            return s;
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            return "(" + e.lhs->toString() + " " + e.op + " " + e.rhs->toString() + ")";
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            return e.op + e.operand->toString();
        } else if constexpr (std::is_same_v<T, StructLiteral>) {
            std::string s = e.structName + " { ";
            for (size_t i = 0; i < e.fields.size(); ++i) {
                if (i) s += ", ";
                s += e.fields[i].first + ": " + e.fields[i].second->toString();
            }
            s += " }";
            return s;
        }
    }, value_);
}

int Statement::line() const {
    return std::visit([](const auto& s) { return s.line; }, value_);
}

std::string Statement::toString() const {
    return std::visit([](const auto& s) -> std::string {
        using T = std::decay_t<decltype(s)>;

        if constexpr (std::is_same_v<T, LetStatement>) {
            std::string t = s.declaredType ? (": " + s.declaredType->toString()) : "";
            return "let " + std::string(s.isMut ? "mut " : "") + s.name + t
                 + " = " + s.value->toString() + ";";
        } else if constexpr (std::is_same_v<T, AssignStatement>) {
            return s.name + " = " + s.value->toString() + ";";
        } else if constexpr (std::is_same_v<T, IfStatement>) {
            std::string out = "if (" + s.condition->toString() + ") { ... }";
            if (s.elseBlock) out += " else { ... }";
            return out;
        } else if constexpr (std::is_same_v<T, ForStatement>) {
            return "for " + s.varName + " in " + s.start.toString()
                 + ".." + s.end.toString() + " { ... }";
        } else if constexpr (std::is_same_v<T, ReturnStatement>) {
            return s.value ? ("return " + s.value->toString() + ";") : "return;";
        } else if constexpr (std::is_same_v<T, ExprStatement>) {
            return s.expr->toString() + ";";
        }
    }, value_);
}


}