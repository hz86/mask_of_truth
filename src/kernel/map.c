#include "map.h"

#include <psp2kern/kernel/sysmem.h> 
#include <psp2kern/kernel/threadmgr.h> 

#include <string.h>
#include <rbtree.h>

typedef struct MapEntry {
    RBTREE_ENTRY entry;
    SceUID pid;
    SceUID fd;
    void *par;
} MapEntry;

static int g_ref = 0;
static SceUID g_heap = -1;
static SceUID g_lock = -1;
static RBTREE_HEADER rb_head;

static int rb_compare(RBTREE_ENTRY *a, RBTREE_ENTRY *b)
{
    MapEntry *a1 = (MapEntry *)a;
    MapEntry *b1 = (MapEntry *)b;

    if (a1->pid > b1->pid)
    {
        if(a1->fd > b1->fd) {
            return 3;
        }
        else if(a1->fd < b1->fd) {
            return 2;
        }
        return 1;
    }
    else if (a1->pid < b1->pid)
    {
        if(a1->fd < b1->fd) {
            return -3;
        }
        if(a1->fd > b1->fd) {
            return -2;
        }
        return -1;
    }
    else 
    {
        if(a1->fd > b1->fd) {
            return 1;
        }
        if(a1->fd < b1->fd) {
            return -1;
        }
        return 0;
    }
}

int mapInit()
{
    int res = 0;
    if(1 == ++g_ref)
    {
        SceKernelHeapCreateOpt opt;
        memset(&opt, 0, sizeof(opt));

        opt.size = sizeof(opt);
        opt.uselock = 1;

        rbtree_init(&rb_head, rb_compare);
        g_heap = ksceKernelCreateHeap("map", 1024, &opt);
        if(g_heap < 0)
        {
            --g_ref;
            res = -1;
        }
        else
        {
            g_lock = ksceKernelCreateMutex("map", SCE_KERNEL_MUTEX_ATTR_RECURSIVE, 0, NULL);
            if(g_lock < 0)
            {
                --g_ref;
                ksceKernelDeleteHeap(g_heap);
                g_heap = -1;
                res = -1;
            }
        }
    }

    return res;
}

int mapRootForPid(SceUID pid, SceUID *fd, void **param)
{
    int res = -1;
    ksceKernelLockMutex(g_lock, 1, NULL);

    MapEntry *entry = (MapEntry *)rbtree_min(&rb_head);
    while(entry)
    {
        if(entry->pid == pid)
        {
            *fd = entry->fd;
            *param = entry->par;
            res = 0;
        }

        entry = (MapEntry *)rbtree_next(&rb_head, (RBTREE_ENTRY *)entry);
    }

    ksceKernelUnlockMutex(g_lock, 1);
    return res;
}

int mapRoot(SceUID *pid, SceUID *fd, void **param)
{
    int res = -1;
    ksceKernelLockMutex(g_lock, 1, NULL);

    MapEntry *entry = (MapEntry *)rbtree_root(&rb_head);
    if(entry)
    {
        *fd = entry->fd;
        *pid = entry->pid;
        *param = entry->par;
        res = 0;
    }

    ksceKernelUnlockMutex(g_lock, 1);
    return res;
}

int mapSet(SceUID pid, SceUID fd, void *param)
{
    int res = -1;
    ksceKernelLockMutex(g_lock, 1, NULL);

    MapEntry find; find.pid = pid; find.fd = fd;
    MapEntry *entry = (MapEntry *)rbtree_find(&rb_head, (RBTREE_ENTRY *)&find);
    if(NULL == entry)
    {
        MapEntry *entry = (MapEntry *)ksceKernelAllocHeapMemory(g_heap, sizeof(MapEntry));

        entry->par = param;
        entry->pid = pid;
        entry->fd = fd;

        rbtree_insert(&rb_head, (RBTREE_ENTRY *)entry);
        res = 0;
    }

    ksceKernelUnlockMutex(g_lock, 1);
    return res;
}

int mapDel(SceUID pid, SceUID fd)
{
    int res = -1;
    ksceKernelLockMutex(g_lock, 1, NULL);

    MapEntry find; find.pid = pid; find.fd = fd;
    MapEntry *entry = (MapEntry *)rbtree_find(&rb_head, (RBTREE_ENTRY *)&find);
    if(entry)
    {
        rbtree_remove(&rb_head, (RBTREE_ENTRY *)entry);
        ksceKernelFreeHeapMemory(g_heap, entry);
        res = 0;
    }

    ksceKernelUnlockMutex(g_lock, 1);
    return res;
}

int mapUninit()
{
    if(0 == --g_ref)
    {
        for(;;)
        {
            RBTREE_ENTRY *entry = rbtree_root(&rb_head);
            if(entry)
            {
                rbtree_remove(&rb_head, entry);
                ksceKernelFreeHeapMemory(g_heap, entry);
            }
            else
            {
                break;
            }
        }

        if(g_lock >= 0) {
            ksceKernelDeleteMutex(g_lock);
            g_lock = -1;
        }

        if(g_heap >= 0) {
            ksceKernelDeleteHeap(g_heap);
            g_heap = -1;
        }
    }

    return 0;
}
