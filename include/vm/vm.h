/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author Carter
 * @date 2022-06-22
 */

#pragma once

#include <common.h>
#include <tree.h>
#include <vector.h>
#include <vm/value.h>

#include <stdio.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CYN_VM_NUM_REGISTERS
#define CYN_VM_NUM_REGISTERS 10
#endif

#ifndef CYN_VM_DEFAULT_SS
#define CYN_VM_DEFAULT_SS (sizeof(u64) * 1024)  // 8k stack by default
#endif

#ifndef CYN_VM_DEFAULT_MS
#define CYN_VM_DEFAULT_MS (1024 * 1024)         // default memory size 1MB
#endif

#ifndef CYN_VM_ALIGNMENT
#define CYN_VM_ALIGNMENT sizeof(uptr)
#endif

#ifndef CYN_VM_HEAP_DEFAULT_STH
#define CYN_VM_HEAP_DEFAULT_STH (16)
#endif

#ifndef CYN_VM_HEAP_DEFAULT_NHBS
#define CYN_VM_HEAP_DEFAULT_NHBS (256)
#endif

#ifdef CYN_VM_BUILD_DEBUG
#define CYN_VM_DEBUG_TRACE
#endif

/**
 * Represents a virtual machine instruction. Instructions size varies
 * by type of instruction (see \property osz)
 *
 * @property osz This represents the size of the instruction excluding
 * the size of the immediate value for instructions that takes on
 *
 * @property osc the op code for the instruction
 *
 * @property ra instruction argument A. Should be a memory reference or
 * register if the instruction takes 2 arguments.
 *
 * @property iam is argument A a memory reference (if set, argument A) will
 * be treated as a memory reference
 *
 * @property rmd the last argument mode, whether it's an register or an
 * immediate value (effective address?)
 *
 * @property imd the instruction mode, weather the instruction operates
 * on byte, shor, word, or q-word data
 *
 * @property ims if the last argument is an immediate value, this will
 * contain the  size of the immediate value
 *
 * @property rb instruction argument B. If the instruction takes 2 arguments
 * and, this holds the register ID if the argument B is passed as a register
 *
 * @property ibm is argument B a memory reference
 *
 * @property imm the immediate value for instructions that passed
 * an immediate value
 *
 * @property iea is effective addressing. Used if the second instruction
 * argument is a memory reference and the immediate value is an offset
 * into the reference
 */
typedef struct VirtualMachineInstruction {
    union {
        struct attr(packed) {
            u8 osz:2;
            u8 opc:6;
        };
        u8 b1;
    };
    union {
        struct attr(packed) {
            u8 ra:4;
            u8 iam:1;
            u8 rmd:1;
            u8 imd:2;
        };
        u8 b2;
    };
    union {
        struct attr(packed) {
            u8 ims:2;
            u8 rb:4;
            u8 ibm:1;
            u8 iea:1;
        };
        u8 b3;
    };

    union {
        u64 iu;
        i64 ii;
    };
} attr(packed) Instruction;

/**
 * A list of registers supported by the virtual machine
 *
 * `r0-r5` these are general purpose registers
 * `sp` the stack pointer register
 * `ip` the instruction pointer register
 * `bp` the base pointer register
 * `flg` the flags register (\see VirtualMachineFlags)
 */
typedef enum VirtualMachineRegister {
    r0,
    r1,
    r2,
    r3,
    r4,
    r5,
    sp,
    ip,
    bp,
    flg,
    regCOUNT
} Register;

/**
 * An array with the register names. Can be used to convert to
 * convert register id's to names
 */
extern const char *vmRegisterNameTbl[];

/**
 * Data modes supported by the virtual machine
 */
typedef enum VirtualMachineMode {
    szByte  = 0b00,
    szShort = 0b01,
    szWord = 0b10,
    szQuad = 0b11
} Mode;

#define SZ_u64 szQuad
#define SZ_u32 szWord
#define SZ_u16 szShort
#define SZ_u8  szByte
#define SZ_i64 szQuad
#define SZ_i32 szWord
#define SZ_i16 szShort
#define SZ_i8  szByte
#define SZ_f64 szQuad
#define SZ_f32 szWord
#define SZ_uptr szQuad

#define SZ_(T) CynPST(SZ_, T)

/**
 * A table used to convert virtual machine modes
 * to actual byte size
 */
extern const u8 vmSizeTbl[];

/**
 * A table used to convert virtual machine modes
 * to their string representation
 *
 * `szQuad` -> `.q`
 */
