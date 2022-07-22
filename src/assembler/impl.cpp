/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-20
 */

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
        if (name == "main") _main = _instructions.size();
    }

    void Assembler::parseVarDcl()
    {
        advance();
        auto tok = consume(Token::IDENTIFIER, "expecting name of variable");
        auto name = tok->range().toString();
        consume(Token::ASSIGN, "expecting assignment operator '='");
        auto pos = _constants.size() + sizeof(CodeHeader);
        auto it = _symbols.emplace(name, Symbol{u32(pos), 0, Symbol::symVar});
        if (!it.second)
            fail(tok->range(), "symbol with '", name, "' already declared");

        u32 size = 0;
        if (match(Token::LBRACE)) {
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
            consume(Token::RBRACE, "expecting a closing brace '}'");
        }
        else if (check(Token::STRING)) {
            tok = advance();
            auto &str = tok->value<std::string>();
            size = (u32) str.size();
            _constants.insert(_constants.end(), str.begin(), str.end());
        }
        else {
            tok = current();
            bool isNeg = match(Token::MINUS);
            if (!isNeg) match(Token::PLUS);

            switch (current()->kind) {
                case Token::INTEGER: {
                    auto value = u2i(advance()->value<u64>());
                    if (isNeg) value = -value;
                    auto mode = szQuad;
                    if (match(Token::BACKQUOTE))
                        mode = parseMode();
                    size = appendIntegralData(value, mode);
                    break;
                }
                case Token::CHAR:
                    if (tok != current())
                        fail(tok->range(), "unexpected token before a character literal");
                    size = appendIntegralData(u2i(advance()->value<u32>()), szByte);
                    break;
                case Token::FLOAT: {
                    auto value = advance()->value<double>();
                    if (isNeg) value = -value;
                    auto mode = szQuad;
                    if (match(Token::BACKQUOTE))
                        mode = parseMode({szWord, szQuad});
                    size = appendIntegralData(f2i(value), mode);
                    break;
                }
                case Token::LBRACKET: {
                    advance();
                    tok = consume(Token::INTEGER, "expecting an integer to indicate number of bytes");
                    size = vmSizeTbl[szByte];
                    if (match(Token::BACKQUOTE))
                        size = vmSizeTbl[parseMode()];
                    consume(Token::RBRACKET, "expecting an ']' to end memory reservation block");
                    size *= tok->value<u64>();
                    _constants.resize(size);
                    break;
                }
                default:
                    fail("unsupported initialization data format");
            }
        }

        it.first->second.size = size;
    }

    Mode Assembler::parseMode(const std::vector<Mode> &modes)
    {
        auto tok = *consume(Token::IDENTIFIER, "expecting either a 'b'/'s'/'w'/'q'");
        auto ext = tok.range().toString();

        Mode mode = szByte;
        if (ext.size() == 1) {
            switch (ext[0]) {
                case 'b' :
                    break;
                case 's' :
                    mode = szShort;
                    break;
                case 'w' :
                    mode = szWord;
                    break;
                case 'q' :
                    mode = szQuad;
                    break;
                default:
                    goto invalidExtension;
            }
        }
        else {
            invalidExtension:
            fail(tok.range(), "unsupported instruction '", ext, "', use b/s/w/q");
        }

        if (!modes.empty() and std::find(modes.begin(), modes.end(), mode) == modes.end()) {
            std::string supported = join(modes, "/", [](Mode m) { return vmSizeTbl[m]; });
            fail("mode '", vmModeNamesTbl[mode], "' not supported in current context, use ", supported);
        }

        return mode;
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
        instr.imd = szQuad;

        if (match(Token::DOT))
            instr.imd = parseMode();

        if (nargs >= 1) {
            auto [isMem, reg] = parseInstructionArg(instr);
            instr.iam = isMem;
            if (instr.rmd == amImm)
                instr.ra = instr.ims;
            else
                instr.ra = reg;
        }
        if (nargs == 2) {
            auto [isMem, reg] = parseInstructionArg(instr, true);
            instr.ibm = isMem;
            instr.rb = reg;
        }

        _instructions.push_back(instr);
    }

    Assembler::Symbol& Assembler::findSymbol(std::string_view name, const Range& range)
    {
        auto it = _symbols.find(name);
        if (it == _symbols.end())
            fail("referenced symbol '", name, "' does not exist");
        return it->second;
    }

    u32 Assembler::addSymbolRef(u32 pos, std::string_view name, const Range& range, bool addToPatchWork)
    {
        auto it = _symbols.find(name);
        if (it == _symbols.end() or it->second.tag == Symbol::symLabel) {
            if (addToPatchWork) {
                _patchWork.emplace(pos, std::pair{name, range});
                return 0;
            }
            fail("referenced symbol '", name, "' must be defined before use");
        }
        else
            return it->second.id;
    }

    u32 Assembler::getVariableSize(std::string_view name, const Range &range)
    {
        auto it = _symbols.find(name);
        if (it == _symbols.end())
            fail(range, "reference to undefined variable '", name, "'");
        if (it->second.tag != Symbol::symVar)
            fail(range, "cannot not read size of non variable symbol '", name, '"');

        return it->second.size;
    }

    u32 Assembler::appendIntegralData(i64 value, Mode mode)
    {
        const auto size = vmSizeTbl[mode];
        const auto pos = _constants.size();
        _constants.resize(pos+size);
        vmWrite(&_constants[pos], value, mode);

        return size;
    }

    std::pair<bool, Register> Assembler::parseInstructionArg(Instruction &instr, bool isRb)
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
        auto sign = current();
        bool isNeg = match(Token::MINUS);
        match(Token::PLUS);

        Register reg{r0};

        if (!check(Token::INTEGER, Token::CHAR, Token::FLOAT, Token::IDENTIFIER, Token::HASH))
            fail("expecting either a number, char literal, register, variable or label");

        bool isSizeOperator = match(Token::HASH);
        auto tok = advance();

        if (tok->kind == Token::IDENTIFIER) {
            if (sign->kind == Token::MINUS or sign->kind == Token::PLUS)
                fail(sign->range(), "+/- not allowed on variables");

            auto name = tok->range().toString();
            auto it = sRegisters.find(name);
            if (it == sRegisters.end()) {
                instr.rmd = amImm;
                instr.iu = (isSizeOperator?
                            getVariableSize(name, tok->range()) :
                            addSymbolRef(_instructions.size(), name, tok->range()));
                instr.ims = instr.iu == 0? szWord : vmIntegerSize(instr.iu);

                if (instr.iu > 0 and check(Token::PLUS, Token::MINUS)) {
                    isNeg = match(Token::MINUS);
                    match(Token::PLUS);
                    tok = consume(Token::INTEGER, "expecting an integer literal to add to a variable");
                    union { i64 i; u64 u; } imm = {.u = tok->value<u64>() };
                    instr.ims = vmIntegerSize(imm.u);
                    if (isNeg) imm.i = -(i64)imm.u;
                    instr.iu += imm.i;
                }
            }
            else {
                if (isSizeOperator)
                    fail(tok->range().extend(current()->range()), "'#' operator cannot be applied to register types");
                instr.rmd = amReg;
                reg = it->second;
            }

            if (isMem and isRb and match(Token::COMMA)) {
                sign = current();
                isNeg = match(Token::MINUS);
                if (!isNeg) match(Token::PLUS); // discard plus sign

                tok = advance();
                switch (tok->kind) {
                    case Token::INTEGER: {
                        union { i64 i; u64 u; } imm = {.u = tok->value<u64>() };
                        instr.ims = vmIntegerSize(imm.u);
                        if (isNeg) imm.i = -(i64)imm.u;
                        instr.iu += imm.i;
                        break;
                    }
                    case Token::HASH:
                        isSizeOperator = true;
                        if (check(Token::IDENTIFIER))
                            fail("'#' operator must be followed by an identifier");
                        // fallthrough
                    case Token::IDENTIFIER:
                        if (sign->kind == Token::MINUS or sign->kind == Token::PLUS)
                            fail(sign->range(), "+/- not allowed on variables");
                        name = tok->range().toString();
                        instr.ims = szWord;
                        instr.ii += (isSizeOperator?
                                     getVariableSize(name, tok->range()) :
                                     addSymbolRef(_instructions.size(), name, tok->range(), false));
                        break;
                    default:
                        L.error(tok->range(), "unexpected effective address, only integers and variables allowed");
                        throw ParseError();
                }
                instr.iea = 1;
            }
        }
        else {
            instr.rmd = amImm;
            switch (tok->kind) {
                case Token::CHAR:
                    instr.iu = tok->value<u32>();
                    instr.ims = SZ_(u32);
                    break;
                case Token::FLOAT:
                    instr.iu = f2u(tok->value<double>());
                    instr.ims = SZ_(u64);
                    break;
                case Token::INTEGER: {
                    union { i64 i; u64 u; } imm = {.u = tok->value<u64>() };
                    instr.ims = vmIntegerSize(imm.u);
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

    Assembler::Assembler(Log& L, Token::Tange tange)
            : L{L},
              _tokens{std::move(tange)}
    {
        _current = _tokens.first;
        init();
    }


    void Assembler::init()
    {
        // argc can be used in source to refer to
        // the position of the number of function arguments in stack
        define("argc", 16);
        define("argv", 24);

        // define builtin variables
#define XX(I, N) define("__"#N, u64(bnc##I));
#define UU(...)
        VM_NATIVE_OS_FUNCS(XX, UU)
#undef UU
#undef XX

        define("__stdin",  STDIN_FILENO);
        define("__stdout", STDOUT_FILENO);
        define("__stderr", STDERR_FILENO);
    }

    void Assembler::define(std::string_view name, u64 value)
    {
        auto it = _symbols.emplace(name, Symbol{value, 0, Symbol::symDefine});
        if (!it.second) {
            L.error({}, "variable '", name, "' already defined");
            abortCompiler(L);
        }
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
                    case Token::COMMENT:
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
            if (sym == _symbols.end()) {
                L.error(work.second, "undefined symbol '", work.first, "' referenced");
                continue;
            }

            if (sym->second.tag == Symbol::symLabel) {
                auto it = refs.emplace(sym->second.id, vec<u32>{});
                it.first->second.push_back(id);
            }
        }

        u8 tmp[sizeof(CodeHeader)] = {0};
        Vector_pushArr(&code, tmp, sizeof(tmp));
        Vector_pushArr(&code, &_constants[0], _constants.size());
        auto db = Vector_len(&code);

        i32 ip = db;
        for (int i = 0; i < _instructions.size(); i++) {
            auto &instr = _instructions[i];

            if (_patchWork.find(i) != _patchWork.end()) {
                instr.ii -= ip;
            }

            {
                auto it = refs.find(i);
                if (it != refs.end()) {
                    for (auto j: it->second) {
                        _instructions[j].ii += ip;
                    }
                }
            }

            ip += instr.osz;
            if (instr.rmd == amImm or instr.iea) {
                ip += vmSizeTbl[instr.ims];
            }
        }

        vmCodeAppend_(&code, &_instructions[0], _instructions.size());

        auto header = (CodeHeader *) Vector_begin(&code);
        header->size = Vector_len(&code);
        header->main = db + _main;
        header->db = db;

        return header->size;
    }
}
