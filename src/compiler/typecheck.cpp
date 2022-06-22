/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-06-15
 */


#include "compiler/ast.hpp"
#include "compiler/typecheck.hpp"

namespace cstar {

    void TypeChecker::visit(BoolExpr &node)
    {
        _return = node.type();
    }

    void TypeChecker::visit(CharExpr &node)
    {
        _return = node.type();
    }

    void TypeChecker::visit(IntegerExpr &node)
    {
        _return = node.type();
    }

    void TypeChecker::visit(FloatExpr &node)
    {
        _return = node.type();
    }

    void TypeChecker::visit(StringExpr &node)
    {
        _return = node.type();
    }

    void TypeChecker::visit(VariableExpr &node)
    {
        if (auto obj = _scope->lookup(node.name)) {
            _return = std::dynamic_pointer_cast<Type>(obj);
        }
        else {
            abort(node.range(), "variable '", node.name, " is not defined");
        }
    }

    void TypeChecker::visit(AssignmentExpr &node)
    {
        if (node.assignee == nullptr || node.value == nullptr) {
            abort(node.range(), "missing variable to assign to");
        }

        auto& assignees = node.assignee->expressions;
        auto& values = node.value->expressions;

        if (assignees.size() != values.size()) {
            abort(node.range(), "number of right hand side expression must match left hand side");
        }

        std::vector<Type::Ptr> retTypes;
        for (int i = 0; i < assignees.size(); i++) {
            auto& assignee = assignees[i];
            auto& value = values[i];
            if (i) retTypes.push_back(std::move(_return));

            auto valueType = evaluate(value);
            _return = evaluate(assignee);
            if (!valueType->isAssignable(_return)) {
                abort(value->range().merge(assignee->range()),
                    "value of type '", valueType->name(),
                    "' cannot be assigned to variable of type '", _return->name());
            }

        }

        if (retTypes.size() > 1) {
            _return  = mk<TupleType>(std::move(retTypes));
        }

        node.type(_return);
    }

    void TypeChecker::visit(BinaryExpr &node)
    {
        auto left = evaluate(node.left);
        auto right = evaluate(node.right);
        if (Token::isLogicalOperator(node.op)) {
            _return = builtin::booleanType();
        }
        else {
            _return = evalBinaryExpr(left, node.op, right);
            if (_return == nullptr) {
                abort(node.range(),
                        "binary operator not allowed between type '",
                        left->name(), "' and type '", right->name(), "'");
            }
        }

        node.type(_return);
    }

    void TypeChecker::visit(UnaryExpr &node)
    {
        auto operandType = evaluate(node.operand);
        switch (node.op) {
            case Token::NOT:
                _return = builtin::booleanType();
                break;
            case Token::PLUS:
            case Token::PLUSPLUS:
            case Token::MINUS:
            case Token::MINUSMINUS: {
                auto bOperand = std::dynamic_pointer_cast<BuiltinType>(operandType);
                if (!bOperand or (bOperand->id != BuiltinType::tidInteger and bOperand->id != BuiltinType::tidFloat)) {
                    abort(node.range(), "'", Token::toString(node.op), "' can only be applied on numbers");
                }
                _return = operandType;
                break;
            }
            default:
                L.error(node.range(), "'", Token::toString(node.op), "' is not a unary operator");
                abortCompiler(L);
        }
        node.type(_return);
    }

    void TypeChecker::visit(CallExpr &node)
    {
        auto type = evaluate(node.callee);
        if (auto func = std::dynamic_pointer_cast<FunctionType>(type)) {
            auto& params = func->parameters;
            auto& args = node.args->expressions;
            if (params.size() != args.size()) {
                abort(node.range(), "expecting '", params.size(), "' arguments, given '", args.size(), "'");
            }

            for (int i = 0; i < args.size(); i++) {
                auto argType = evaluate(args[i]);
                if (!params[i]->isAssignable(argType)) {
                    abort(args[i]->range(), "parameter type mismatch, expecting '", params[i]->name(), "', given '",
                          argType->name(), "'");
                }
            }
            _return = mk<TupleType>(func->parameters);
            node.type(_return);
        }
        else {
            abort(node.range(), "calling an invalid callee, i.e callee is not callable");
        }
    }

