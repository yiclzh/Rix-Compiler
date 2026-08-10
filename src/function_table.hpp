#pragma once
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "ast.hpp"      
#include "rix_type.hpp"

namespace rix {

struct FunctionError : std::runtime_error {
    explicit FunctionError(const std::string& msg) : std::runtime_error(msg) {}
};

struct FunctionSig {
    std::string name;
    std::vector<Param> params;   // may reference free dim/tag vars, e.g. T, N, t
    RixType returnType;
};

// Registry of every callable function -- stdlib (cov, pct_change, ...) and,
// later, user-defined functions parsed from a .rix file.
class FunctionTable {
public:
    void declare(const std::string& name, FunctionSig sig);
    bool exists(const std::string& name) const;

    // Fresh-renames every free dim/tag variable in the signature (T -> T@3,
    // t -> t@3, ...) so this call site's bindings can't collide with a
    // different call to the same generic function elsewhere. callId should
    // be unique per call site -- see typecheck_call in unify.py for the
    // original version of this idea.
    FunctionSig instantiate(const std::string& name, int callId) const;

    // Powers the "did you mean pct_change?" note: finds a single-argument
    // function whose param tag is badTag and whose return tag is wantedTag.
    std::optional<std::string> suggestFix(const std::string& badTag, const std::string& wantedTag) const;

private:
    std::map<std::string, FunctionSig> table_;
};

} 
