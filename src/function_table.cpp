#include "function_table.hpp"

namespace rix {

namespace {

DimExpr renameDim(const DimExpr& d, const std::string& suffix) {
    std::map<std::string, int> renamed;
    for (const auto& [name, coeff] : d.coefficients()) {
        renamed[name + suffix] = coeff;
    }
    return DimExpr(d.constantTerm(), renamed);
}

Tag renameTag(const Tag& t, const std::string& suffix) {
    if (!t.isVar) return t;   // concrete tags (Prices, Returns, ...) never rename
    return Tag{t.name + suffix, true};
}

RixType renameType(const RixType& t, const std::string& suffix) {
    if (t.is<MatrixType>()) {
        const auto& m = t.as<MatrixType>();
        return RixType::matrix(renameTag(m.tag, suffix), renameDim(m.rows, suffix), renameDim(m.cols, suffix));
    }
    if (t.is<ResultType>()) {
        const auto& r = t.as<ResultType>();
        return RixType::result(renameType(*r.okType, suffix));
    }
    if (t.is<StructType>()) {
        const auto& s = t.as<StructType>();
        std::vector<DimExpr> renamedArgs;
        for (const auto& a : s.genericArgs) renamedArgs.push_back(renameDim(a, suffix));
        return RixType::structType(s.name, renamedArgs);
    }
    return t;   // Scalar, Bool, Int, Nat, String, Void carry no variables
}

} // namespace

void FunctionTable::declare(const std::string& name, FunctionSig sig) {
    table_[name] = std::move(sig);
}

bool FunctionTable::exists(const std::string& name) const {
    return table_.count(name) > 0;
}

FunctionSig FunctionTable::instantiate(const std::string& name, int callId) const {
    auto it = table_.find(name);
    if (it == table_.end()) {
        throw FunctionError("unknown function '" + name + "'");
    }
    const FunctionSig& sig = it->second;
    std::string suffix = "@" + std::to_string(callId);

    FunctionSig fresh;
    fresh.name = sig.name;
    for (const auto& p : sig.params) {
        fresh.params.push_back(Param{p.name, renameType(p.type, suffix)});
    }
    fresh.returnType = renameType(sig.returnType, suffix);
    return fresh;
}

std::optional<std::string> FunctionTable::suggestFix(const std::string& badTag, const std::string& wantedTag) const {
    for (const auto& [name, sig] : table_) {
        if (sig.params.size() != 1) continue;
        if (!sig.params[0].type.is<MatrixType>()) continue;
        if (!sig.returnType.is<MatrixType>()) continue;
        if (sig.params[0].type.as<MatrixType>().tag.name == badTag &&
            sig.returnType.as<MatrixType>().tag.name == wantedTag) {
            return name;
        }
    }
    return std::nullopt;
}

} 