    void TypeChecker::visit(GetExpr &node)
    {
        auto type = evaluate(node.object);
        if (auto cls = std::dynamic_pointer_cast<ClassType>(type)) {
            _return = cls->get(node.field);
            node.type(_return);
        }
        else {
            abort(node.range(), "field access not supported in non-class types");
        }
    }

    void TypeChecker::visit(SetExpr &node)
    {
        auto type = evaluate(node.object);
        if (auto cls = std::dynamic_pointer_cast<ClassType>(type)) {
            _return = cls->get(node.field);
            auto valueType = evaluate(node.value);

            if (!_return or !_return->isAssignable(valueType)) {
                abort(node.range(), "setting field '", node.field,
                      "' type mismatch, expecting '", _return->name(), "'");
            }

            node.type(_return);
        }
        else {
            abort(node.range(), "field access not supported in non-class types");
        }
    }

    void TypeChecker::visit(GroupExpr &node)
    {
        _return = evaluate(node.expression);
        node.type(_return);
    }

    void TypeChecker::visit(NewExpr &node)
    {
        auto obj = _scope->lookup(node.target);
        if (auto type = std::dynamic_pointer_cast<Type>(obj)) {
            _return = type;
            node.type(_return);
        }
        else {
            abort(node.range(), "type '", node.target, "' not found");
        }
    }

    void TypeChecker::visit(ThisExpr &node)
    {
        auto obj = _scope->lookup("this");
        if (auto type = std::dynamic_pointer_cast<Type>(obj)) {
            _return = type;
            node.type(_return);
        }
        else {
            abort(node.range(), "'this' undefined in current scope");
        }
    }

    void TypeChecker::visit(SuperExpr &node)
    {
        auto obj = _scope->lookup("super");
        if (auto type = std::dynamic_pointer_cast<Type>(obj)) {
            _return = type;
            node.type(_return);
        }
        else {
            abort(node.range(), "'super' undefined in current scope");
        }
    }

    void TypeChecker::visit(LambdaExpr &node)
    {
        vec<Type::Ptr> returnTypes{};
        for (auto& name: node.returnType) {
            auto obj = _scope->lookup(name);
            if (auto type = std::dynamic_pointer_cast<Type>(obj)) {
                returnTypes.push_back(std::move(type));
            }
            else {
                abort(node.range(), "return type '", name, "' undefined in current scope");
            }
        }

        vec<Type::Ptr> paramTypes{};
        for (auto& param: node.parameters) {
            auto obj = _scope->lookup(param->type);
            if (auto type = std::dynamic_pointer_cast<Type>(obj)) {
                if (!_scope->define(param->name, type)) {
                    abort(param->range(), "parameter with name '", param, "' already defined in current scope");
                }
                paramTypes.push_back(std::move(type));
            }
            else {
                abort(node.range(), "parameter type '", param->name, "' undefined in current scope");
            }
        }

        _return = mk<FunctionType>(std::move(returnTypes), std::move(paramTypes));
        _scope->define("this", _return);
        execute(node.body);
        popScope();
        node.type(_return);
    }

    void TypeChecker::visit(BraceExpr &node)
    {
        auto& expressions = node.expressions->expressions;
        if (expressions.empty()) {
            _return = builtin::nullType();
            node.type(_return);
            return;
        }

        auto base = evaluate(expressions[0]);
        for (int i = 1; i < expressions.size(); i++) {
            auto other = evaluate(expressions[i]);
            auto upper = Type::leastUpperBound(other, base);
            if (upper) {
                base = upper;
            }
            else {
                abort(expressions[i]->range(), "expression type '", other->name(),
                      "' does not match expected elements type '", base->name(), "'");
            }
        }
        _return = mk<ArrayType>(base, expressions.size());
        node.type(_return);
    }

