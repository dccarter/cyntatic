/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-16
 */

#include "vm/assembler.hpp"
#include "compiler/source.hpp"
#include "compiler/lexer.hpp"

namespace cyn {

    bool Assembler::EoT()
    {
        return _current == _tokens.second || _current->kind == Token::EoF;
    }

    Token::Ref Assembler::advance()
    {
        if (_current != _tokens.second) {
            auto tok = _current;
            std::advance(_current, 1);
            return tok;
        }
        else {
            return _tokens.second;
        }
    }

    Token::Ref Assembler::peek()
    {
        if (_current != _tokens.second)
            return std::next(_current);
        return _tokens.second;
    }

    Token::Ref Assembler::previous()
    {
        if (_current != _tokens.first)
            return std::next(_current, -1);
        return _current;
    }

    void Assembler::synchronize()
    {
        advance();
        while (!EoT()) {
            if (previous()->kind == Token::Nl) break;
            advance();
        }
    }

    void Assembler::parseLabel()
    {
        auto& tok = *consume(Token::IDENTIFIER, "expecting label name");
        auto name = tok.range().toString();
        auto it = _symbols.find(name);
        if (it != _symbols.end()) {
            L.error(tok.range(), "label '", name, "' already defined");
            throw ParseError{};
        }

        _symbols.emplace(name, Symbol{u32(_instructions.size()), Symbol::symLabel});
        _symRefs[_instructions.size()] = {};
    }

    void Assembler::parseVarDcl()
    {
        advance();
        auto tok = consume(Token::IDENTIFIER, "expecting name of variable");
        auto name = tok->range().toString();
        consume(Token::ASSIGN, "expecting assignment operator '='");
        auto pos = _constants.size() + sizeof(CodeHeader);
        auto it = _symbols.emplace(name, Symbol{u32(pos), Symbol::symVar});
        if (!it.second) {
            fail("symbol with '", name, "' already declared");
        }

        if (match(Token::LBRACE)) {
            auto patch = (u32)_constants.size();
            _constants.insert(_constants.end(), (u8 *)&patch, (u8 *)&patch+sizeof(patch));

            u32 size = 0;
            do {
                tok = current();
                u32 val = 0;
                if (tok->kind == Token::CHAR) {
                    val = tok->value<u32>();
                }
                else if (tok->kind == Token::INTEGER){
                    val = tok->value<u64>();
                }
                else {
                    fail("unexpected token");
                }

                advance();
                if (val > 0xFFu) {
                    error("assembler only supports characters in range 0-255");
                }

                _constants.push_back((u8)val);
                size++;
            } while (match(Token::COMMA));

            *((u32 *)&_constants[patch]) = size;
            consume(Token::RBRACE, "expecting a closing brace '}'");
        }
        else {
            tok = consume(Token::STRING, "expecting string constant");
            auto& str = tok->value<std::string>();
            auto size = (u32) str.size();
            auto tag = _constants.size();
            _constants.insert(_constants.end(), (u8 *)&size, (u8 *)&size+sizeof(u32));
            *((u32 *)&_constants[tag]) = size;
            _constants.insert(_constants.end(), str.begin(), str.end());
        };
    }

    void Assembler::parseInstruction()
    {
        static const std::unordered_map<std::string_view, std::pair<OpCodes, u8>> sInstructions = {
#define XX(OP, N, SZ) {#N, {op##OP, SZ}},
            VM_OP_CODES(XX)
#undef XX
        };

        auto& tok = *current();
        auto name = tok.range().toString();
        auto it = sInstructions.find(name);
        if (it == sInstructions.end()) {
            fail("unsupported instruction '", name, "'");
        }

        advance();

        auto [op, nargs] = it->second;
        Instruction instr = {0};
        instr.osz = nargs + 1;
        instr.opc = op;
        if (nargs >= 1) {
            auto [isMem, reg] = parseInstructionArg(instr);
            instr.iam = isMem;
            instr.ra = reg;
        }
        if (nargs == 2) {
            auto [isMem, reg] = parseInstructionArg(instr);
            instr.ibm = isMem;
            instr.rb = reg;
        }

        _instructions.push_back(instr);
    }

    Assembler::Symbol& Assembler::findSymbol(std::string_view name, const Range& range)
    {
        auto it = _symbols.find(name);
        if (it == _symbols.end()) {
            L.error(range, "referenced symbol '", name, "' does not exist");
            throw ParseError{};
        }
        return it->second;
    }

    u32 Assembler::addSymbolRef(u32 pos, std::string_view name, const Range& range)
    {
        auto it = _symbols.find(name);
        if (it == _symbols.end() or it->second.tag == Symbol::symLabel) {
            _patchWork.emplace(pos, std::pair{name, range});
        }
        else
            return it->second.id;
    }

