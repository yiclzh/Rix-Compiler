#include "rix_type.hpp"
#include <type_traits>

namespace rix {

RixType RixType::matrix(Tag tag, DimExpr rows, DimExpr cols) {
    return RixType(MatrixType{std::move(tag), std::move(rows), std::move(cols)});
}

RixType RixType::scalar() { return RixType(ScalarType{}); }
RixType RixType::boolean() { return RixType(BoolType{}); }
RixType RixType::integer() { return RixType(IntType{}); }
RixType RixType::nat() { return RixType(NatType{}); }
RixType RixType::string() { return RixType(StringType{}); }
RixType RixType::voidType() { return RixType(VoidType{}); }

RixType RixType::result(RixType ok) {
    return RixType(ResultType{std::make_shared<RixType>(std::move(ok))});
}

RixType RixType::structType(std::string name, std::vector<DimExpr> genericArgs) {
    return RixType(StructType{std::move(name), std::move(genericArgs)});
}

bool ResultType::operator==(const ResultType& other) const {
    if (okType && other.okType) return *okType == *other.okType;
    return okType == other.okType;
}

std::string RixType::toString() const {
    return std::visit([](const auto& t) -> std::string {
        using T = std::decay_t<decltype(t)>;

        if constexpr (std::is_same_v<T, MatrixType>) {
            return "Matrix<" + t.tag.name + ", " + t.rows.toString() + ", " + t.cols.toString() + ">";
        } else if constexpr (std::is_same_v<T, ScalarType>) {
            return "Scalar";
        } else if constexpr (std::is_same_v<T, BoolType>) {
            return "Bool";
        } else if constexpr (std::is_same_v<T, IntType>) {
            return "Int";
        } else if constexpr (std::is_same_v<T, NatType>) {
            return "Nat";
        } else if constexpr (std::is_same_v<T, StringType>) {
            return "String";
        } else if constexpr (std::is_same_v<T, VoidType>) {
            return "Void";
        } else if constexpr (std::is_same_v<T, ResultType>) {
            return "Result<" + (t.okType ? t.okType->toString() : std::string("?")) + ", Error>";
        } else if constexpr (std::is_same_v<T, StructType>) {
            std::string s = t.name;
            if (!t.genericArgs.empty()) {
                s += "<";
                for (size_t i = 0; i < t.genericArgs.size(); ++i) {
                    if (i) s += ", ";
                    s += t.genericArgs[i].toString();
                }
                s += ">";
            }
            return s;
        }


    }, value_);
}




}