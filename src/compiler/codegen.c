/**
 * Copyright (c) 2022 suilteam, Carter
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author Mpho Mbotho
 * @date 2022-12-19
 */

#include "compiler/ast.h"
#include "vm/builder.h"
#include "vm/instr.h"

#include <math.h>

void Codegen_generateCyn(Builder *cb, AstNode *node)
{
    switch (node->id) {
    case astStringLit:
        Builder_appendInstruction(cb,
            cMOV(rRa(r5),
                 xIMb(u32,
                      Builder_appendString(
                            cb,
                            node->astString.value))));
        break;
    case astBoolLit:
        Builder_appendInstruction(
            cb,
            cMOV(rRa(r5), xIMb(u8, node->astBool.value)));
        break;

    case astCharLit:
        Builder_appendInstruction(
            cb,
            cMOV(rRa(r5), xIMb(u32, node->astChar.value)));
        break;

    case astIntegerLit:
        Builder_appendInstruction(
            cb,
            cMOV(rRa(r5), xIMb(u64, node->astInteger.value)));
        break;

    case astFloatLit:
        Builder_appendInstruction(
            cb,
            cMOV(rRa(r5), xIMb(f64, node->astFloat.value)));
        break;
    case astVarExpr:
        Builder_appendInstruction(cb,
                       cMOV(rRa(r4), rbEA(bp, u8, node->astVar.offset)));
        Builder_appendInstruction(cb, cMOV(rRa(r5), mRb(r4)));
        break;
    case astUnaryExpr:
        Codegen_generateCyn(cb, node->astUnary.expr);
        if (node->astUnary.op == tokNot) {
            Builder_appendInstruction(cb, cNOT(rRa(r5)));
        }
        else if (node->astUnary.op == tokComplement) {
            Builder_appendInstruction(cb, cBNOT(rRa(r5)));
        }
        else {
            unreachable();
        }

        break;

    case astPrefixExpr:
        Codegen_generateCyn(cb, node->astPrefix.expr);
        if (node->astUnary.op == tokPlusPlus) {
            Builder_appendInstruction(cb, cINC(mRa(r4)));
        }
        else if (node->astUnary.op == tokMinusMinus) {
            Builder_appendInstruction(cb, cDEC(mRa(r4)));
        }
        else {
            unreachable();
        }
        Builder_appendInstruction(cb, cMOV(rRa(r5), mRb(r4)));
        break;

    case astPostfixExpr:
        Codegen_generateCyn(cb, node->astPostfix.expr);
        if (node->astUnary.op == tokPlusPlus) {
            Builder_appendInstruction(cb, cINC(mRa(r4)));
        }
        else if (node->astUnary.op == tokMinusMinus) {
            Builder_appendInstruction(cb, cDEC(mRa(r4)));
        }
        else {
            unreachable();
        }
        break;
    case astBinaryExpr:
        Codegen_generateCyn(cb, node->astBinary.lhs);
        Builder_appendInstruction(cb, cPUSH(rRa(r5)));
        Codegen_generateCyn(cb, node->astBinary.rhs);
        Builder_appendInstruction(cb, cPOP(rRa(r4)));
        switch (node->astBinary.op) {
            case tokPlus:
                Builder_appendInstruction(cb, cADD(rRa(r5), rRb(r4)));
                break;
            case tokMinus:
                Builder_appendInstruction(cb, cSUB(rRa(r5), rRb(r4)));
                break;
            case tokMult:
                Builder_appendInstruction(cb, cMUL(rRa(r5), rRb(r4)));
                break;
            case tokDiv:
                Builder_appendInstruction(cb, cDIV(rRa(r5), rRb(r4)));
                break;
            case tokMod:
                Builder_appendInstruction(cb, cMOD(rRa(r5), rRb(r4)));
                break;
            case tokBitAnd:
                Builder_appendInstruction(cb, cBAND(rRa(r5), rRb(r4)));
                break;
            case tokBitOr:
                Builder_appendInstruction(cb, cBOR(rRa(r5), rRb(r4)));
                break;
            case tokBitXor:
                Builder_appendInstruction(cb, cXOR(rRa(r5), rRb(r4)));
                break;
            case tokSal:
                Builder_appendInstruction(cb, cSAL(rRa(r5), rRb(r4)));
                break;
            case tokSar:
                Builder_appendInstruction(cb, cSAR(rRa(r5), rRb(r4)));
                break;
            case tokLt:
                Builder_appendInstruction(cb,
                               cCMP(rRa(r5), rRb(r4)));
                Builder_appendInstruction(cb,
                              cBAND(rRa(flg), xIMb(u64, flgLess)));
            case tokLte:
                Builder_appendInstruction(cb,
                               cCMP(rRa(r5), rRb(r4)));
                Builder_appendInstruction(cb,
                              cBAND(rRa(flg), xIMb(u64, flgLess|flgZero)));
            case tokGt:
                Builder_appendInstruction(cb,
                               cCMP(rRa(r5), rRb(r4)));
                Builder_appendInstruction(cb,
                              cBAND(rRa(flg), xIMb(u64, flgGreater)));
                break;
            case tokGte:
                Builder_appendInstruction(cb,
                               cCMP(rRa(r5), rRb(r4)));
                Builder_appendInstruction(cb,
                              cBAND(rRa(flg), xIMb(u64, flgGreater|flgZero)));
                break;
            default:
                unreachable();
        }
        break;

    case astLogicExpr: {
        Ident label;
        u32 pos;
        Codegen_generateCyn(cb, node->astLogic.lhs);
        label = Ident_genLabel();

#define Epilogue(INS) Builder_appendInstruction(cb, cPUSH(rRa(r5))); \
        Codegen_generateCyn(cb, node->astLogic.lhs);                 \
        Builder_appendInstruction(cb, cPOP(rRa(r4)));                \
        Builder_appendInstruction(cb, INS(rRa(r5), rRb(r4)))

        if (node->astLogic.op == tokOr) {
            Builder_appendInstruction(cb, cCMP(rRa(r5), xIMb(u8, 0)));
            pos = Builder_pos(cb);
            pos =
                Builder_addSymbolReference(cb, pos, label, &node->range, true);
            Builder_appendInstruction(cb, cJMPNZ(xIMb(u64, pos)));
            Epilogue(cOR);
        }
        else if (node->astLogic.op == tokAnd) {
            Builder_appendInstruction(cb, cCMP(rRa(r5), xIMb(u8, 0)));
            pos = Builder_pos(cb);
            pos =
                Builder_addSymbolReference(cb, pos, label, &node->range, true);
            Builder_appendInstruction(cb, cJMPZ(xIMb(u64, pos)));
            Epilogue(cAND);
        }
        else
            unreachable();

        pos = Builder_pos(cb);
        Builder_addSymbol(cb, pos, sytLabel, label, &node->range);
        break;
    }
    case astFuncDecl: {
        u32 offset = 0;
        Vector_foreach(&node->astFunc.params, param) {
            offset += (u32)ceil(
                (double)param->astVarDecl.type->size/sizeof(Value));
            // reserve size of variable
            Builder_appendInstruction(cb,
                                      cALLOCA(rRa(r5), xIMb(u64, offset)));
        }
    }

    }
}