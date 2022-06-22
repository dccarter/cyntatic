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

#include <compiler/log.hpp>
#include <compiler/symbol.hpp>
#include <compiler/types.hpp>
#include <compiler/vistor.hpp>


namespace cstar {

    class TypeChecker : public Visitor {
    public:
        TypeChecker(Log& L, SymbolTable::Ptr scope)
            : L{L}, _scope{std::move(scope)}
        {}

        void run(Program& program);

        void visit(BoolExpr &node) override;
        void visit(CharExpr &node) override;
        void visit(IntegerExpr &node) override;
        void visit(FloatExpr &node) override;
        void visit(StringExpr &node) override;
        void visit(VariableExpr &node) override;
        void visit(AssignmentExpr &node) override;
        void visit(BinaryExpr &node) override;
        void visit(UnaryExpr &node) override;
        void visit(CallExpr &node) override;
        void visit(GetExpr &node) override;
        void visit(SetExpr &node) override;
        void visit(GroupExpr &node) override;
        void visit(LambdaExpr &node) override;
        void visit(NewExpr &node) override;
        void visit(ThisExpr &node) override;
        void visit(SuperExpr &node) override;
        void visit(BraceExpr &node) override;
        void visit(TupleExpr &node) override;
        void visit(IfStmt &node) override;
        void visit(BlockStmt &node) override;
        void visit(ForStmt &node) override;
        void visit(VarStmt &node) override;

    private:
        Type::Ptr evaluate(ptr<Node> expr);
        void execute(ptr<Node> stmt);
        Type::Ptr evalBinaryExpr(Type::Ptr& left, Token::Kind op, Type::Ptr& right);

        void pushScope();
        void popScope();

        template <typename ...Args>
        [[noreturn]] void abort(Range range, Args... args)
        {
            L.error(std::move(range), std::forward<Args>(args)...);
            abortCompiler(L);
        }

        Log& L;
        SymbolTable::Ptr _scope{};
        Type::Ptr _return{};
    };
}