    std::pair<bool, Register> Assembler::parseInstructionArg(Instruction &instr)
    {
        static const std::unordered_map<std::string_view, Register> sRegisters = {
                {"r0", r0},
                {"r1", r1},
                {"r2", r2},
                {"r3", r3},
                {"r4", r4},
                {"r5", r5},
                {"sp", sp},
                {"ip", ip},
                {"bp", bp},
                {"flg", flg},
        };

        bool isMem = match(Token::LBRACKET);
        bool isNeg = match(Token::MINUS);
        match(Token::PLUS);

        Register reg{r0};

        if (!check(Token::INTEGER, Token::CHAR, Token::FLOAT, Token::IDENTIFIER))
               fail("expecting either a number, char literal, register, variable or label");

        auto tok = advance();
        if (tok->kind == Token::IDENTIFIER) {
            auto name = tok->range().toString();
            auto it = sRegisters.find(name);
            if (it == sRegisters.end()) {
                instr.type = dtImm;
                instr.size = szDWord;
                instr.iu = addSymbolRef(_instructions.size(), name, tok->range());
            }
            else {
                instr.type = dtReg;
                reg = it->second;
            }
        }
        else {
            instr.type = dtImm;
            switch (tok->kind) {
                case Token::CHAR:
                    instr.iu = tok->value<u32>();
                    instr.size = SZ_(u32);
                    break;
                case Token::FLOAT:
                    instr.iu = f2u64(tok->value<double>());
                    instr.size = SZ_(u64);
                    break;
                case Token::INTEGER: {
                    union { i64 i; u64 u; } imm = {.u = tok->value<u64>() };
                    instr.size = vmIntegerSize(imm.u);
                    if (isNeg) imm.i = -(i64)imm.u;
                    instr.iu = imm.u;
                    break;
                }
                default:
                    unreachable("should get to this point");
            }
        }
        if (isMem)
            consume(Token::RBRACKET, "expecting a closing ']' to terminate instruction argument");

        return {isMem, reg};
    }
    
    u32 Assembler::assemble(Code &code)
    {
        while (!EoT()) {
            try {
                switch (current()->kind) {
                    case Token::IDENTIFIER:
                        parseInstruction();
                        break;

                    case Token::COLON:
                        advance();
                        parseLabel();
                        break;

                    case Token::DOLLAR:
                        parseVarDcl();
                        break;
                    case Token::Nl:
                        advance();
                        break;
                    default:
                        fail("unsupported token");
                }
            }
            catch(...) {
                synchronize();
            }
        }

        if (L.hasErrors())
            return 0;
        std::unordered_map<u32, vec<u32>> refs;
        for (auto& [id, work] : _patchWork) {
            auto sym = _symbols.find(work.first);
            if (sym->second.tag == Symbol::symLabel) {
                auto it = refs.emplace(sym->second.id, vec<u32>{});
                it.first->second.push_back(id);
            }
            else {
                L.error(work.second, "variable '", work.first, "' should be defined before use");
            }
        }

        u8 tmp[sizeof(CodeHeader)] = {0};
        Vector_pushArr(&code, tmp, sizeof(tmp));
        Vector_pushArr(&code, &_constants[0], _constants.size());
        auto db = Vector_len(&code);

        i32 ip = db;
        {
            vec<i32> ips(_instructions.size(), 0);
            for (int i = 0; i < _instructions.size(); i++) {
                ips[i] += ip;
                auto &instr = _instructions[i];
                {
                    auto it = refs.find(i);
                    if (it != refs.end()) {
                        for (auto j: it->second) {
                            _instructions[j].ii = (ip - ips[j+1]);
                        }
                    }
                }

                if (_patchWork.find(i) != _patchWork.end()) {
                    instr.ii -= ip;
                }

                ip += instr.osz;
                if (instr.type == dtImm) {
                    static const u8 sztbl_[] = {1, 2, 4, 8};
                    ip += sztbl_[instr.size];
                }
            }
        }

        for (auto& instr: _instructions) {
            vmCodeAppend_(&code, &instr, 1);
        }

        auto header = (CodeHeader *) Vector_begin(&code);
        header->size = Vector_len(&code);
        header->main = db + _main;
        header->db = db;

        return header->size;
    }

}


int main(int argc, char *argv[])
{
    cyn::Source src("<stdin>", R"(
$hello = "Hello World Hello World Hello World"

:main
    mov r0 -1000
    add r0 -3
    puti r0

:exit
    halt
)");
    cyn::Log Log;
    cyn::Lexer L(Log, src);
    if (!L.tokenize()) {
        cyn::abortCompiler(Log);
    }

    cyn::Assembler assembler(Log, L.tange());
    Code  code;
    Vector_init(&code);
    assembler.assemble(code);
    if (Log.hasErrors()) {
        cyn::abortCompiler(Log);
    }

    u32 i = 0;
    for (auto j = sizeof(CodeHeader); j < Vector_len(&code); j++) {
        printf("%02x ", *Vector_at(&code, j));
        if (++i % 8 == 0) printf("\b\n");
    }
    std::cout << std::endl;

    VM vm;
    vmInit(&vm, CYN_VM_DEFAULT_MS);
    vmRun(&vm, &code, argc, argv);
    return 0;
}