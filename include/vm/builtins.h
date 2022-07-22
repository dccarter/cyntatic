/**
 * Copyright (c) 2022 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2022-07-20
 */

#pragma once

#include <vm/vm.h>
#include <vm/instr.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VM_NATIVE_OS_FUNCS(XX, UU)     \
    XX(Read,        read)                    \
    XX(Write,       write)                   \
    XX(Open,        open)                    \
    XX(Close,       close)                   \
    XX(Stat,        stat)                    \
    XX(Fstat,       fstat)                   \
    XX(Lstat,       lstat)                   \
    XX(Poll,        poll)                    \
    XX(Lseek,       lseek)                   \
    XX(Pipe,        pipe)                    \
    XX(Select,      select)                  \
    XX(Dup,         dup)                     \
    XX(Dup2,        dup2)                    \
    XX(Getpid,      getpid)                  \
    XX(Sendfile,    sendfile)                \
    XX(Socket,      socket)                  \
    XX(Connect,     connect)                 \
    XX(Accept,      accept)                  \
    XX(Sendto,      sendto)                  \
    XX(Recvfrom,    recvfrom)                \
    XX(Shutdown,    shutdown)                \
    XX(Bind,        bind)                    \
    XX(Listen,      listen)                  \
    XX(Getsockname, getsockname)             \
    XX(Getpeername, getpeername)             \
    XX(Fcntl,       fcntl)                   \
    UU(Flock,       flock)                   \
    UU(Fsync,       fsync)                   \
    UU(Fetcwd,      getcwd)                  \
    UU(Chdir,       chdir)                   \
    UU(Rename,      rename)                  \
    UU(Mkdir,       mkdir)                   \
    UU(Rmdir,       rmdir)                   \
    UU(Creat,       creat)                   \
    UU(Link,        link)                    \
    UU(Unlink,      unlink)                  \
    UU(Symlink,     symlink)                 \


typedef enum VirtualMachineBuiltinNativeCall {
#define XX(N, ...) bnc##N,
#define UU(N, ...) bnc##N,
    VM_NATIVE_OS_FUNCS(XX, UU)
#undef UU
#undef XX
    bncCOUNT
} BuiltinNativeCall;

extern NativeCall vmNativeBuiltinCallTbl[];

#define bncWRITE(FD, BUF, S, R)         \
    cPUSH(FD, dW),                      \
    cPUSH(BUF,dQ),                      \
    cPUSH(S,  dW),                      \
    cPUSH(xIMa(i32, 3), dW),            \
    cNCALL(xIMa(u32, bncWrite), dQ),    \
    cPOPN(xIMa(u32, 1)),                \
    cPOP(R, dW)

#define bncREAD(FD, BUF, S, R)  \
    cPUSH(FD),                  \
    cPUSH(BUF),                 \
    cPUSH(S),                   \
    cPUSH(xIMa(3,     i32)),    \
    cCALL(xIMa(bncRead, u32)), \
    cPOP(R)

#define bncOPEN(P, F, R)            \
    cPUSH(P),                       \
    cPUSH(F),                       \
    cPUSH(xIMa(2,     i32)),        \
    cNCALL(xIMa(bncOpen, u32)),     \
    cPOP(R)

#define bncOPENm(P, F, M, R)        \
    cPUSH(P),                       \
    cPUSH(F),                       \
    cPUSH(M),                       \
    cPUSH(xIMa(2,     i32)),        \
    cNCALL(xIMa(bncOpen, u32)),     \
    cPOP(R)

#define bncCLOSE(FD, R)             \
    cPUSH(FD),                      \
    cPUSH(xIMa(1,     i32)),        \
    cNCALL(xIMa(bncClose, u32)),    \
    cPOP(R)

#ifdef __cplusplus
}
#endif