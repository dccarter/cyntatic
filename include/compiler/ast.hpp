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

#include <compiler/token.hpp>
#include <compiler/types.hpp>
#include <compiler/vistor.hpp>

namespace cstar {

    struct Node {
    public:
        CSTAR_PTR(Node);

        Node(Range range = {}) : _range{std::move(range)} {}

        void range(Range range) { _range = std::move(range); }

        const Range &range() const { return _range; }

        virtual void accept(Visitor& visitor) { visitor.visit(*this); }

    private:
        Range _range{};
    };

    struct Expr : public Node {
    public:
        CSTAR_PTR(Expr);

        using Node::Node;

        void type(Type::Ptr tp) { _type = std::move(tp); }

        Type::Ptr type() const { return _type; }

        VisitableNode()

    private:
        Type::Ptr _type{};
    };

    struct Stmt : public Node {
    public:
        CSTAR_PTR(Stmt);

    public:
        using Node::Node;

    public:
        VisitableNode()
    };

    struct ExprList : public Node {
    public:
        CSTAR_PTR(ExprList);

    public:
        vec<Expr::Ptr> expressions{};

    public:
        using Node::Node;

        ExprList(vec<Expr::Ptr> expressions, Range range = {})
                : Node(std::move(range)), expressions{std::move(expressions)} {}

    public:
        void add(Expr::Ptr expression) { expressions.push_back(std::move(expression)); }

        VisitableNode()
    };

    struct VariableExpr : public Expr {
    public:
        CSTAR_PTR(VariableExpr);

    public:
        std::string_view name;

    public:
        VariableExpr(std::string_view name, Range range = {})
                : Expr(std::move(range)), name{name} {}

        VisitableNode()
    };

    struct AssignmentExpr : public Expr {
    public:
        CSTAR_PTR(AssignmentExpr);

    public:
        ExprList::Ptr assignee{};
        ExprList::Ptr value{};

    public:
        using Expr::Expr;

        AssignmentExpr(ExprList::Ptr assignee, ExprList::Ptr value, Range range = {})
                : Expr(std::move(range)), assignee{std::move(assignee)}, value{std::move(value)} {}

    public:
        VisitableNode()
    };

    struct BinaryExpr : public Expr {
    public:
        CSTAR_PTR(BinaryExpr);

    public:
        Expr::Ptr left{};
        Token::Kind op{};
        Expr::Ptr right{};

    public:
        using Expr::Expr;

        BinaryExpr(Expr::Ptr left, Token::Kind op, Expr::Ptr right, Range range = {})
                : Expr(std::move(range)), left{std::move(left)}, op{op}, right{std::move(right)} {}

    public:
        VisitableNode()
    };

    struct UnaryExpr : public Expr {
    public:
        CSTAR_PTR(UnaryExpr);

    public:
        Token::Kind op{};
        Expr::Ptr operand{};

    public:
        using Expr::Expr;

        UnaryExpr(Token::Kind op, Expr::Ptr operand, Range range = {})
                : Expr(std::move(range)), op{op}, operand{std::move(operand)} {}

    public:
        VisitableNode()
    };

    struct CallExpr : public Expr {
    public:
        CSTAR_PTR(CallExpr);

    public:
        Expr::Ptr callee{};
        ExprList::Ptr args{};

    public:
        using Expr::Expr;

        CallExpr(Expr::Ptr callee, ExprList::Ptr args, Range range = {})
                : Expr(std::move(range)), callee{std::move(callee)}, args{std::move(args)} {}

        CallExpr(Expr::Ptr callee, Range range = {})
                : CallExpr(std::move(callee), {}, std::move(range)) {}

    public:
        VisitableNode()
    };

    struct SetExpr : public Expr {
    public:
        CSTAR_PTR(SetExpr);

    public:
        Expr::Ptr object{};
        std::string_view field{};
        Expr::Ptr value{};

    public:
        using Expr::Expr;

        SetExpr(Expr::Ptr obj, std::string_view field, Expr::Ptr value, Range range = {})
                : Expr(std::move(range)), object{std::move(obj)}, field{field}, value{std::move(value)} {}