extern const char* vmModeNamesTbl[];

/**
 * A union used to copy u32 bytes to f32 type and
 * vice versa
 */
typedef union {
    f32 f;
    u32 u;
    u8  _b[4];
} FltU32;

/**
 * A union used to copy u64 bytes to f64 type and
 * vice versa
 */
typedef union {
    f64 f;
    u64 u;
    u8  _b[8];
} Flt64;

/**
 * A macro used to copy bytes from a float value \param V of size
 * \param B to an unsigned type of the same size
 */
#define f2uX(V, B) ({ CynPST(Flt, B) LineVAR(f) = {.f = (V)}; LineVAR(f).u; })

/**
 * A macro used to copy bytes from a unsigned value \param V of size
 * \param B to a float type of the same size
 */
#define u2fX(V, B) ({ CynPST(Flt, B) LineVAR(u) = {.u = (V)}; LineVAR(u).v; })

/**
 * A list of supported virtual machine argument mode
 *
 * `amReg` the argument is a register
 * `amImm` the argument is an immediate value
 */
typedef enum VirtualMachineArgMode {
    amReg,
    amImm
} ArgMode;

/**
 * A list of flags that can be set to the flags
 * register
 *
 * `flgZero` set when values compared by the `cmp` instruction
 * are equal
 *
 * `flgLess` set when the argument A of `cmp` instruction is
 * less than argument B
 *
 * `flgGreater` set when argument A of `cmp` instruction is
 * greater than argument B
 *
 */
typedef enum VirtualMachineFlags {
    flgZero = 0b10000000,
    flgLess = 0b01000000,
    flgGreater  = 0b00100000
} Flags;

/**
 * A list of virtual machine OP codes.
 *
 * @param XX this a macro used to generate from the
 * this list of opcode.
 */
#define VM_OP_CODES(XX)                 \
    XX(Halt,  halt, 0)                  \
    XX(Dbg,   dbg,  0)                  \
                                       \
    XX(Ret,   ret, 1)                  \
    XX(Jmp,   jmp, 1)                  \
    XX(Jmpz,  jmpz, 1)                 \
    XX(Jmpnz, jmpnz, 1)                \
    XX(Jmpg,  jmpg, 1)                 \
    XX(Jmps,  jmps, 1)                 \
    XX(Not,   lnot, 1)                 \
    XX(BNot,  bnot, 1)                 \
    XX(Inc,   inc, 1)                  \
    XX(Dec,   dec, 1)                  \
                                       \
    XX(Call,  call, 1)                 \
    XX(Push,  push, 1)                 \
    XX(Pop,   pop,  1)                 \
    XX(Popn,  popn, 1)                 \
    XX(Puti,  puti, 1)                 \
    XX(Puts,  puts, 1)                 \
    XX(Putc,  putc, 1)                 \
    XX(Mcpy,  mcpy, 1)                 \
    XX(Dlloc, dlloc, 1)                \
    XX(Ncall, ncall, 1)                \
                                       \
    XX(Alloca,alloca ,2)               \
    XX(Rmem,  rmem,2)                  \
    XX(Mov,   mov, 2)                  \
    XX(Add,   add, 2)                  \
    XX(Sub,   sub, 2)                  \
    XX(And,   and, 2)                  \
    XX(Or,    or, 2)                   \
    XX(Sar,   sar, 2)                  \
    XX(Sal,   sal, 2)                  \
    XX(Xor,   xor, 2)                  \
    XX(Bor,   bor, 2)                  \
    XX(Band,  band, 2)                 \
    XX(Mul,   mul, 2)                  \
    XX(Div,   div, 2)                  \
    XX(Mod,   mod, 2)                  \
    XX(Cmp,   cmp, 2)                  \
    XX(Alloc, alloc, 2)                \

/**
 * An enum listing all the op codes define above (\see VM_OP_CODES)
 */
typedef enum VirtualMachineOpCodes {
#define XX(N, ...) op##N,
    VM_OP_CODES(XX)
#undef XX

    opcCOUNT
} OpCodes;

typedef Pair(OpCodes, u8) OpCodeInfo;

#ifdef CYN_VM_BUILD_TOOL
OpCodeInfo VM_get_opcode_for_instr_(const char *instr, u32 len);
#define VM_get_opcode_for_instr(instr) VM_get_opcode_for_instr_((instr), strlen(instr))
Register Vm_get_register_from_str_(const char *str, u32 len);
#define Vm_get_register_from_str(str) Vm_get_register_from_str_((str), strlen(str))
#endif

