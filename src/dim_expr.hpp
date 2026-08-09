#pragma once
#include <map>
#include <set>
#include <string>

namespace rix {

class DimExpr {
    public:
        DimExpr(int constantTerm, std::map<std::string, int> coeffs);
        static DimExpr constant(int n);
        static DimExpr var (const std::string& name);

        bool isConstant() const;
        std::set<std::string> freeVars() const;

        DimExpr operator+(const DimExpr& other) const;
        DimExpr operator-(const DimExpr& other) const;
        DimExpr scale(int k) const;

        // substitute expression's variables into T - n + 1
        DimExpr substitute(const std::map<std::string, DimExpr>& bindings) const;

        // needed to compare two DimExpr for equivalence
        bool operator==(const DimExpr& other) const;
        bool operator!=(const DimExpr& other) const;

        std::string toString() const;
        int constantTerm() const;
        const std::map<std::string, int>& coefficients() const;

        private:
            int const_;
            std::map<std::string, int> coeffs_; // never contains a 0

            // initialize map, returns cleaned up map
            static std::map<std::string, int> pruneZeros(std::map<std::string, int> m);


};

}