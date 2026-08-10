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
    std::vector<Param> params;   
    RixType returnType;
    std::vector<std::string> constGenericParams;
};

class FunctionTable {
public:
    void declare(const std::string& name, FunctionSig sig);
    bool exists(const std::string& name) const;

    FunctionSig instantiate(const std::string& name, int callId) const;

    std::optional<std::string> suggestFix(const std::string& badTag, const std::string& wantedTag) const;

private:
    std::map<std::string, FunctionSig> table_;
};

} 
