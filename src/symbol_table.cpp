#include "symbol_table.hpp"

namespace rix {

SymbolTable::SymbolTable() {
    scopes_.emplace_back();  
}

void SymbolTable::enterScope() {
    scopes_.emplace_back();
}

void SymbolTable::exitScope() {
    if (scopes_.size() <= 1) {
        throw SymbolError("exitScope() called with no matching enterScope()");
    }
    scopes_.pop_back();
}

void SymbolTable::define(const std::string& name, const RixType& type, bool isMut) {
    scopes_.back()[name] = Entry{type, isMut};
}

const SymbolTable::Entry& SymbolTable::lookup(const std::string& name) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second;
    }
    throw SymbolError("undefined variable '" + name + "'");
}

bool SymbolTable::isDefined(const std::string& name) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        if (it->count(name)) return true;
    }
    return false;
}

const RixType& SymbolTable::typeOf(const std::string& name) const {
    return lookup(name).type;
}

bool SymbolTable::isMutable(const std::string& name) const {
    return lookup(name).isMut;
}

} 
