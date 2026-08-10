#pragma once
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "rix_type.hpp"

namespace rix {

struct SymbolError : std::runtime_error {
    explicit SymbolError(const std::string& msg) : std::runtime_error(msg) {}
};

// Tracks what's in scope as TypeChecker walks a function body

class SymbolTable {
public:
    SymbolTable();   

    void enterScope();
    void exitScope();

    void define(const std::string& name, const RixType& type, bool isMut);

    bool isDefined(const std::string& name) const;
    const RixType& typeOf(const std::string& name) const;   
    bool isMutable(const std::string& name) const;          

private:
    struct Entry {
        RixType type;
        bool isMut;
    };

    std::vector<std::map<std::string, Entry>> scopes_;

    const Entry& lookup(const std::string& name) const;  
};

} 