/**
 * A conversion table used to convert opcodes to their respective
 * names
 */
extern const char* vmInstructionNamesTbl[];

/**
 * Code is stored in a vector
 */
typedef Vector(u8) Code;

/**
 * Defines the header of code that can be loaded into the virtual machine
 *
 * @property size the size of the code
 *
 * @property db the data boundary, this is basically the size of the data
 * block that should be copied to ram.
 *
 * @property main the first instruction that should be executed by the virtual machine
 *
 */
typedef struct VirtualMachineCodeHeader {
    u64 size;
    u32 db;
    u32 main;
    u8  code[0];
} attr(packed) CodeHeader;

/**
 * Holds information about the memory allocated for the virtual
 * machine
 *
 * @property base the start of the virtual machine memory
 *
 * @property top the last memory address accessible by the virtual
 * machine
 *
 * @property sb stack boundary, this marks the end of the virtual machine
 * stack. The stack grows from \property down to this boundary
 *
 * @property hb heap boundary, this marks the start of heap memory, heap
 * allocation will start from this address
 *
 * @property hlm heap limit, this marks the top of the total heap
 * memory. Heap allocations cannot be made past this address
 *
 * @property size the total size of memory allocated for the virtual machine
 */
typedef struct VirtualMachineMemory {
    u8 *ptr;
    u8 *base;
    u8 *top;
    u32 sb;
    u32 hb;
    u32 hlm;
    u32 size;
} Memory;

typedef enum VirtualMachineExecFlags {
    eflHalt  = BIT(0),
#ifdef CYN_VM_DEBUGGER
    eflDbgBreak = BIT(17)
#endif
} ExecFlags;

struct VirtualMachine;
typedef ExecFlags (*VirtualMachineDebugger)(struct VirtualMachine *, u32, const Instruction *);

/**
 * Holds virtual machine state
 *
 * @property flags virtual machine execution flags (\see VirtualMachineExecFlags)
 *
 * @property regs list of register used by the virtual machine
 *
 * @property ram virtual machine random access memory
 */
typedef struct VirtualMachine {
    u64 flags;
    u64 regs[regCOUNT];
    Code *code;
    Memory ram;
#ifdef CYN_VM_DEBUG_TRACE
    u8 dbgTrace;
#endif
#ifdef CYN_VM_DEBUGGER
    VirtualMachineDebugger debugger;
#endif
} VM;

typedef struct VirtualMachineHeapBlock {
    struct VirtualMachineHeapBlock *next;
    u32 size;
    u32 addr;
} attr(packed) HeapBlock;

typedef struct VirtualMachineMemoryHeap {
    HeapBlock *free;
    HeapBlock *used;
    HeapBlock *fresh;
    u32   top;
    u32   sth;
    u32   lmt;
    u8    aln;
    u8    mem[0];
} attr(packed) Heap;

/**
* Declaration for a native function
*/
typedef void(*NativeCall)(VM*, const Value*, u32);

/**
 * Should be invoked to exit the virtual. This will unconditionally
 * cause a an abnormal VM termination
 *
 * @param vm
 * @param fmt printf style string containing text to be written
 * to stderr
 *
 * @param ... additional arguments to use when formatting the string
 */
attr(noreturn)
attr(format, printf, 2, 3)
void VM_abort(VM *vm, const char *fmt, ...);

