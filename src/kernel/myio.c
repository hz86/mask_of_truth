#include "myio.h"

#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/threadmgr.h> 
#include "map.h"

#define MAX_BUF_LEN 2048
#define MAX_NAME_LEN 256

SceUID myIoOpen(const char *file, int flags, SceMode mode)
{
    SceUID res;
    SceUID pid;
    uint32_t state;
    char k_file[MAX_NAME_LEN];
    ENTER_SYSCALL(state);

    ksceKernelStrncpyUserToKernel(k_file, (uintptr_t)file, MAX_NAME_LEN);
    res = ksceIoOpen(k_file, flags, mode);
    if(res >= 0)
    {
        pid = ksceKernelGetProcessId();
        mapSet(pid, res, NULL);
    }

    EXIT_SYSCALL(state);
    return res;
}

void myIoLseekPtr(SceUID fd, int whence, SceOff *offset)
{
    uint32_t state;
    SceOff k_offset;
    ENTER_SYSCALL(state);
    ksceKernelMemcpyUserToKernel(&k_offset, (uintptr_t)offset, sizeof(SceOff));

    k_offset = ksceIoLseek(fd, k_offset, whence);

    ksceKernelMemcpyKernelToUser((uintptr_t)offset, &k_offset, sizeof(SceOff));
    EXIT_SYSCALL(state);
}

int myIoRead(SceUID fd, void *data, SceSize size)
{
    int res;
    uint32_t state;
    unsigned char k_data[MAX_BUF_LEN];
    size = size > MAX_BUF_LEN ? MAX_BUF_LEN : size;

    ENTER_SYSCALL(state);
    res = ksceIoRead(fd, k_data, size);
    if(res > 0) ksceKernelMemcpyKernelToUser((uintptr_t)data, k_data, res);
    EXIT_SYSCALL(state);

    return res;
}

int myIoWrite(SceUID fd, const void *data, SceSize size)
{
    int res;
    uint32_t state;
    unsigned char k_data[MAX_BUF_LEN];
    size = size > MAX_BUF_LEN ? MAX_BUF_LEN : size;

    ENTER_SYSCALL(state);
    ksceKernelMemcpyUserToKernel(k_data, (uintptr_t)data, size);
    res = ksceIoWrite(fd, k_data, size);
    EXIT_SYSCALL(state);

    return res;
}

int myIoGetstatByFd(SceUID fd, SceIoStat *stat)
{
    int res;
    uint32_t state;
    SceIoStat k_stat;
    ENTER_SYSCALL(state);

    res = ksceIoGetstatByFd(fd, &k_stat);
    if(0 == res)
    {
        ksceKernelMemcpyKernelToUser((uintptr_t)stat, &k_stat, sizeof(k_stat));
    }

    EXIT_SYSCALL(state);
    return res;
}

int myIoClose(SceUID fd)
{
    int res;
    SceUID pid;
    uint32_t state;
    ENTER_SYSCALL(state);

    res = ksceIoClose(fd);
    if(0 == res)
    {
        pid = ksceKernelGetProcessId();
        mapDel(pid, res);
    }

    EXIT_SYSCALL(state);
    return res;
}
