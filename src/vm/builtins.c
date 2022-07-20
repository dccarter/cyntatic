/**
 * Copyright (c) 2022 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2022-07-20
 */

#include "vm/builtins.h"
#include "vm/vm.h"

#include <unistd.h>
#include <fcntl.h>

#define XX(I, N) static void vmBnc##I (VM *vm, const Value *args, u32 nargs);
#define UU(I, N) void vmBnc##I (VM *vm, const Value *args, u32 nargs)    \
{                                                                        \
    vmAssert(vm, false, "Builtin native call '" #N "' not implemented"); \
}

VM_NATIVE_OS_FUNCS(XX, UU)

#undef UU
#undef XX

NativeCall vmNativeBuiltinCallTbl[] = {
#define XX(I, N) vmBnc##I,
    VM_NATIVE_OS_FUNCS(XX, XX)
    NULL,
#undef XX
};

void vmBncWrite(VM *vm, const Value *args, u32 nargs)
{
    int fd;
    void *src;
    ssize_t size;

    vmAssert(vm, nargs == 3, "SYS_write requires at 2 arguments");

    fd = v2i(args[0]);
    src = &MEM(vm, v2i(args[1]));
    size = v2i(args[2]);
    size = write(fd, src, size);

    vmReturn(vm, u2v(size));
}

void vmBncRead(VM *vm, const Value *args, u32 nargs)
{
    int fd;
    void *src;
    ssize_t size;

    vmAssert(vm, nargs == 3, "SYS_read requires at 2 arguments");

    fd = v2i(args[0]);
    src = &MEM(vm, v2i(args[1]));
    size = v2i(args[2]);
    size = read(fd, src, size);

    vmReturn(vm, i2v(size));
}

void vmBncOpen(VM *vm, const Value *args, u32 nargs)
{
    int fd;
    void *path;
    int flags;
    mode_t mode = 0640;

    vmAssert(vm, nargs >= 2, "SYS_write requires at 2 arguments");

    path = &MEM(vm, v2i(args[0]));
    flags = v2i(args[1]);
    if (nargs == 3) mode = v2i(args[2]);

    fd = open(path, flags, mode);

    vmReturn(vm, i2v(fd));
}

void vmBncClose(VM *vm, const Value *args, u32 nargs)
{
    int fd;

    vmAssert(vm, nargs == 1, "SYS_write requires at 2 arguments");

    fd = v2i(args[0]);
    fd = close(fd);

    vmReturn(vm, i2v(fd));
}
