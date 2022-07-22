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
#include <poll.h>

#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/socket.h>

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

    vmAssert(vm, nargs == 3, "BncWrite requires at 3 arguments");

    fd = v2i(args[0]);
    src = (void *) v2p(args[-1]);
    size = v2i(args[-2]);
    size = write(fd, src, size);

    vmReturn(vm, u2v(size));
}

void vmBncRead(VM *vm, const Value *args, u32 nargs)
{
    int fd;
    void *src;
    ssize_t size;

    vmAssert(vm, nargs == 3, "BncRead requires at 3 arguments");

    fd = v2i(args[0]);
    src = (void *)v2p(args[-1]);
    size = v2i(args[-2]);
    size = read(fd, src, size);

    vmReturn(vm, i2v(size));
}

void vmBncOpen(VM *vm, const Value *args, u32 nargs)
{
    int fd;
    void *path;
    int flags;
    mode_t mode = 0640;

    vmAssert(vm, nargs == 2 || nargs == 3, "BncOpen requires at least 2 arguments");

    path = (void *)v2p(args[0]);
    flags = v2i(args[-1]);
    if (nargs == 3) mode = v2i(args[-2]);

    fd = open(path, flags, mode);

    vmReturn(vm, i2v(fd));
}

void vmBncClose(VM *vm, const Value *args, u32 nargs)
{
    int fd;

    vmAssert(vm, nargs == 1, "BncClose requires 1 argument");

    fd = v2i(args[0]);
    fd = close(fd);

    vmReturn(vm, i2v(fd));
}

void vmBncStat(VM *vm, const Value *args, u32 nargs)
{
    int ret;
    void *path;
    struct stat *st;

    vmAssert(vm, nargs == 2, "BncStat requires 2 arguments");

    path = (void *)v2p(args[0]);
    st = (struct stat*) v2p(args[-1]);

    ret = stat(path, st);

    vmReturn(vm, i2v(ret));
}

void vmBncLstat(VM *vm, const Value *args, u32 nargs)
{
    int ret;
    void *path;
    struct stat *st;

    vmAssert(vm, nargs == 2, "BncLstat requires 2 arguments");

    path = (void *)v2p(args[0]);
    st = (struct stat*) v2p(args[-1]);

    ret = lstat(path, st);

    vmReturn(vm, i2v(ret));
}

void vmBncFstat(VM *vm, const Value *args, u32 nargs)
{
    int fd;
    struct stat *st;

    vmAssert(vm, nargs == 2, "BncFstat requires 2 arguments");

    fd = v2i(args[0]);
    st = (struct stat*) v2p(args[-1]);

    fd = fstat(fd, st);

    vmReturn(vm, i2v(fd));
}

void vmBncPoll(VM *vm, const Value *args, u32 nargs)
{
    struct pollfd *fds;
    nfds_t nfds;
    int timeout, ret;

    vmAssert(vm, nargs == 3, "BncPoll requires 3 arguments");

    fds = (struct pollfd *) v2i(args[0]);
    nfds = v2i(args[-1]);
    timeout = v2i(args[-2]);

    ret = poll(fds, nfds, timeout);

    vmReturn(vm, i2v(ret));
}

void vmBncLseek(VM *vm, const Value *args, u32 nargs)
{
    int fd;
    off_t offset;
    int whence;

    vmAssert(vm, nargs == 3, "BncLseek requires 3 arguments");

    fd = v2i(args[0]);
    offset = v2i(args[-1]);
    whence = v2i(args[-2]);

    offset = lseek(fd, offset, whence);

    vmReturn(vm, i2v(offset));
}

void vmBncPipe(VM *vm, const Value *args, u32 nargs)
{
    int *pipefd, ret;

    vmAssert(vm, nargs == 1, "BncPipe requires 1 argument");

    pipefd =  (void *) v2p(args[0]);;

    ret = pipe(pipefd);

    vmReturn(vm, i2v(ret));
}

void vmBncSelect(VM *vm, const Value *args, u32 nargs)
{
    int nfds, ret;
    fd_set *readfds, *writefds, *exceptfds;
    struct timeval *timeout;

    vmAssert(vm, nargs == 5, "BncSelect requires 5 arguments");

    nfds = v2i(args[0]);;
    readfds = (void *)v2p(args[-1]);
    writefds = (void *)v2p(args[-2]);
    exceptfds = (void *)v2p(args[-3]);
    timeout = (void *)v2p(args[-4]);

    ret = select(nfds, readfds, writefds, exceptfds, timeout);

    vmReturn(vm, i2v(ret));
}

void vmBncDup(VM *vm, const Value *args, u32 nargs)
{
    int fd;

    vmAssert(vm, nargs == 1, "BncDup requires 1 argument");

    fd =  v2i(args[0]);;

    fd = dup(fd);

    vmReturn(vm, i2v(fd));
}

void vmBncDup2(VM *vm, const Value *args, u32 nargs)
{
    int fd, newfd;

    vmAssert(vm, nargs == 1, "BncDup2 requires 1 argument");

    fd =  v2i(args[0]);;
    newfd =  v2i(args[-1]);;

    fd = dup2(fd, newfd);

    vmReturn(vm, i2v(fd));
}

void vmBncGetpid(VM *vm, const Value *args, u32 nargs)
{
    vmAssert(vm, nargs == 0, "BncGetpid requires 0 arguments");

    vmReturn(vm, u2v(getpid()));
}

