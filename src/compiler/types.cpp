/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-06-14
 */

#include "compiler/types.hpp"

namespace cstar {

    bool Type::isAssignable(const Type::Ptr from)
    {
        return this == from.get();
    }

    Type::Ptr Type::leastUpperBound(Type::Ptr t1, Type::Ptr t2)
    {
        if (t1 == t2) return t1;
        if (auto bt1 = std::dynamic_pointer_cast<BuiltinType>(t1)) {
            if (bt1->id == BuiltinType::tidNull) return t2;
        }
        if (auto bt2 = std::dynamic_pointer_cast<BuiltinType>(t2)) {
            if (bt2->id == BuiltinType::tidNull) return t1;
        }
        if (t1->kind == tpClass and t2->kind == tpClass) {
            auto h1 = std::dynamic_pointer_cast<ClassType>(t1)->getInheritanceHierarchy();
            auto h2 = std::dynamic_pointer_cast<ClassType>(t2)->getInheritanceHierarchy();
            for (auto& mi : h1) {
                for (auto& mj: h2) {
                    if (mi == mj)
                        return mi;
                }
            }
        }
        return nullptr;
    }

    bool ClassType::isAssignable(const Type::Ptr from)
    {
        if (Type::isAssignable(from)) return true;
        if (auto cf = std::dynamic_pointer_cast<ClassType>(from)) {
            auto super = base;
            while (super) {
                if (super == from)
                    return true;
                super = super->base;
            }
        }
        return false;
    }

    const vec<ClassType::Ptr>& ClassType::getInheritanceHierarchy()
    {
        if (_inheritanceHierarchy.empty())
            getInheritanceHierarchy(_inheritanceHierarchy);
        return _inheritanceHierarchy;
    }

    void ClassType::getInheritanceHierarchy(vec<ClassType::Ptr> &dest)
    {
        if (base != nullptr) {
            dest.push_back(base);
            base->getInheritanceHierarchy(dest);
        }
    }

    bool TupleType::isAssignable(const Type::Ptr from)
    {
        if (Type::isAssignable(from)) return true;
        if (auto tf = std::dynamic_pointer_cast<TupleType>(from)) {
            if (members.size() != tf->members.size()) return false;
            for (int i = 0; i < members.size(); i++) {
                if (!members[i]->isAssignable(tf->members[i])) return false;
            }
            return true;
        }
        return false;
    }

    Type::Ptr ClassType::get(const std::string_view& member)
    {
        auto fit = fields.find(member);
        if (fit != fields.end()) return fit->second;
        auto mit = methods.find(member);
        if (mit != methods.end()) return mit->second;
        return nullptr;
    }

#define BUILTIN_CREATE(I, N)                                            \
    static auto s##I##Type = mk<BuiltinType>(BuiltinType::tid##I, N);   \
    return s##I##Type \

    BuiltinType::Ptr builtin::voidType()
    {
        static auto sVoidType = mk<BuiltinType>();
        return sVoidType;
    }

    BuiltinType::Ptr builtin::nullType()
    {
        BUILTIN_CREATE(Null, "Null");
    }

#undef BUILTIN_CREATE

#define BUILTIN_CREATE(T, ...)                             \
    static auto s##T##Type = mk<T##Type>(__VA_ARGS__);     \
    return s##T##Type

    BuiltinType::Ptr builtin::booleanType()
    {
        BUILTIN_CREATE(Bool);
    }

    BuiltinType::Ptr builtin::charType()
    {
        BUILTIN_CREATE(Char);
    }

    BuiltinType::Ptr builtin::stringType()
    {
        BUILTIN_CREATE(String);
    }

#undef BUILTIN_CREATE

#define u false
#define i true

#define BUILTIN_CREATE(S, N)                                     \
    static auto s_##S##N##Type = mk<IntegerType>(#S#N, N, S);    \
    return s_##S##N##Type

    BuiltinType::Ptr builtin::i8Type()
    {
        BUILTIN_CREATE(i, 8);
    }

    BuiltinType::Ptr builtin::u8Type()
    {
        BUILTIN_CREATE(u, 8);
    }

    BuiltinType::Ptr builtin::i16Type()
    {
        BUILTIN_CREATE(i, 16);
    }

    BuiltinType::Ptr builtin::u16Type()
    {
        BUILTIN_CREATE(u, 16);
    }

    BuiltinType::Ptr builtin::i32Type()
    {
        BUILTIN_CREATE(i, 32);
    }

    BuiltinType::Ptr builtin::u32Type()
    {
        BUILTIN_CREATE(u, 32);
    }

    BuiltinType::Ptr builtin::i64Type()
    {
        BUILTIN_CREATE(i, 64);
    }

    BuiltinType::Ptr builtin::u64Type()
    {
        BUILTIN_CREATE(u, 64);
    }

#undef u
#undef i
#undef BUILTIN_CREATE

#define BUILTIN_CREATE(N)                                      \
    static auto s_Float##N##Type = mk<FloatType>("f"#N, N);    \
    return s_Float##N##Type

    BuiltinType::Ptr builtin::f32Type()
    {
        BUILTIN_CREATE(32);
    }

    BuiltinType::Ptr builtin::f64Type()
    {
        BUILTIN_CREATE(64);
    }
#undef BUILTIN_CREATE
}