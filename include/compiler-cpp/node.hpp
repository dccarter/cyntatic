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

#include <compiler/token.hpp>
#include <compiler/visitor.hpp>

namespace cyn {

    class AstNode {
    public:
        CYN_PTR(AstNode);
        GenericFlags flags;

        AstNode(Range&& range = {})
            : _range{std::move(range)}
        {}

        const Range& range() { return _range; }

        virtual void accept(Visitor& visitor) { visitor.visit(*this); }
    protected:
        Range _range;
    };

    class Expression : public AstNode {
    public:
        CYN_PTR(Expression);

        Expression(Range range = {})
            : AstNode(std::move(range))
        {};

    public:
        CynMakeVisitable()
    };

    class Statement : public AstNode {
    public:
        CYN_PTR(Statement);

        Statement(Range range = {})
            : AstNode(std::move(range))
        {}

    public:
        CynMakeVisitable()
    };

    class Declaration : public AstNode {
    public:
        CYN_PTR(Declaration);

        Declaration(Range range = {})
        : AstNode(std::move(range))
        {}

    public:
        CynMakeVisitable()
    };

    class NullExpr : public Expression {
    public:
        CYN_PTR(NullExpr);

        NullExpr(Range range = {})
            : Expression(std::move(range))
        {}

    public:
        CynMakeVisitable()
    };

    class BoolExpr : public Expression {
    public:
        CYN_PTR(BoolExpr);
        bool Value{false};

        BoolExpr(bool value, Range range)
            : Expression(std::move(range)),
              Value{value}
        {}

    public:
        CynMakeVisitable()
    };

    class CharExpr : public Expression {
    public:
        CYN_PTR(CharExpr);
        std::uint32_t Value{'\0'};

        CharExpr(std::uint32_t value, Range range = {})
            : Expression(std::move(range)),
              Value{value}
        {}

    public:
        CynMakeVisitable()
    };

    class IntegerExpr : public Expression {
    public:
        typedef enum : std::uint8_t {
            Int8 =  1,
            Int16 = 2,
            Int32 = 4,
            Int64 = 8
        } IntSize;

        CYN_PTR(IntegerExpr);

        std::uint64_t Value{0};
        IntSize       ByteSize{Int32};

        IntegerExpr(std::uint64_t value, Range range = {})
            : Expression(std::move(range)),
              Value{value}
        {}

        IntegerExpr(std::uint64_t value, IntSize byteSize, Range range = {})
            : Expression(std::move(range)),
              Value{value},
              ByteSize{byteSize}
        {}

    public:
        CynMakeVisitable()
    };

    class FloatExpr : public Expression {
    public:
        typedef enum : std::uint8_t {
            Flt32 = 4,
            Flt64 = 8
        } FloatSize;

        CYN_PTR(FloatExpr);

        double    Value{0};
        FloatSize ByteSize{Flt32};

        FloatExpr(double value, Range range = {})
            : Expression(std::move(range)),
              Value{value}
        {}

        FloatExpr(double value, FloatSize byteSize, Range range = {})
            : Expression(std::move(range)),
              Value{value},
              ByteSize{byteSize}
        {}

    public:
        CynMakeVisitable()
    };

    class StringExpr : public Expression {
    public:
        CYN_PTR(StringExpr);

        std::string_view Value{};

        StringExpr(std::string_view value, Range range = {})
            : Expression(std::move(range)),
              Value{value}
        {}

    public:
        CynMakeVisitable()
    };

    class VariableExpr : public Expression {
    public:
        CYN_PTR(VariableExpr);

        Identifier  Name{};

        VariableExpr(Identifier name, Range range = {})
            : Expression(std::move(range)),
              Name{name}
        {}

    public:
        CynMakeVisitable()
    };

    class StrExprExpr : public Expression {
    public:
        CYN_PTR(VariableExpr);

        vec<Expression::Ptr> Expressions{};

        StrExprExpr(vec<Expression::Ptr> exprs, Range range = {})
                : Expression(std::move(range)),
                  Expressions{std::move(exprs)}
        {}

    public:
        CynMakeVisitable()
    };

    class BinaryExpr : public Expression {
    public:
        CYN_PTR(BinaryExpr);

        Expression::Ptr Left{};
        Token::Kind Op{};
        Expression::Ptr Right{};

        BinaryExpr(Expression::Ptr left, Token::Kind op, Expression::Ptr right, Range range = {})
            : Expression(std::move(range)),
              Left{std::move(left)}, Op{op}, Right{std::move(right)}
        {}

    public:
        CynMakeVisitable()
    };

    class UnaryExpr : public Expression {
    public:
        CYN_PTR(UnaryExpr);

