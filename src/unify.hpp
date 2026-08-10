#pragma once
#include <set>
#include <stdexcept>
#include <string>

#include "dim_expr.hpp"
#include "rix_type.hpp"
#include "tag.hpp"

namespace rix {

struct UnifyError : std::runtime_error {
    explicit UnifyError(const std::string& msg) : std::runtime_error(msg) {}
};

class Subst {
public:
    DimExpr resolveDim(const DimExpr& d) const;
    Tag resolveTag(const Tag& t) const;

    void bindDim(const std::string& var, const DimExpr& value);
    void bindTag(const std::string& var, const Tag& value);

    bool hasDim(const std::string& var) const;
    bool hasTag(const std::string& var) const;

private:
    std::map<std::string, DimExpr> dimBindings_;
    std::map<std::string, Tag> tagBindings_;
};

void unifyDim(const DimExpr& pattern, const DimExpr& value, Subst& subst);

void unifyTag(const Tag& pattern, const Tag& value, Subst& subst);

void unifyMatrixType(const MatrixType& pattern, const MatrixType& value, Subst& subst);

} 
