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

namespace cstar {

    class Program;

#define NODE_STMT_LIST(XX)              \
    XX(If)                              \
    XX(Block)                           \
    XX(Break)                           \
    XX(Continue)                        \
    XX(For)                             \
    XX(Var)                             \
    XX(Vars)                            \
    XX(Class)                           \
    XX(Function)                        \
    XX(Return)                          \
    XX(Expression)                      \
    XX(Print)                           \

#define NODE_EXPR_LIST(XX)              \
    XX(Assignment)                      \
    XX(Binary)                          \
    XX(Unary)                           \
    XX(Call)                            \
    XX(Get)                             \
    XX(Set)                             \
    XX(Group)                           \
    XX(Lambda)                          \
    XX(Tuple)                           \
    XX(New)                             \
    XX(Brace)                           \
    XX(This)                            \
    XX(Super)                           \
    XX(Variable)                        \
    XX(Literal)                         \
    XX(Bool)                            \
    XX(Char)                            \
    XX(Integer)                         \
    XX(Float)                           \
    XX(String)                          \
    \

#define NODE_LIST(XX)                   \
    XX(Node)                            \
    XX(Stmt)                            \
    XX(Expr)                            \
    XX(ExpList)                         \
    XX(ParamDecl)                       \
    \


#define XX(N) struct N;
    NODE_LIST(XX)
#undef XX

#define XX(N) struct N##Stmt;
    NODE_STMT_LIST(XX)
#undef XX

#define XX(N) struct N##Expr;
    NODE_EXPR_LIST(XX)
#undef XX

    class Visitor {
    public:
#define XX(N) virtual void visit(N &node) { }
        NODE_LIST(XX)
#undef XX

#define XX(N) virtual void visit(N##Stmt &node) { }
        NODE_STMT_LIST(XX)
#undef XX

#define XX(N) virtual void visit(N##Expr &node) { }
        NODE_EXPR_LIST(XX)
#undef XX
    };

#define VisitableNode() virtual void accept(Visitor& visitor) override { visitor.visit(*this); }
}