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

namespace cyn {
struct Type;

#define CYN_VISITABLE_EXPRS(XX)             \
    XX(Expression)                          \
    XX(Null)                                \
    XX(Bool)                                \
    XX(Char)                                \
    XX(Integer)                             \
    XX(Float)                               \
    XX(String)                              \
    XX(Variable)                            \
    XX(StrExp)                              \
    XX(Binary)                              \
    XX(Range)                               \
    XX(Unary)                               \
    XX(Refer)                               \
    XX(Deref)                               \
    XX(New)                                 \
    XX(Free)                                \
    XX(Cast)                                \
    XX(Call)                                \
    XX(MethodCall)                          \
    XX(Index)                               \
    XX(Conditional)

#define XX(N) class N##Expr;
    CYN_VISITABLE_EXPRS(XX)
#undef XX


#define CYN_VISITABLE_STMTS(XX)             \
    XX(Statement)                           \
    XX(Define)                              \
    XX(Block)                               \
    XX(Assignment)                          \
    XX(OpAssignment)                        \
    XX(For)                                 \
    XX(ForRange)                            \
    XX(While)                               \
    XX(If)                                  \
    XX(Return)                              \
    XX(Expression)                          \

#define XX(N) class N##Stmt;
    CYN_VISITABLE_STMTS(XX)
#undef XX

#define CYN_VISITABLE_DECLS(XX)             \
    XX(Declaration)                         \
    XX(Constant)                            \
    XX(Function)                            \
    XX(Structure)                           \
    XX(Assert)                              \
    XX(If)                                  \
    XX(For)                                 \
    XX(Error)                               \
    XX(Message)                             \
    XX(Extern)                              \
    XX(Include)

#define XX(N) class N##Decl;
    CYN_VISITABLE_DECLS(XX)
#undef XX

    class AstNode;

    class Visitor {
    public:
        virtual void visit(AstNode& node) { }

#define XX(N) virtual void visit(N##Expr& node) { }
        CYN_VISITABLE_EXPRS(XX)
#undef XX

#define XX(N) virtual void visit(N##Stmt& node) { }
        CYN_VISITABLE_STMTS(XX)
#undef XX

#define XX(N) virtual void visit(N##Decl& node) { }
        CYN_VISITABLE_DECLS(XX)
#undef XX

    };

#define CynMakeVisitable() virtual void accept(Visitor& visitor) override { visitor.visit(*this); }

}