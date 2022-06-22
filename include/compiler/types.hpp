/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-06-14
 */

#pragma once

#include <compiler/symbol.hpp>
#include <compiler/utils.hpp>
#include <unordered_map>

namespace cstar {

#define TYPE_KIND_LIST(XX)          \
    XX(Builtin)                     \
    XX(Function)                    \
    XX(Class)                       \
    XX(Tuple)                       \
    XX(Array)                       \
    \

    class Type : public Object {
    public:
        CSTAR_PTR(Type);
        typedef enum {
#define XX(N) tp##N,
            TYPE_KIND_LIST(XX)
#undef XX
        } Kind;
        Kind kind{tpBuiltin};

        Type(Kind kind)
            : Object(objType),
              kind{kind}
        {}

        virtual ~Type() = default;

        virtual bool isAssignable(const Type::Ptr from);

        virtual std::string_view name() const { return ""; }

        static Type::Ptr leastUpperBound(Type::Ptr t1, Type::Ptr t2);
    };

#define BUILTIN_TYPE_IDS(XX)            \
    XX(Void)                            \
    XX(Null)                            \
    XX(Bool)                            \
    XX(Char)                            \
    XX(Integer)                         \
    XX(Float)                           \
    XX(String)                          \
    \

    class BuiltinType : public Type {
    public:
        typedef enum {
#define XX(N) tid##N,
            BUILTIN_TYPE_IDS(XX)
#undef XX
        } Id;

        CSTAR_PTR(BuiltinType);
        Id id{tidVoid};

        BuiltinType() : Type(tpBuiltin), _name{"void"} {}
        BuiltinType(Id tid, std::string name) : Type(tpBuiltin), _name{std::move(name)} {}

        std::string_view name() const override { return _name; }

    private:
        std::string _name{};
    };

    class BoolType : public BuiltinType {
    public:
        CSTAR_PTR(BoolType);
        BoolType() : BuiltinType(tidBool, "bool") {}
    };

    class CharType : public BuiltinType {
    public:
        CSTAR_PTR(CharType);
        CharType() : BuiltinType(tidBool, "char") {}
    };

    class StringType : public BuiltinType {
    public:
        CSTAR_PTR(StringType);
        StringType() : BuiltinType(tidString, "string") {}
    };

    class IntegerType : public BuiltinType {
    public:
        CSTAR_PTR(IntegerType);
        IntegerType(std::string name, uint8_t bits, bool isSigned)
            : BuiltinType(tidInteger, std::move(name)), bits{bits}, isSigned{isSigned}
        {}

        static IntegerType::Ptr biggerType(IntegerType::Ptr i1, IntegerType::Ptr i2);

        uint8_t bits{0};
        bool    isSigned{true};
    };

    class FloatType : public BuiltinType {
    public:
        CSTAR_PTR(FloatType);
        FloatType(std::string name, uint8_t bits)
            : BuiltinType(tidFloat, std::move(name)), bits{bits}
        {}

        uint8_t bits{0};
    };

    struct FunctionType : public Type {
    public:
        CSTAR_PTR(FunctionType);

        FunctionType(vec<Type::Ptr> returnTypes, vec<Type::Ptr> parameters, std::string name = {})
            : Type(tpFunction),
              returnTypes{std::move(returnTypes)},
              parameters{std::move(parameters)},
              _name{std::move(name)}
        {}

        std::string_view name() const override { return _name; }

        vec<Type::Ptr> returnTypes{};
        vec<Type::Ptr> parameters;

    private:
        std::string _name{};
    };

    struct ClassType : public Type {
    public:
        using Fields = std::unordered_map<std::string_view, Type::Ptr>;
        using Methods = std::unordered_map<std::string_view, FunctionType::Ptr>;

        CSTAR_PTR(ClassType);

        ClassType(std::string name)
            : Type(tpClass), _name{std::move(name)}
        {}

        ClassType::Ptr base{};
        Fields fields{};
        Methods methods{};

        std::string_view name() const override { return _name; }
        Type::Ptr get(const std::string_view& member);

        bool isAssignable(const Type::Ptr from) override;

        const vec<ClassType::Ptr>& getInheritanceHierarchy();

    private:
        void getInheritanceHierarchy(vec<ClassType::Ptr>& dest);
        std::string _name{};
        std::vector<ClassType::Ptr> _inheritanceHierarchy{};
    };

    class TupleType : public Type {
    public:
        CSTAR_PTR(TupleType);
        TupleType(vec<Type::Ptr> members = {})
            : Type(tpTuple), members{std::move(members)}
        {}

        vec<Type::Ptr> members{};

        std::string_view name() const override { return "tuple"; }
        bool isAssignable(const Type::Ptr from) override;
    };

    class ArrayType : public Type {
    public:
        CSTAR_PTR(ArrayType);
        ArrayType(Type::Ptr member, size_t size)
            : Type(tpArray), member{std::move(member)}, size{size}
        {}

        vec<Type::Ptr> member{};
        size_t size{0};

        std::string_view name() const override { return "tuple"; }
        bool isAssignable(const Type::Ptr from) override;
    };

    namespace builtin {
        BuiltinType::Ptr voidType();
        BuiltinType::Ptr nullType();
        BuiltinType::Ptr booleanType();
        BuiltinType::Ptr charType();
        BuiltinType::Ptr i8Type();
        BuiltinType::Ptr u8Type();
        BuiltinType::Ptr i16Type();
        BuiltinType::Ptr u16Type();
        BuiltinType::Ptr i32Type();
        BuiltinType::Ptr u32Type();
        BuiltinType::Ptr i64Type();
        BuiltinType::Ptr u64Type();
        BuiltinType::Ptr f32Type();
        BuiltinType::Ptr f64Type();
        BuiltinType::Ptr stringType();
    }
}