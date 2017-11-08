#include "map.h"

#include <psp2/kernel/sysmem.h> 
#include <psp2/kernel/threadmgr.h> 

#include <stdlib.h>
#include <string.h>
#include <rbtree.h>

typedef struct MapEntry {
    RBTREE_ENTRY entry;
    SceUID fd;
    void *par;
} MapEntry;

static int g_ref = 0;
static SceUID g_lock = -1;
static RBTREE_HEADER rb_head;

static int rb_compare(RBTREE_ENTRY *a, RBTREE_ENTRY *b)
{
    MapEntry *a1 = (MapEntry *)a;
    MapEntry *b1 = (MapEntry *)b;

    if(a1->fd > b1->fd) {
        return 1;
    }
    else if(a1->fd < b1->fd) {
        return -1;
    }

    return 0;
}

int mapInit()
{
    int res = 0;
    if(1 == ++g_ref)
    {
        rbtree_init(&rb_head, rb_compare);
        g_lock = sceKernelCreateMutex("map", 2, 0, NULL);
        if(g_lock < 0)
        {
            --g_ref;
            res = -1;
        }
    }

    return res;
}

int mapRoot(SceUID *fd, void **param)
{
    int res = -1;
    sceKernelLockMutex(g_lock, 1, NULL);

    MapEntry *entry = (MapEntry *)rbtree_root(&rb_head);
    if(entry)
    {
        *fd = entry->fd;
        *param = entry->par;
        res = 0;
    }

    sceKernelUnlockMutex(g_lock, 1);
    return res;
}

int mapGet(SceUID fd, void **param)
{
    int res = -1;
    sceKernelLockMutex(g_lock, 1, NULL);

    MapEntry find; find.fd = fd;
    MapEntry *entry = (MapEntry *)rbtree_find(&rb_head, (RBTREE_ENTRY *)&find);
    if(entry)
    {
        *param = entry->par;
        res = 0;
    }

    sceKernelUnlockMutex(g_lock, 1);
    return res;
}

int mapSet(SceUID fd, void *param)
{
    int res = -1;
    sceKernelLockMutex(g_lock, 1, NULL);

    MapEntry find; find.fd = fd;
    MapEntry *entry = (MapEntry *)rbtree_find(&rb_head, (RBTREE_ENTRY *)&find);
    if(NULL == entry)
    {
        MapEntry *entry = (MapEntry *)malloc(sizeof(MapEntry));

        entry->par = param;
        entry->fd = fd;

        rbtree_insert(&rb_head, (RBTREE_ENTRY *)entry);
        res = 0;
    }

    sceKernelUnlockMutex(g_lock, 1);
    return res;
}

int mapDel(SceUID fd)
{
    int res = -1;
    sceKernelLockMutex(g_lock, 1, NULL);

    MapEntry find; find.fd = fd;
    MapEntry *entry = (MapEntry *)rbtree_find(&rb_head, (RBTREE_ENTRY *)&find);
    if(entry)
    {
        rbtree_remove(&rb_head, (RBTREE_ENTRY *)entry);
        free(entry);
        res = 0;
    }

    sceKernelUnlockMutex(g_lock, 1);
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
                free(entry);
            }
            else
            {
                break;
            }
        }

        if(g_lock >= 0) {
            sceKernelDeleteMutex(g_lock);
            g_lock = -1;
        }
    }

    return 0;
}