        Expression::Ptr Operand{};
        Token::Kind Op{};

        UnaryExpr(Expression::Ptr operand, Token::Kind op, Range range = {})
            : Expression(std::move(range)),
              Operand{std::move(operand)}, Op{op}
        {}

    public:
        CynMakeVisitable()
    };

    class ReferExpr : public Expression {
    public:
        CYN_PTR(ReferExpr);

        Identifier Name{};

        ReferExpr(Identifier name, Range range = {})
            : Expression(std::move(range)),
              Name{name}
        {}

    public:
        CynMakeVisitable()
    };

    class DerefExpr : public Expression {
    public:
        CYN_PTR(DerefExpr);

        Expression::Ptr Target{};

        DerefExpr(Expression::Ptr target, Range range = {})
                : Expression(std::move(range)),
                  Target{std::move(target)}
        {}

    public:
        CynMakeVisitable()
    };

    class NewExpr : public Expression {
    public:
        CYN_PTR(NewExpr);

        Expression::Ptr Expr{};

        NewExpr(Expression::Ptr expr, Range range = {})
                : Expression(std::move(range)),
                  Expr{std::move(expr)}
        {}

    public:
        CynMakeVisitable()
    };

    class FreeExpr : public Expression {
    public:
        CYN_PTR(NewExpr);

        Expression::Ptr Expr{};

        FreeExpr(Expression::Ptr expr, Range range = {})
            : Expression(std::move(range)),
              Expr{std::move(expr)}
        {}

    public:
        CynMakeVisitable()
    };

    class CastExpr : public Expression {
    public:
        CYN_PTR(CastExpr);

        Expression::Ptr Expr{};
        ptr<Type>       Dest{};

        CastExpr(Expression::Ptr expr, ptr<Type> dst, Range range = {})
            : Expression(std::move(range)),
              Expr{std::move(expr)}, Dest{std::move(dst)}
        {}

    public:
        CynMakeVisitable()
    };

    class CallExpr : public Expression {
    public:
        CYN_PTR(CallExpr);

        Identifier Name{};
        vec<Expression::Ptr> Arguments{};

        CallExpr(Identifier name, vec<Expression::Ptr> args, Range range = {})
            : Expression(std::move(range)),
              Name{name}, Arguments{std::move(args)}
        {}

    public:
        CynMakeVisitable()
    };

    class MethodCallExpr : public  Expression {
    public:
        CYN_PTR(MethodCallExpr);

        Expression::Ptr This{};
        Identifier Name{};
        vec<Expression::Ptr> Arguments{};

        MethodCallExpr(Expression::Ptr self, Identifier name, vec<Expression::Ptr> args, Range range = {})
            : Expression(std::move(range)),
                This{std::move(self)}, Name{name}, Arguments{std::move(args)}
        {}

    public:
        CynMakeVisitable()
    };

    class IndexExpr : public  Expression {
    public:
        CYN_PTR(IndexExpr);

        Expression::Ptr This{};
        vec<Expression::Ptr> Indices{};

        IndexExpr(Expression::Ptr self, vec<Expression::Ptr> indices, Range range = {})
                : Expression(std::move(range)),
                  This{std::move(self)}, Indices{std::move(indices)}
        {}

    public:
        CynMakeVisitable()
    };

    class ConditionalExpr : public  Expression {
    public:
        CYN_PTR(ConditionalExpr);

        Expression::Ptr Cond{};
        Expression::Ptr True{};
        Expression::Ptr False{};

        ConditionalExpr(Expression::Ptr cond, Expression::Ptr iTrue, Expression::Ptr iFalse, Range range = {})
                : Expression(std::move(range)),
                  Cond{std::move(cond)}, True{std::move(iTrue)}, False{std::move(iFalse)}
        {}

    public:
        CynMakeVisitable()
    };

    class DefineStmt : public Statement {
    public:
        CYN_PTR(DefineStmt);

        VariableExpr::Ptr Var{};
        ptr<Type>  DefType{};
        Expression::Ptr Expr{};

        DefineStmt(VariableExpr::Ptr var, ptr<Type> defType, Expression::Ptr expr, Range range = {})
            : Statement(std::move(range)),
              Var{std::move(var)}, DefType{std::move(defType)}, Expr{std::move(expr)}
        {}

    public:
        CynMakeVisitable()
    };

    class BlockStmt : public Statement {
    public:
        CYN_PTR(BlockStmt);

        vec<Statement::Ptr> Statements{};

        BlockStmt(vec<Statement::Ptr> statements, Range range = {})
                : Statement(std::move(range)),
                  Statements{std::move(statements)}
        {}