#define VM_assert(V, COND, FMT, ...) \
    if (!(COND)) VM_abort((V), "(" #COND "): " FMT, ##__VA_ARGS__)

/**
 * Macro used access the given register
 *
 * @param V the virtual machine whose register will be accessed
 * @param R the register to access
 */
#define REG(V, R) (vm)->regs[(R)]

/**
 * Macro used to access the virtual machine memory
 * at the given address
 */
attr(always_inline)
u8* MEM(VM *vm, u32 addr)
{
    if (addr > vm->ram.size)
        VM_abort(vm, "Memory access violation %x/%x", addr, vm->ram.hlm);

    return &vm->ram.base[addr];
}

typedef enum {
    trcEXEC = BIT(0),
    trcHEAP = BIT(1)
} VmDebugTraceLevel;

#ifdef CYN_VM_DEBUG_TRACE
/**
 * A helper macro to conditionally execute tracing code. Code surrounded
 * by this macro will compiled in only if tracing is allowed and
 * tracing for the specific module is enabled
 *
 * @param COMP the component that is executing the trace one of
 * `HEAP`, 'EXEC
 */
#define VM_dbg_trace(vm, COMP, ...) if (((vm)->dbgTrace & (COMP)) != 0) { __VA_ARGS__ ; }
#else
#define VM_dbg_trace(vm, COMP, ...)
#endif

/**
 * Push the given \param data onto the VM's stack. VM will abort
 * if there is a stack overflow
 *
 * @param vm
 * @param data the data to push onto the stack, if this value is `NULL`,
 * the size will be reserved on the stack
 * @param count the number of bytes to copy onto the stack
 *
 * @return Pointer to the data that was push
 */
attr(always_inline)
Value* VM_pushn(VM *vm, const Value *data, u8 count)
{
    u32 size = count << 3;
    if (((REG(vm, sp) - size) <= vm->ram.sb)) {
        VM_abort(vm, "VM stack memory overflow - collides with heap boundary");
    }

    REG(vm, sp) -= size;
    if (data != NULL)
        memcpy(MEM(vm, REG(vm, sp)), data, size);

    return (Value *)MEM(vm, REG(vm, sp));
}

/**
 * Helper macro to push a value onto the stack
 */
#define VM_push(vm, val) ({ VM_pushn((vm), NULL, 1)->i = (val); })

/**
 * Pops some data off the virtual machine's stack. VM will abort
 * if there is a stack overflow
 *
 * @param vm
 * @param data pointer to a variable to hold the popped data. This can
 * be `NULL` in which case nothing will be copied out
 * @param count number of entries to pop off the stack
 *
 * @return Address of the item pop out
 */
attr(always_inline)
Value* VM_popn(VM *vm, Value *data, u8 count)
{
    Value* ret;
    u32 size = count << 3;
    if (((REG(vm, sp) + size) > vm->ram.size)) {
        VM_abort(vm, "VM stack memory underflow - escapes virtual machine memory");
    }

    ret = (Value *) MEM(vm, REG(vm, sp));
    if (data != NULL)
        memcpy(data, ret, size);
    REG(vm, sp) += size;
    return ret;
}

/**
 * Helper macro to pop a value of the given type \param T from
 * the stack
 */
#define VM_pop(vm, T) ({ (T)VM_popn((vm), NULL, 1)->i; })

/**
 * Used by native/sys calls to return values to
 * the system
 *
 * @param vm
 * @param vals
 * @param count
 */
void VM_returnx(VM *vm, Value *vals, u32 count);

/**
 * Returns an arbitrary number of values to the VM
 */
#define VM_return(V, ...)                                        \
    ({                                                          \
        Value LineVAR(v)[] = {{.i = 0}, ##__VA_ARGS__};         \
        VM_returnx((V), LineVAR(v)+1, sizeof__(LineVAR(v))-1);  \
    })


/**
 * Initialize the virtual machine
 *
 * @param vm The virtual machine to initialize
 * @param code The code to load onto the virtual machine
 * @param mem the total size of the ram to be allocated for the
 * virtual machine
 * @param ss the size of the stack
 */
void VM_init_(VM *vm, Code *code, u64 mem, u32 nhbs, u32 ss);

/**
 * Helper macro to initialize the virtual machine with the
 * default stack value of \see CYN_VM_DEFAULT_SS
 *
 * @param V the virtual machine to initialize
 * @param CD the code to load onto the virtual machine
 * @param S the total size of the ram to be allocated for the
 * virtual machine
 */
#define VM_init(V, CD, S) VM_init_((V), (CD), (S), CYN_VM_HEAP_DEFAULT_NHBS, CYN_VM_DEFAULT_SS)

/**
 * Run the code loaded onto the virtual machine, parsing
 * in the given command line arguments
 *
 * @param vm The virtual machine to run
 * @param argc the number of arguments to pass to
 * the virtual machine
 * @param argv an list of string arguments passed to the
 * virtual machine
 */
void VM_run(VM *vm, int argc, char *argv[]);

/**
 * De-initialize the given virtual machine
 *
 * @param vm
 */
void VM_deinit(VM *vm);

/**
 * Initialize heap memory allocator
 *
 * @param vm the virtual whose memory allocator should be initialized
 * @param sth the memory split threshold for splitting chunks
 * @param alignment memory alignment
 */
void VM_heap_init_(VM *vm, u32 blocks, u32 sth, u8 alignment);

/**
 * Helper macro to initialize the virtual machine heap with default
 * values
 */
#define VM_heap_init(vm, NBS) \
    VM_heap_init_(vm, (NBS), CYN_VM_HEAP_DEFAULT_STH, CYN_VM_ALIGNMENT)

/**
 * Allocate memory from virtual machine's heap
 *
 * @param vm
 * @param size
 *
 * @return address of the memory (index in the heap memory)
 */
u32  VM_alloc(VM *vm, u32 size);

/**
 * Free previously allocated memory
 *
 * @param vm
 * @param mem
 * @return
 */
bool VM_free(VM *vm, u32 mem);

u32 VM_cstring_dup_(VM *vm, const char *s, u32 len);

#define VM_cstring_dup(V, S) VM_cstring_dup_((V), (S), strlen(S))

/**
 * A helper function to compute the mode of an
 * immediate value
 *
 * @param imm
 * @return
 */
attr(always_inline)
Mode VM_integer_size(u64 imm)
{
    if (imm <= 0x7F) return szByte;
    if (imm <= 0x7FFF) return szShort;
    if (imm <= 0x7FFFFFFF) return szWord;
    return szQuad;
}

/**
 * Given a source pointer and value mode, reads the integer value
 * stored at the address, computing the size of the value using the
 * given mode
 *
 * @param src
 * @param size
 *
 * @return
 */
attr(always_inline)
static i64 VM_read(const void *src, Mode size)
{
    i64 ret;
    switch (size) {
        case szByte:  ret = u2iX(*((u8 *)src), 8); break;
        case szShort: ret = u2iX(*((i16 *)src), 16); break;
        case szWord:  ret = u2iX(*((i32 *)src), 32); break;
        case szQuad:  ret = u2iX(*((i64 *)src), 64); break;
        default:
            unreachable("!!!!");
    }

    return ret;
}

/**
 * Given a destination pointer, source integer value and value mode,
 * writes the integer value to the given address, computing the integral
 * type of the number using the given mode.
 *
 * @param dst
 * @param src
 * @param size
 */
attr(always_inline)
static void VM_write(void *dst, i64 src, Mode size)
{
    switch (size) {
        case szByte:
            *((u8 *) dst) = i2uX(src, 8);
            break;
        case szShort:
            *((u16 *)dst) = i2uX(src, 16);
            break;
        case szWord:
            *((u32 *)dst) = i2uX(src, 32);
            break;
        case szQuad:
            *((u64 *)dst) = i2uX(src, 64);
            break;
        default:
            unreachable("!!!!");
    }
}

void VM_code_append_(Code *code, const Instruction *seq, u32 sz);
void* VM_code_append_data_(Code *code, const void *data, u32 sz);
#define VM_code_append_data(C, D, N) \
    ({ u32 LineVAR(l) = Vector_len(C); VM_code_append_data_((C), (D), (N)); LineVAR(l); })

#define VM_code_append_number(C, N) \
    ({ \
        u32 LineVAR(l) = Vector_len(C); \
        __typeof__(N)* LineVAR(n) = VM_code_append_data_((C), NULL, sizeof(N)); *LineVAR(n) = (N); LineVAR(l); \
    })

#define VM_code_append_stringX(C, S, N) \
    ({u32 LineVAR(l) = Vector_len(C); VM_code_append_data_((C), (S), (N)); LineVAR(l); })
#define VM_code_append_string(C, S)    VM_code_append_stringX((C), (S), strlen(S))
#define VM_code_reserve_data(C, T, N)  \
    ({u32 LineVAR(l) = Vector_len(C); VM_code_append_data_((C), NULL, sizeof(T)*(N)); LineVAR(l);})

#define VM_code_append(C, ...) \
    ({ Instruction LineVAR(cc)[] = {{}, ##__VA_ARGS__}; \
       VM_code_append_((C), LineVAR(cc)+1, sizeof__(LineVAR(cc))-1); \
    })

void VM_code_disassemble_(Code *code, FILE *fp, bool showAddr);
#define VM_code_disassemble(C, F) VM_code_disassemble_((C), (F), true)

u32 VM_code_instruction_at(const Code *code, Instruction* instr, u32 iip);
void VM_code_print_instruction_(const Instruction* instr, FILE *fp);
#define VM_code_print_instruction(I) VM_code_print_instruction_((I), stdout)


void VM_put_utf8_chr_(VM *vm, u32 chr, FILE *fp);
#define VM_put_utf8_chr(CH, FP) VM_put_utf8_chr_(NULL, (CH), FP)

#ifdef __cplusplus
}
#endif
