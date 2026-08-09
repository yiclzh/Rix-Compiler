#include "dim_expr.hpp"
#include <sstream>
#include <utility>


// DimExpr is how the compiler represents a matrix dimension like T, T-1 or 192 as data it can do math on.

namespace rix {

// map never contains 0
std::map<std::string, int> DimExpr::pruneZeros(std::map<std::string, int> m) {
    for (auto it = m.begin(); it != m.end(); ) {
        if (it->second == 0) it = m.erase(it);
        else ++it;
    }
    return m;
}


DimExpr::DimExpr(int constantTerm, std::map<std::string, int> coeffs) : 
const_(constantTerm), coeffs_(pruneZeros(std::move(coeffs))) {}

DimExpr DimExpr::constant(int n) {
    return DimExpr(n, {});
}

DimExpr DimExpr::var(const std::string& name) {
    return DimExpr(0, {{name, 1}});
}


bool DimExpr::isConstant() const {
    return coeffs_.empty();
}

std::set<std::string> DimExpr::freeVars() const {
    std::set<std::string> vars;
    for (const auto& [name, coeff] : coeffs_) vars.insert(name);
    return vars;
}

DimExpr DimExpr::operator+(const DimExpr& other) const {
    std::map<std::string, int> merged = coeffs_;
    for (const auto& [name, coeff] : other.coeffs_) {
        merged[name] += coeff;
    }

    return DimExpr(const_ + other.const_, std::move(merged));

}

DimExpr DimExpr::scale(int k) const {
    std::map<std::string, int> scaled;
    for (const auto& [name, coerff] : coeffs_) scaled[name] = coerff * k;
    return DimExpr(const_ * k, std::move(scaled));
}


DimExpr DimExpr::operator-(const DimExpr& other) const {
    return *this + other.scale(-1);
}

DimExpr DimExpr::substitute(const std::map<std::string, DimExpr>& bindings) const {
    DimExpr result = DimExpr::constant(const_);
    for (const auto& [name, coeff] : coeffs_) {
        auto it = bindings.find(name);
        if (it != bindings.end()) {
            result = result + it->second.scale(coeff);
        } else {
            result = result + DimExpr(0, {{name, coeff}});
        }
    }
    return result;
}


bool DimExpr::operator==(const DimExpr& other) const {
    return const_ == other.const_ && coeffs_ == other.coeffs_;
}

bool DimExpr::operator!=(const DimExpr& other) const {
    return !(*this == other);
}

std::string DimExpr::toString() const {
    if (isConstant()) return std::to_string(const_);

    std::ostringstream oss;
    bool first = true;
    for (const auto& [name, coeff] : coeffs_) {
        if (!first) oss << " + ";
        if (coeff == 1) oss << name;
        else oss << coeff << "*" << name;
        first = false;
    }
    if (const_ > 0) oss << "+" << const_;
    else if (const_ < 0) oss << " - " << -const_;
    return oss.str();
}

int DimExpr::constantTerm() const { return const_; }
const std::map<std::string, int>& DimExpr::coefficients() const { return coeffs_; }




}