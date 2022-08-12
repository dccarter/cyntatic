/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-08-12
 */

#pragma once

#include <vector.h>

#include <compiler/common/ident.h>
#include <compiler/common/source.h>
#include <compiler/common/token.h>

#include <compiler/type.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CYN_AST_NODES(XX, YY)   \
XX(Bool,            Lit)        \
XX(Char,            Lit)        \
XX(Integer,         Lit)        \
XX(Float,           Lit)        \
XX(String,          Lit)        \
YY(String,          Expr)       \
XX(Unary,           Expr)       \
XX(Binary,          Expr)       \
XX(Ternary,         Expr)       \
XX(Var,             Expr)       \
XX(Assign,          Expr)       \
XX(Ref,             Expr)       \
XX(Deref,           Expr)       \
XX(Member,          Expr)       \
XX(FuncCall,        Expr)       \
XX(Stmt,            Expr)       \
XX(Expr,            Stmt)       \
XX(Block,           Stmt)       \
XX(VarDecl,         Stmt)       \
XX(If,              Stmt)       \
XX(For,             Stmt)       \
XX(While,           Stmt)       \
XX(DoWhile,         Stmt)       \
XX(Switch,          Stmt)       \
XX(Case,            Stmt)       \
XX(Break,           Stmt)       \
XX(Continue,        Stmt)       \
XX(Func,            Decl)       \
XX(Struct,          Decl)       \

enum {
#define XX(N, E) ast##N##E,
    CYN_AST_NODES(XX, XX)
#undef XX

    astCOUNT
};

#define IAstNode struct {   \
    u8 id;                  \
    Range range;            \
    Type *type;             \
}

typedef IAstNode AstNodeInf;
typedef union AstNode AstNode;

typedef Vector(AstNode*) NodeList;

#define newLit(A, T, ...)   New(A, Ast##T##Lit, .id = ast##T##Lit, ##__VA__ARGS__ )
#define newExpr(A, T, ...)  New(A, Ast##T##Expr, .id = ast##T##Expr, ##__VA_ARGS__ )
#define newStmt(A, T, ...)  New(A, Ast##T##Stmt, .id = ast##T##Stmt, ##__VA_ARGS__ )
#define newDecl(A, T, ...)  New(A, Ast##T##Decl, .id = ast##T##Decl, ##__VA_ARGS__ )

typedef struct {
    IAstNode;
    bool value;
} AstBoolLit;

typedef struct {
    IAstNode;
    u32 value;
} AstCharLit;

typedef struct {
    IAstNode;
    u64 value;
} AstIntegerLit;

typedef struct {
    IAstNode;
    f64 value;
} AstFloatLit;

typedef struct {
    IAstNode;
    const char *value;
} AstStringLit;

typedef struct {
    IAstNode;
    NodeList exprs;
} AstStringExpr;

typedef struct {
    IAstNode;
    TokenKind op;
    AstNode  *expr;
} AstUnaryExpr;

typedef struct {
    IAstNode;
    TokenKind op;
    AstNode  *lhs;
    AstNode  *rhs;
} AstBinaryExpr;

typedef struct {
    IAstNode;
    AstNode  *cond;
    AstNode  *then;
    AstNode  *els;
} AstTernaryExpr;

typedef struct {
    IAstNode;
    Ident name;
} AstVarExpr;

typedef struct {
    IAstNode;
    AstNode *left;
    AstNode *right;
} AstAssignExpr;

typedef struct {
    IAstNode;
    AstNode *expr;
} AstRefExpr;

typedef struct {
    IAstNode;
    AstNode *expr;
} AstDerefExpr;

typedef struct {
    IAstNode;
    AstNode *obj;
    Ident    member;
} AstMemberExpr;

typedef struct {
    IAstNode;
    AstNode *func;
    NodeList args;
} AstFuncCallExpr;

typedef struct {
    IAstNode;
    AstNode *stmt;
} AstStmtExpr;

typedef struct {
    IAstNode;
    AstNode *expr;
} AstExprStmt;

typedef struct {
    IAstNode;
    NodeList stmts;
} AstBlockStmt;

typedef struct {
    IAstNode;
    Ident name;
    AstNode *init;
} AstVarDeclStmt;

typedef struct {
    IAstNode;
    AstNode *cond;
    AstNode *then;
    AstNode *els;
} AstIfStmt;

typedef struct {
    IAstNode;
    AstNode *init;
    AstNode *cond;
    AstNode *expr;
    AstNode *body;
} AstForStmt;

typedef struct {
    IAstNode;
    AstNode *cond;
    AstNode *body;
} AstWhileStmt;

typedef struct {
    IAstNode;
    AstNode *cond;
    AstNode *body;
} AstDoWhileStmt;

typedef struct {
    IAstNode;
    AstNode *cond;
    AstNode *body;
} AstCaseStmt;

typedef struct {
    IAstNode;
    AstNode *cond;
    NodeList *cases;
} AstSwitchStmt;

typedef struct {
    IAstNode;
} AstBreakStmt;

typedef struct {
    IAstNode;
} AstContinueStmt;


typedef struct {
    IAstNode;
    Ident name;
    NodeList params;
} AstFuncDecl;

typedef struct {
    IAstNode;
    Ident name;
    NodeList members;
} AstStructDecl;

union AstNode {
    IAstNode;

#define XX(N, E) Ast##N##E ast##N;
#define YY(N, E) Ast##N##E ast##N##E;
    CYN_AST_NODES(XX, YY)
#undef XX
};

typedef struct {
    NodeList nodes;
} AstModule;

#ifdef __cplusplus
}
#endif
