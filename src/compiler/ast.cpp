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

namespace cstar {

    VarStmt::VarStmt(VariableExpr::Ptr variable, Type::Ptr tp, Range range)
        : Stmt(std::move(range))
    {
        variable->type(std::move(tp));
        variables.push_back(std::move(variable));
    }

}