    void TypeChecker::visit(TupleExpr &node)
    {
        auto& expressions = node.expressions->expressions;
        if (expressions.empty()) {
            _return = builtin::nullType();
            node.type(_return);
            return;
        }

        auto tupType = mk<TupleType>();
        for (auto& expression : expressions) {
            tupType->members.push_back(evaluate(expression));
        }

        _return = tupType;
        node.type(_return);
    }

    void TypeChecker::visit(IfStmt &node)
    {
        evaluate(node.condition);
        execute(node.then);
        execute(node.els);
    }

    void TypeChecker::visit(ForStmt &node)
    {
        auto iteType = evaluate(node.values);
        pushScope();
        if (node.variables.size() > 1) {
            auto tupType = std::dynamic_pointer_cast<TupleType>(iteType);
            auto& members = tupType->members;
            if (node.variables.size() > members.size()) {
                abort(node.range(), "number of variables mismatch iterator return");
            }

            for (int i = 0; i < node.variables.size(); i++) {
                auto& var = node.variables[i];
                if (var->name == "_") {
                    continue;
                }
                if (!_scope->define(var->name, members[i])) {
                    abort(var->range(), "loop variable with name '", var->name, " already defined in current loop");
                }
            }
        }
        else {
            auto& var = node.variables[0];
            abort(var->range(), "loop variable with name '", var->name, " already defined in current loop");
        }

        execute(node.body);
        popScope();
    }

    void TypeChecker::visit(BlockStmt &node)
    {
        pushScope();
        for (auto& stmt: node.statements) {
            execute(stmt);
        }
        popScope();
    }

    void TypeChecker::visit(VarStmt &node)
    {
        auto initType = evaluate(node.initializer);
        if (node.type) {
            auto varType = _scope->lookup<Type>(node.type->name);
            if (!varType) {
                abort(node.type->range(), "variable type '", node.type->name, "' not found");
            }
            if (!varType->isAssignable(initType)) {
                abort(node.range(), "initializer type '", initType->name(),
                      "' incompatible with variable type '", varType->name(), "'");
            }
            _scope->define(node.variable->name, varType);
        }
        else if (initType) {
            _scope->define(node.variable->name, initType);
        }
        else {
            abort(node.range(), "cannot declare a variable without a type");
        }
    }

    Type::Ptr TypeChecker::evaluate(ptr<Node> expr)
    {
        if (expr) expr->accept(*this);
        return std::exchange(_return, nullptr);
    }

    void TypeChecker::execute(ptr<Node> stmt)
    {
        if (stmt) stmt->accept(*this);
    }

    Type::Ptr TypeChecker::evalBinaryExpr(Type::Ptr &left, Token::Kind op, Type::Ptr &right)
    {
        if (left == builtin::stringType() and right == builtin::stringType() and op == Token::PLUS) {
            return  builtin::stringType();
        }

        if (left->kind == Type::tpBuiltin and right->kind == Type::tpBuiltin) {
            auto bLeft = std::dynamic_pointer_cast<BuiltinType>(left);
            auto bRight = std::dynamic_pointer_cast<BuiltinType>(right);
            if (bRight->id == BuiltinType::tidInteger and bLeft->id == BuiltinType::tidInteger) {
                return IntegerType::biggerType(
                        std::dynamic_pointer_cast<IntegerType>(bLeft),
                        std::dynamic_pointer_cast<IntegerType>(bRight));
            }
        }
        return nullptr;
    }

    void TypeChecker::pushScope()
    {
        _scope = mk<SymbolTable>(_scope);
    }

    void TypeChecker::popScope()
    {
        if (_scope and _scope->enclosing())
            _scope = _scope->enclosing();
    }

}