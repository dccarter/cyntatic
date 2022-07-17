/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-06-15
 */

#pragma once

#include <utility>
#include <compiler/utils.hpp>
#include <unordered_map>

namespace cyn {

#define OBJECT_KIND_LIST(XX)                \
    XX(Var)                                 \
    XX(Type)                                \
    XX(Func)                                \


    struct Object {
    public:
        CSTAR_PTR(Object);
        typedef enum {
#define XX(N) obj##N,
            OBJECT_KIND_LIST(XX)
#undef XX
        } Kind;

    public:
        Kind objKind{};

    public:
        Object(Kind kind)
            : objKind{kind}
        {}

        virtual ~Object() = default;
    };

    class SymbolTable {
    public:
        CSTAR_PTR(SymbolTable);

    public:
        static constexpr int MAX_LOOKUP_DEPTH = 500;

    public:
        SymbolTable(SymbolTable::Ptr enclosing = nullptr)
            : _enclosing{std::move(enclosing)}
        {}

        bool define(std::string_view name, Object::Ptr obj);
        Object::Ptr lookup(std::string_view name, int depth = MAX_LOOKUP_DEPTH);
        template <typename T>
            requires std::is_base_of_v<Object, T>
        auto lookup(std::string_view name, int depth = MAX_LOOKUP_DEPTH) {
            return std::dynamic_pointer_cast<T>(lookup(name, depth));
        }
        bool assign(std::string_view name, Object::Ptr  obj);
        SymbolTable::Ptr& enclosing() { return _enclosing; }
    private:
        using SymbolsMap = std::unordered_map<std::string_view, Object::Ptr>;

        SymbolsMap   _symbols{};
        SymbolTable::Ptr _enclosing{};
    };

}