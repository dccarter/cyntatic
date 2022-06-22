/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-06-15
 */

#include "compiler/symbol.hpp"

namespace cstar {

    bool SymbolTable::define(std::string_view name, Object::Ptr obj)
    {
        auto it = _symbols.find(name);
        if (it == _symbols.end()) {
            _symbols.emplace(name, std::move(obj));
            return true;
        }
        return false;
    }

    Object::Ptr SymbolTable::lookup(std::string_view name, int depth)
    {
        if (depth < 0) return nullptr;
        auto it = _symbols.find(name);
        if (it != _symbols.end())
            return it->second;
        if (_enclosing)
            return _enclosing->lookup(name, depth-1);
        return nullptr;
    }

    bool SymbolTable::assign(std::string_view name, Object::Ptr obj)
    {
        auto scope = this;
        for (auto i = 0; ((i <= MAX_LOOKUP_DEPTH) && (scope != nullptr)); i++) {
            auto it = scope->_symbols.find(name);
            if (it != scope->_symbols.end()) {
                it->second = std::move(obj);
                return true;
            }
        }
        return false;
    }
}