    public:
        VisitableNode()
    };

    struct GetExpr : public Expr {
    public:
        CSTAR_PTR(GetExpr);

    public:
        Expr::Ptr object{};
        std::string_view field{};

    public:
        using Expr::Expr;

        GetExpr(Expr::Ptr obj, std::string_view field, Range range = {})
                : Expr(std::move(range)), object{std::move(obj)}, field{std::move(field)} {}

    public:
        VisitableNode()
    };

    struct LiteralExpr : public Expr {
    public:
        CSTAR_PTR(LiteralExpr);

    public:
        using Expr::Expr;

    public:
        VisitableNode()
    };

    struct BoolExpr : public LiteralExpr {
    public:
        CSTAR_PTR(BoolExpr);

    public:
        bool value{};

    public:
        using LiteralExpr::LiteralExpr;

        BoolExpr(bool value, Range range = {})
                : LiteralExpr(std::move(range)), value{value}
        {
            type(builtin::booleanType());
        }

    public:
        VisitableNode()
    };

    struct CharExpr : public LiteralExpr {
    public:
        CSTAR_PTR(CharExpr);

    public:
        uint32_t value{};

    public:
        using LiteralExpr::LiteralExpr;

        CharExpr(uint32_t value, Range range = {})
                : LiteralExpr(std::move(range)), value{value}
        { type(builtin::charType()); }

    public:
        VisitableNode()
    };

    struct IntegerExpr : public LiteralExpr {
    public:
        CSTAR_PTR(IntegerExpr);

    public:
        int64_t value{};

    public:
        using LiteralExpr::LiteralExpr;

        IntegerExpr(int64_t value, Range range = {})
                : LiteralExpr(std::move(range)), value{value} {}

    public:
        VisitableNode()
    };

    struct FloatExpr : public LiteralExpr {
    public:
        CSTAR_PTR(FloatExpr);

    public:
        double value{};

    public:
        using LiteralExpr::LiteralExpr;

        FloatExpr(double value, Range range = {})
                : LiteralExpr(std::move(range)), value{value}
        { type(builtin::f64Type()); }

    public:
        VisitableNode()
    };

    struct StringExpr : public LiteralExpr {
    public:
        CSTAR_PTR(StringExpr);

    public:
        std::string value{};

    public:
        using LiteralExpr::LiteralExpr;

        StringExpr(std::string value, Range range = {})
                : LiteralExpr(std::move(range)), value{std::move(value)}
        { type(builtin::stringType()); }

    public:
        VisitableNode()
    };

    struct GroupExpr : public Expr {
    public:
        CSTAR_PTR(GroupExpr);

    public:
        Expr::Ptr expression{};

    public:
        using Expr::Expr;

        GroupExpr(Expr::Ptr expression, Range range = {})
                : Expr(std::move(range)), expression{std::move(expression)} {}

    public:
        VisitableNode()
    };

    struct ParamDecl : public Node {
    public:
        CSTAR_PTR(ParamDecl);

    public:
        std::string_view name;
        std::string_view type;

    public:
        using Node::Node;

        ParamDecl(std::string_view name, std::string_view type, Range range = {})
                : Node(std::move(range)), name{name}, type{type} {}

    public:
        VisitableNode()
    };

    struct LambdaExpr : public Expr {
    public:
        CSTAR_PTR(LambdaExpr);

    public:
        vec<std::string_view> returnType{};
        vec<ParamDecl::Ptr> parameters{};
        Stmt::Ptr body{};

    public:
        using Expr::Expr;

        LambdaExpr(vec<std::string_view> returnType, Stmt::Ptr body, vec<ParamDecl::Ptr> parameters, Range range = {})
                : Expr(std::move(range)),
                  returnType{std::move(returnType)},
                  body{std::move(body)},
                  parameters{std::move(parameters)} {}

        LambdaExpr(vec<std::string_view> returnType, Stmt::Ptr body, Range range = {})
                : LambdaExpr(std::move(returnType), std::move(body), {}, std::move(range)) {}