    public:
        CynMakeVisitable()
    };

    class AssignStmt : public Statement {
    public:
        CYN_PTR(DefineStmt);

        Expression::Ptr Lhs{};
        Expression::Ptr Rhs{};

        AssignStmt(Expression::Ptr lhs, Expression::Ptr rhs, Range range = {})
                : Statement(std::move(range)),
                  Lhs{std::move(lhs)}, Rhs{std::move(rhs)}
        {}

    public:
        CynMakeVisitable()
    };

    class OpAssignmentStmt : public Statement {
    public:
        CYN_PTR(OpAssignmentStmt);

        Expression::Ptr Lhs{};
        Token::Kind     Op{};
        Expression::Ptr Rhs{};

        OpAssignmentStmt(Expression::Ptr lhs, Token::Kind op, Expression::Ptr rhs, Range range = {})
                : Statement(std::move(range)),
                  Lhs{std::move(lhs)}, Op{op}, Rhs{std::move(rhs)}
        {}

    public:
        CynMakeVisitable()
    };

    class ForStmt : public Statement {
    public:
        CYN_PTR(ForStmt);

        Statement::Ptr Init{};
        Expression::Ptr Cond{};
        Expression::Ptr Next{};
        BlockStmt::Ptr  Body;

        ForStmt(Statement::Ptr init, Expression::Ptr cond, Expression::Ptr next, BlockStmt::Ptr body, Range range = {})
                : Statement(std::move(range)),
                  Init{std::move(init)}, Cond{std::move(cond)}, Next{std::move(next)}, Body{std::move(body)}
        {}

        ForStmt(Statement::Ptr init, Expression::Ptr cond, Expression::Ptr next, Range range = {})
            : ForStmt(std::move(init), std::move(cond), std::move(next), {}, std::move(range))
        {}

    public:
        CynMakeVisitable()
    };

    class ForRangeStmt : public Statement {
    public:
        CYN_PTR(ForRangeStmt);

        vec<VariableExpr::Ptr> Variables{};
        Expression::Ptr      Iterator{};
        BlockStmt::Ptr       Body;

        ForRangeStmt(vec<VariableExpr::Ptr> vars, Expression::Ptr iterator, BlockStmt::Ptr body, Range range = {})
            : Statement(std::move(range)),
              Variables{std::move(vars)}, Iterator{std::move(iterator)}, Body{std::move(body)}
        {}

        ForRangeStmt(vec<VariableExpr::Ptr> vars, Expression::Ptr iterator, Range range = {})
            : ForRangeStmt(std::move(vars), std::move(iterator), {}, std::move(range))
        {}

    public:
        CynMakeVisitable()
    };

    class WhileStmt : public Statement {
    public:
        CYN_PTR(WhileStmt);

        Expression::Ptr Cond{};
        BlockStmt::Ptr  Body{};

        WhileStmt(Expression::Ptr cond, BlockStmt::Ptr body, Range range = {})
            : Statement(std::move(range)),
              Cond{std::move(cond)}, Body{std::move(body)}
        {}

        WhileStmt(Expression::Ptr cond, Range range = {})
            : WhileStmt(std::move(cond), {}, std::move(range))
        {}

    public:
        CynMakeVisitable()
    };

    class IfStmt : public Statement {
    public:
        CYN_PTR(IfStmt);

        Expression::Ptr Cond{};
        BlockStmt::Ptr  Then{};
        Statement::Ptr  Else{};


        IfStmt(Expression::Ptr cond, BlockStmt::Ptr then, Statement::Ptr els, Range range = {})
                : Statement(std::move(range)),
                  Cond{std::move(cond)}, Then{std::move(then)}, Else{std::move(els)}
        {}

        IfStmt(Expression::Ptr cond, BlockStmt::Ptr then, Range range = {})
            : IfStmt(std::move(cond), std::move(then), nullptr, std::move(range))
        {}

    public:
        CynMakeVisitable()
    };

    class ReturnStmt : public Statement {
    public:
        CYN_PTR(ReturnStmt);

        vec<Expression::Ptr> Values{};


        ReturnStmt(vec<Expression::Ptr> values, Range range = {})
                : Statement(std::move(range)), Values{std::move(values)}
        {}

    public:
        CynMakeVisitable()
    };

    class ExpressionStmt: public Statement {
    public:
        CYN_PTR(ExpressionStmt);

        Expression::Ptr Expr{};


        ExpressionStmt(Expression::Ptr expr, Range range = {})
            : Statement(std::move(range)),
              Expr{std::move(expr)}
        {}

    public:
        CynMakeVisitable()
    };



}