void vmBncSendfile(VM *vm, const Value *args, u32 nargs)
{
    ssize_t ret;
    int outfd, infd;
    off_t *offset;
    size_t count;

    vmAssert(vm, nargs == 4, "BncSendfile requires 4 argument");

    outfd = v2i(args[0]);
    infd = v2i(args[-1]);
    offset = (void *)v2p(args[-2]);
    count = v2u(args[-3]);

    ret = sendfile(outfd, infd, offset, count);

    vmReturn(vm, i2v(ret));
}

void vmBncSocket(VM *vm, const Value *args, u32 nargs)
{
    int fd, domain, type, protocol;

    vmAssert(vm, nargs == 3, "BncSocket requires 3 arguments");

    domain = v2i(args[0]);
    type = v2i(args[-1]);
    protocol = v2i(args[-2]);

    fd = socket(domain, type, protocol);
    vmReturn(vm, i2v(fd));
}

void vmBncConnect(VM *vm, const Value *args, u32 nargs)
{
    int fd;
    struct sockaddr *addr;
    socklen_t addrlen;

    vmAssert(vm, nargs == 3, "BncConnect requires 3 arguments");

    fd = v2i(args[0]);
    addr = (void *)v2p(args[-1]);
    addrlen = v2u(args[-2]);

    fd = connect(fd, addr, addrlen);

    vmReturn(vm, i2v(fd));
}

void vmBncAccept(VM *vm, const Value *args, u32 nargs)
{
    int fd;
    struct sockaddr *addr;
    socklen_t *addrlen;

    vmAssert(vm, nargs == 3, "BncConnect requires 3 arguments");

    fd = v2i(args[0]);
    addr = (void *)v2p(args[-1]);
    addrlen = (void *)v2u(args[-2]);

    fd = accept(fd, addr, addrlen);

    vmReturn(vm, i2v(fd));
}

void vmBncSendto(VM *vm, const Value *args, u32 nargs)
{
    ssize_t ret;
    int sock;
    void *message;
    size_t length;
    int flags;
    struct sockaddr *addr;
    socklen_t addrlen;


    vmAssert(vm, nargs == 6, "BncSendto requires 6 arguments");

    sock = v2i(args[0]);
    message = (void *) v2p(args[-1]);
    length = v2u(args[-2]);
    flags = v2i(args[-3]);
    addr = (void *) v2p(args[-4]);
    addrlen = v2u(args[-5]);

    ret = sendto(sock, message, length, flags, addr, addrlen);

    vmReturn(vm, i2v(ret));
}

void vmBncRecvfrom(VM *vm, const Value *args, u32 nargs)
{
    ssize_t ret;
    int sock;
    void *message;
    size_t length;
    int flags;
    struct sockaddr *addr;
    socklen_t *addrlen;


    vmAssert(vm, nargs == 6, "BncRecvfrom requires 6 arguments");

    sock = v2i(args[0]);
    message = (void *) v2p(args[-1]);
    length = v2u(args[-2]);
    flags = v2i(args[-3]);
    addr = (void *) v2p(args[-4]);
    addrlen = (void *)v2u(args[-5]);

    ret = recvfrom(sock, message, length, flags, addr, addrlen);

    vmReturn(vm, i2v(ret));
}

void vmBncShutdown(VM *vm, const Value *args, u32 nargs)
{
    int fd, how;

    vmAssert(vm, nargs == 2, "BncShutdown requires 2 arguments");

    fd = v2i(args[0]);
    how = v2i(args[-1]);

    fd = shutdown(fd, how);

    vmReturn(vm, i2v(fd));
}

void vmBncBind(VM *vm, const Value *args, u32 nargs)
{
    int fd;
    struct sockaddr *addr;
    socklen_t addrlen;

    vmAssert(vm, nargs == 3, "BncBind requires 3 arguments");

    fd = v2i(args[0]);
    addr = (void *)v2p(args[-1]);
    addrlen = v2u(args[-2]);

    fd = bind(fd, addr, addrlen);

    vmReturn(vm, i2v(fd));
}

void vmBncListen(VM *vm, const Value *args, u32 nargs)
{
    int fd, backlog;

    vmAssert(vm, nargs == 2, "BncListen requires 2 arguments");

    fd = v2i(args[0]);
    backlog = v2i(args[-1]);

    fd = listen(fd, backlog);

    vmReturn(vm, i2v(fd));
}

void vmBncGetsockname(VM *vm, const Value *args, u32 nargs)
{
    int fd;
    struct sockaddr *addr;
    socklen_t *addrlen;

    vmAssert(vm, nargs == 3, "BncGetsockname requires 3 arguments");

    fd = v2i(args[0]);
    addr = (void *)v2p(args[-1]);
    addrlen = (void *)v2p(args[-2]);

    fd = getsockname(fd, addr, addrlen);

    vmReturn(vm, i2v(fd));
}

void vmBncGetpeername(VM *vm, const Value *args, u32 nargs)
{
    int fd;
    struct sockaddr *addr;
    socklen_t *addrlen;

    vmAssert(vm, nargs == 3, "BncGetpeername requires 3 arguments");

    fd = v2i(args[0]);
    addr = (void *)v2p(args[-1]);
    addrlen = (void *)v2p(args[-2]);

    fd = getpeername(fd, addr, addrlen);

    vmReturn(vm, i2v(fd));
}

void vmBncFcntl(VM *vm, const Value *args, u32 nargs)
{
    int fd, cmd;

    vmAssert(vm, nargs == 2 || nargs == 3, "BncFcntl requires 2 or 3 arguments");

    fd = v2i(args[0]);
    cmd = v2i(args[-1]);
    if (nargs == 2) {
        fd = fcntl(fd, cmd, v2i(args[-2]));
    }
    else
        fd = fcntl(fd, cmd);

    vmReturn(vm, i2v(fd));
}