    public:
        VisitableNode()
    };


    // (e1, e2, e3)
    struct TupleExpr : public Expr {
    public:
        CSTAR_PTR(TupleExpr);

    public:
        ExprList::Ptr expressions{};

    public:
        using Expr::Expr;

        TupleExpr(ExprList::Ptr expressions, Range range = {})
            : Expr(std::move(range)), expressions{std::move(expressions)} {}

    public:
        VisitableNode()
    };

    struct NewExpr : public Expr {
    public:
        CSTAR_PTR(NewExpr);

    public:
        std::string_view target{};
        ExprList::Ptr args{};

    public:
        using Expr::Expr;

        NewExpr(std::string_view target, ExprList::Ptr args, Range range = {})
                : Expr(std::move(range)), target{std::move(target)}, args{std::move(args)} {}

        NewExpr(std::string_view target, Range range = {})
                : NewExpr(target, {}, std::move(range)) {}

    public:
        VisitableNode()
    };

    // {e1, e2, e3}
    struct BraceExpr : public Expr {
    public:
        CSTAR_PTR(BraceExpr);

    public:
        ExprList::Ptr expressions{};

    public:
        using Expr::Expr;

        BraceExpr(ExprList::Ptr expressions, Range range = {})
                : Expr(std::move(range)), expressions{std::move(expressions)} {}

    public:
        VisitableNode()
    };

    struct ThisExpr : public Expr {
    public:
        CSTAR_PTR(ThisExpr);

    public:
        using Expr::Expr;

    public:
        VisitableNode()
    };

    struct SuperExpr : public Expr {
    public:
        CSTAR_PTR(SuperExpr);

    public:
        using Expr::Expr;

    public:
        VisitableNode()
    };

    struct BlockStmt : public Stmt {
    public:
        CSTAR_PTR(BlockStmt);

    public:
        vec<Stmt::Ptr> statements{};

    public:
        using Stmt::Stmt;

        BlockStmt(vec<Stmt::Ptr> statements, Range range = {})
                : Stmt(std::move(range)), statements{std::move(statements)} {}

    public:
        VisitableNode()

        void add(Stmt::Ptr stmt) { statements.push_back(std::move(stmt)); }
    };

    struct IfStmt : public Stmt {
    public:
        CSTAR_PTR(IfStmt);

    public:
        Expr::Ptr condition{};
        Stmt::Ptr then{};
        Stmt::Ptr els{};

    public:
        using Stmt::Stmt;

        IfStmt(Expr::Ptr condition, Stmt::Ptr then, Stmt::Ptr els, Range range = {})
                : Stmt(std::move(range)),
                  condition{std::move(condition)}, then{std::move(then)}, els{std::move(els)} {}

        IfStmt(Expr::Ptr condition, Stmt::Ptr then, Range range = {})
                : IfStmt(std::move(condition), std::move(then), {}, std::move(range)) {}
    };

    struct BreakStmt : public Stmt {
    public:
        CSTAR_PTR(BreakStmt);

    public:
        using Stmt::Stmt;

    public:
        VisitableNode()
    };

    struct ContinueStmt : public Stmt {
    public:
        CSTAR_PTR(ContinueStmt);

    public:
        using Stmt::Stmt;

    public:
        VisitableNode()
    };

    struct ForStmt : public Stmt {
    public:
        CSTAR_PTR(ForStmt);

    public:
        vec<VariableExpr::Ptr> variables{};
        ExprList::Ptr values{};
        Stmt::Ptr body{};

    public:
        using Stmt::Stmt;
        ForStmt(vec<VariableExpr::Ptr> variables, ExprList::Ptr values, Range range = {})
            : Stmt(std::move(range)),
              variables{std::move(variables)}, values{std::move(values)}
        {}

    public:
        VisitableNode()
    };

    struct VarStmt : public Stmt {
    public:
        CSTAR_PTR(VarStmt);

    public:
        VariableExpr::Ptr variable{};
        VariableExpr::Ptr type{};
        ExprList::Ptr initializer{};

