#include "unify.hpp"

namespace rix {

DimExpr Subst::resolveDim(const DimExpr& dIn) const {
    DimExpr d = dIn;
    std::set<std::string> seen;   // cycle guard, mirrors the Python prototype
    while (true) {
        std::set<std::string> freeBound;
        for (const auto& v : d.freeVars()) {
            if (dimBindings_.count(v)) freeBound.insert(v);
        }
        bool allSeen = true;
        for (const auto& v : freeBound) {
            if (!seen.count(v)) { allSeen = false; break; }
        }
        if (freeBound.empty() || allSeen) return d;

        d = d.substitute(dimBindings_);
        for (const auto& v : freeBound) seen.insert(v);
    }
}

Tag Subst::resolveTag(const Tag& tIn) const {
    Tag t = tIn;
    std::set<std::string> seen;
    while (t.isVar && tagBindings_.count(t.name) && !seen.count(t.name)) {
        seen.insert(t.name);
        t = tagBindings_.at(t.name);
    }
    return t;
}

void Subst::bindDim(const std::string& var, const DimExpr& value) { dimBindings_.insert_or_assign(var, value); }
void Subst::bindTag(const std::string& var, const Tag& value)     { tagBindings_.insert_or_assign(var, value); }
bool Subst::hasDim(const std::string& var) const { return dimBindings_.count(var) > 0; }
bool Subst::hasTag(const std::string& var) const { return tagBindings_.count(var) > 0; }

void unifyDim(const DimExpr& patternIn, const DimExpr& valueIn, Subst& subst) {
    DimExpr pattern = subst.resolveDim(patternIn);
    DimExpr value = subst.resolveDim(valueIn);

    if (pattern.isConstant()) {
        if (!value.isConstant() || pattern.constantTerm() != value.constantTerm()) {
            throw UnifyError("expected dimension " + pattern.toString() + ", got " + value.toString());
        }
        return;
    }

    auto free = pattern.freeVars();
    if (free.size() != 1) {
        throw UnifyError("cannot unify non-linear/multi-variable pattern " + pattern.toString());
    }
    const std::string& v = *free.begin();
    int coeff = pattern.coefficients().at(v);
    if (coeff != 1) {
        throw UnifyError("unsupported coefficient on '" + v + "' (only coeff=1 supported)");
    }

    // pattern == v + pattern.constantTerm()  =>  v = value - pattern.constantTerm()
    DimExpr solved = value - DimExpr::constant(pattern.constantTerm());

    if (subst.hasDim(v)) {
        DimExpr existing = subst.resolveDim(DimExpr::var(v));
        if (existing != subst.resolveDim(solved)) {
            throw UnifyError("inconsistent binding for '" + v + "': " +
                              existing.toString() + " vs " + solved.toString());
        }
    } else {
        subst.bindDim(v, solved);
    }
}

void unifyTag(const Tag& patternIn, const Tag& valueIn, Subst& subst) {
    Tag pattern = subst.resolveTag(patternIn);
    Tag value = subst.resolveTag(valueIn);

    if (pattern.name == "Untagged" || value.name == "Untagged") return;

    if (pattern.isVar) {
        if (subst.hasTag(pattern.name)) {
            Tag existing = subst.resolveTag(Tag{pattern.name, true});
            if (existing != value) {
                throw UnifyError("inconsistent tag for '" + pattern.name + "': " +
                                  existing.name + " vs " + value.name);
            }
        } else {
            subst.bindTag(pattern.name, value);
        }
        return;
    }

    if (value.isVar) {
        unifyTag(value, pattern, subst);
        return;
    }

    if (pattern.name != value.name) {
        throw UnifyError("tag mismatch: expected '" + pattern.name + "', got '" + value.name + "'");
    }
}

void unifyMatrixType(const MatrixType& pattern, const MatrixType& value, Subst& subst) {
    unifyTag(pattern.tag, value.tag, subst);
    unifyDim(pattern.rows, value.rows, subst);
    unifyDim(pattern.cols, value.cols, subst);
}

} 
