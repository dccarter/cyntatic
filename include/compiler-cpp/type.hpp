/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-23
 */

#pragma once

#include <compiler/utils.hpp>

#include <optional>

namespace cyn {

    class Statement;
    class AstNode;
    class Value;

    class Type {
    public:
        CYN_PTR(Type);

        typedef enum {
            tpVoid,
            tpBool,
            tpChar,
            tpInt,
            tpFloat,
            tpString,
            tpPointer,
            tpArray,
            tpTuple,
            tpEnum,
            tpFunction,
            tpStruct,
            tpUnion
        } Kind;

        Kind kind{};
        GenericFlags Flags{};

        Type(Kind kind = tpVoid) : kind{kind}
        {}

    public:
        virtual bool isMovable() const { return true; }
        virtual std::optional<u32> size() const { return std::nullopt; }
    };

    Type::Ptr voidType();

    class BoolType : public  Type {
    public:
        CYN_PTR(BoolType);

        BoolType() : Type(tpBool)
        {}

        std::optional<u32> size() const override { return sizeof(u8); }
    };

    BoolType::Ptr boolType();

    class CharType : public  Type {
    public:
        CYN_PTR(CharType);

        CharType() : Type(tpChar)
        {}

        std::optional<u32> size() const override { return sizeof(u32); }
    };

    CharType::Ptr charType();

    class IntegerType : public  Type {
    public:
        typedef enum : u8 {
            Int8  = 1,
            Int16 = 2,
            Int32 = 4,
            Int64 = 8,
        } Size;

        CYN_PTR(IntegerType);

        IntegerType(Size size, bool isSigned)
            : Type(tpInt),
              _size{size}, _signed{isSigned}
        {}

        std::optional<u32> size() const override { return _size; }
        bool isSigned() const { return _signed; }

    private:
        Size _size{Int32};
        bool _signed{true};
    };

    IntegerType::Ptr i8Type();
    IntegerType::Ptr u8Type();
    IntegerType::Ptr i16Type();
    IntegerType::Ptr u16Type();
    IntegerType::Ptr i32Type();
    IntegerType::Ptr u32Type();
    IntegerType::Ptr i64Type();
    IntegerType::Ptr u64Type();

    class FloatType : public  Type {
    public:
        typedef enum : u8 {
            Flt32 = 4,
            Flt64 = 8,
        } Size;

        CYN_PTR(FloatType);

        FloatType(Size size)
            : Type(tpFloat),
              _size{size}
        {}

        std::optional<u32> size() const override { return _size; }

    private:
        Size _size{Flt32};
    };

    FloatType::Ptr f32Type();
    FloatType::Ptr f64Type();

    class PointerType : public  Type {
    public:
        CYN_PTR(FloatType);
        Type::Ptr pointee{};

        PointerType(Type::Ptr pointee)
            : Type(tpPointer),
              pointee{std::move(pointee)}
        {}

        std::optional<u32> size() const override {
            return pointee? pointee->size() : std::nullopt;
        }
    };

    class ArrayType : public  Type {
    public:
        CYN_PTR(ArrayType);
        Type::Ptr    element{};
        vec<u32>     dimensions{};
        ptr<AstNode> declNode{};

        ArrayType(Type::Ptr element, vec<u32> sizes, ptr<AstNode> declNode = nullptr)
            : Type(tpArray),
              element{std::move(element)}, dimensions{std::move(sizes)}, declNode{std::move(declNode)}
        {}

        std::optional<u32> size() const override;
    };

    class TupleType : public  Type {
    public:
        CYN_PTR(TupleType);

        vec<Type::Ptr> members{};
        ptr<AstNode>   declNode{};

        TupleType(vec<Type::Ptr> members, ptr<AstNode> declNode = nullptr)
            : Type(tpTuple),
              members{std::move(members)}, declNode{std::move(declNode)}
        {}

        std::optional<u32> size() const override;
    };

    class EnumField {
    public:
        CYN_PTR(EnumField);

        Identifier   name{};
        i32          value{0};
        ptr<AstNode> declNode;

        EnumField(Identifier name, i32 value, ptr<AstNode> declNode)
            : name{name}, value{value}, declNode{std::move(declNode)}
        {}
    };

    class EnumType : public Type {
    public:
        CYN_PTR(EnumType);

        Identifier          name{};
        vec<EnumField::Ptr> fields{};
        Type::Ptr           base{};
        ptr<AstNode>        declNode{};

        EnumType(Identifier name, vec<EnumField::Ptr> fields, Type::Ptr base, ptr<AstNode> declNode)
            : Type(tpEnum),
              name{name}, fields{std::move(fields)}, base{std::move(base)}, declNode{std::move(declNode)}
        {}

        std::optional<u32> size() const override { return base? base->size() : std::nullopt; }
    };

    class FunctionType : public Type {
    public:
        CYN_PTR(FunctionType);

        Identifier     name{};
        ptr<AstNode>   protoDecl;
        ptr<AstNode>   bodyDecl{};

        FunctionType(Identifier name, ptr<AstNode> proto, ptr<AstNode> body)
            : Type(tpFunction),
              name{name}, protoDecl{std::move(proto)}, bodyDecl{std::move(body)}
        {}

        std::optional<u32> size() const override { return sizeof(u64); }
    };

    class StructField {
    public:
        CYN_PTR(StructField);

        Identifier      name{};
        Type::Ptr       type{};
        ptr<Value>      value{};
        ptr<AstNode>    declNode{};
        u32             offset{0};

        StructField(Identifier name, Type::Ptr type, ptr<AstNode> decl = nullptr)
            : name{name}, type{std::move(type)}, declNode{std::move(decl)}
        {}
    };

    class StructType : public Type {
    public:
        CYN_PTR(StructType);

        Identifier             name{};
        vec<FunctionType::Ptr> methods{};
        vec<StructField::Ptr>  fields{};
        ptr<AstNode>           declNode{};

        StructType(Identifier name,
                   vec<StructField::Ptr> fields,
                   vec<FunctionType::Ptr> methods,
                   ptr<AstNode> decl = {})
            : Type(tpStruct),
              name{name}, fields{std::move(fields)}, methods{std::move(methods)}, declNode{std::move(decl)}
        {}

        std::optional<u32> size() const override;
    };



    class UnionType: public Type {
    public:
        CYN_PTR(UnionType);

        Identifier             name{};
        vec<FunctionType::Ptr> methods{};
        vec<Type::Ptr>         types{};
        ptr<AstNode>           declNode{};

        UnionType(Identifier name, vec<Type::Ptr> types, vec<FunctionType::Ptr> methods, ptr<AstNode> decl = {})
            : Type(tpUnion),
              name{name}, types{std::move(types)}, methods{std::move(methods)}, declNode{std::move(decl)}
        {}

        UnionType(Identifier name, vec<Type::Ptr> types, ptr<AstNode> decl = {})
            : UnionType(name, std::move(types), {}, std::move(decl))
        {}

        std::optional<u32> size() const override;
    };

}