    public:
        using Stmt::Stmt;
        VarStmt(VariableExpr::Ptr variable, VariableExpr::Ptr type, ExprList::Ptr initializer, Range range = {})
            : Stmt(std::move(range)),
              variable{std::move(variable)},
              type{std::move(type)},
              initializer{std::move(initializer)}
        {}

        VarStmt(VariableExpr::Ptr variable,VariableExpr::Ptr type, Range range = {})
            : VarStmt(std::move(variable), std::move(type), {}, std::move(range))
        {}

    public:
        VisitableNode()
    };

    class FunctionStmt : public Stmt {
    public:
        CSTAR_PTR(FunctionStmt);

    public:
        std::string_view name{};
        ExprList::Ptr returnTypes{};
        vec<ParamDecl::Ptr> parameters{};
        Stmt::Ptr body{};

    public:
        using Stmt::Stmt;
        FunctionStmt(std::string_view name, ExprList::Ptr returnTypes, Stmt::Ptr body, vec<ParamDecl::Ptr> parameters, Range range = {})
            : Stmt(std::move(range)),
              name{name},
              returnTypes{std::move(returnTypes)},
              body{std::move(body)},
              parameters{std::move(parameters)}
        {}

        FunctionStmt(std::string_view name, ExprList::Ptr returnTypes, Stmt::Ptr body, Range range = {})
            : FunctionStmt(name, std::move(returnTypes), std::move(body), {}, std::move(range))
        {}

    };

    struct Field : public Node {
    public:
        CSTAR_PTR(Field);

    public:
        std::string_view name{};
        Expr::Ptr type{};

    public:
        using Node::Node;
        Field(std::string_view name, Expr::Ptr type, Range range = {})
            : Node(std::move(range)), name{name}, type{std::move(type)}
        {}

    public:
        VisitableNode()
    };

    struct ClassStmt : public Stmt {
    public:
        CSTAR_PTR(ClassStmt);

    public:
        std::string_view name{};
        Expr::Ptr super{};
        vec<Field::Ptr> fields{};
        vec<FunctionStmt> methods{};

    public:
        using Stmt::Stmt;
        ClassStmt(std::string_view name, vec<Field::Ptr> fields, vec<FunctionStmt> methods, Expr::Ptr super, Range range = {})
            : Stmt(std::move(range)),
              fields{std::move(fields)},
              methods{std::move(methods)},
              super{std::move(super)}
        {}

        ClassStmt(std::string_view name, vec<Field::Ptr> fields, vec<FunctionStmt> methods, Range range = {})
            : ClassStmt(name, std::move(fields), std::move(methods), {}, std::move(range))
        {}

    public:
        VisitableNode()
    };

    struct ReturnStmt : public Stmt {
    public:
        CSTAR_PTR(ReturnStmt);

    public:
        ExprList::Ptr expressions;

    public:
        using Stmt::Stmt;
        ReturnStmt(ExprList::Ptr expressions, Range range = {})
            : Stmt(std::move(range)), expressions{std::move(expressions)}
        {}

        ReturnStmt(Range range = {}) : ReturnStmt({}, std::move(range)) {}

    public:
        VisitableNode()
    };

    struct EmptyStmt : public Stmt {
    public:
        CSTAR_PTR(EmptyStmt);

    public:
        using Stmt::Stmt;

    public:
        VisitableNode()
    };

    struct ExpressionStmt : public Stmt {
    public:
        CSTAR_PTR(ExpressionStmt);

    public:
        Expr::Ptr expression{};

    public:
        using Stmt::Stmt;
        ExpressionStmt(Expr::Ptr expr, Range range = {})
            : Stmt(std::move(range)), expression{std::move(expr)}
        {}

    public:
        VisitableNode()
    };

    class Program  {
    public:
        CSTAR_PTR(Program);

    public:
        vec<Node::Ptr> nodes{};

    public:
        Program(vec<Node::Ptr> nodes  = {}) : nodes{std::move(nodes)}
        {}

    public:
        void add(Node::Ptr node) { nodes.push_back(std::move(node)); }
    };
}