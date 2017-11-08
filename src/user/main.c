#include <taihen.h>

#include <psp2/display.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h> 
#include <psp2/sysmodule.h>
#include <psp2/io/fcntl.h> 
#include <psp2/io/stat.h> 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "map.h"
#include "myio.h"
#include "sha1.h"

#define TITLEID "PCSG00838"
#define ROOTPATH "utawarerumono"
#define CHECKSHA1 "df4fdb8e8c72a43089074f80f1a98e313ad57c9d"

typedef struct PrxFix {
    uint32_t nums;
    SceUID *hooks;
} PrxFix;

typedef struct PrxFixPack {
    uint32_t offset;
    uint32_t oldlen;
    uint32_t newlen;
} PrxFixPack;

static int install = 0;
static PrxFix *prxfix = NULL;
static SceUID hooks[16];
static int n_hooks = 0;

static SceUID _sceIoOpen(const char *file, int flags, SceMode mode)
{
    if(0 == strncmp(file, "app0:", 5))
    {
        char newfile[256];
        strcat(strcpy(newfile, "ux0:/" ROOTPATH "/" TITLEID), file + 5);
        SceUID fd = myIoOpen(newfile, SCE_O_RDONLY, 0777);
        if(fd >= 0)
        {
            mapSet(fd, NULL);
            return fd;
        }
    }

    return -1;
}

static int _sceIoClose(SceUID fd)
{
    void *param;
    if(0 == mapGet(fd, &param))
    {
        int res = myIoClose(fd);
        mapDel(fd);
        return res;
    }

    return -1;
}

static int _sceIoPread(SceUID fd, void *data, SceSize size, SceOff offset)
{
    void *param;
    if(0 == mapGet(fd, &param))
    {
        SceOff pos = offset;
        myIoLseekPtr(fd, SCE_SEEK_SET, &pos);
        if(pos < 0) return pos;

        int len = 0;
        int max = 2048;
        while(len < size)
        {
            int res;
            int rlen = size - len;
            if(rlen > max) rlen = max;
            res = myIoRead(fd, (unsigned char *)data + len, rlen);
            if(res < 0) return res;
            if(0 == res) break;
            len += rlen;
        }

        return len;
    }

    return -1;
}

static int _sceIoLseek(SceUID fd, SceOff offset, int whence)
{
    void *param;
    if(0 == mapGet(fd, &param))
    {
        SceOff pos = offset;
        myIoLseekPtr(fd, whence, &pos);
        return pos;
    }

    return -1;
}

static int _sceIoGetstatByFd(SceUID fd, SceIoStat *stat)
{
    void *param;
    if(0 == mapGet(fd, &param))
    {
        return myIoGetstatByFd(fd, stat);
    }

    return -1;
}

static int _sceIoGetstat(const char *file, SceIoStat *stat)
{
    if(0 == strncmp(file, "app0:", 5))
    {
        char newfile[256];
        strcat(strcpy(newfile, "ux0:/" ROOTPATH "/" TITLEID), file + 5);
        SceUID fd = myIoOpen(newfile, SCE_O_RDONLY, 0777);
        if(fd >= 0)
        {
            int res = myIoGetstatByFd(fd, stat);
            myIoClose(fd);
            return res;
        }
    }

    return -1;
}

static tai_hook_ref_t sceAvPlayerIoOpenRef;
static SceUID sceAvPlayerIoOpenPatched(const char *file, int flags, SceMode mode)
{
    SceUID res = _sceIoOpen(file, flags, mode);
    return -1 == res ? TAI_CONTINUE(SceUID, sceAvPlayerIoOpenRef, file, flags, mode) : res;
}

static tai_hook_ref_t sceAvPlayerIoCloseRef;
static int sceAvPlayerIoClosePatched(SceUID fd)
{
    int res = _sceIoClose(fd);
    return -1 == res ? TAI_CONTINUE(int, sceAvPlayerIoCloseRef, fd) : res;
}

static tai_hook_ref_t sceAvPlayerIoPreadRef;
static int sceAvPlayerIoPreadPatched(SceUID fd, void *data, SceSize size, SceOff offset)
{
    int res = _sceIoPread(fd, data, size, offset);
    return -1 == res ? TAI_CONTINUE(int, sceAvPlayerIoPreadRef, fd, data, size, offset) : res;
}

static tai_hook_ref_t sceAvPlayerIoLseekRef;
static SceOff sceAvPlayerIoLseekPatched(SceUID fd, SceOff offset, int whence)
{
    SceOff res = _sceIoLseek(fd, offset, whence);
    return -1 == res ? TAI_CONTINUE(SceOff, sceAvPlayerIoLseekRef, fd, offset, whence) : res;
}

static tai_hook_ref_t sceIoOpenRef;
static SceUID sceIoOpenPatched(const char *file, int flags, SceMode mode)
{
    SceUID res = _sceIoOpen(file, flags, mode);
    return -1 == res ? TAI_CONTINUE(SceUID, sceIoOpenRef, file, flags, mode) : res;
}

static tai_hook_ref_t sceIoCloseRef;
static int sceIoClosePatched(SceUID fd)
{
    int res = _sceIoClose(fd);
    return -1 == res ? TAI_CONTINUE(int, sceIoCloseRef, fd) : res;
}

static tai_hook_ref_t sceIoPreadRef;
static int sceIoPreadPatched(SceUID fd, void *data, SceSize size, SceOff offset)
{
    int res = _sceIoPread(fd, data, size, offset);
    return -1 == res ? TAI_CONTINUE(int, sceIoPreadRef, fd, data, size, offset) : res;
}

static tai_hook_ref_t sceIoGetstatByFdRef;
static int sceIoGetstatByFdPatched(SceUID fd, SceIoStat *stat)
{
    int res = _sceIoGetstatByFd(fd, stat);
    return -1 == res ? TAI_CONTINUE(int, sceIoGetstatByFdRef, fd, stat) : res;
}

static tai_hook_ref_t sceIoGetstatRef;
static int sceIoGetstatPatched(const char *file, SceIoStat *stat)
{
    int res = _sceIoGetstat(file, stat);
    return -1 == res ? TAI_CONTINUE(int, sceIoGetstatRef, file, stat) : res;
}

static int checkPrxFix(SceUID modid, const char *file, char *sha1)
{
    SceKernelModuleInfo info;
    info.size = sizeof(SceKernelModuleInfo);
    if(sceKernelGetModuleInfo(modid, &info) < 0) {
        return -1;
    }

    int res = -1;
    char *data = (char *)info.segments[0].vaddr;
    unsigned int datalen = info.segments[0].memsz;

    SceUID fd = myIoOpen(file, SCE_O_RDONLY, 0);
    if (fd >= 0)
    {
        SHA1Context context;
        uint8_t digest[SHA1HashSize];
        SHA1Reset(&context);

        uint32_t nums;
        myIoRead(fd, &nums, sizeof(uint32_t));
        for (uint32_t i = 0; i < nums; i++)
        {
            PrxFixPack pack;
            myIoRead(fd, &pack, sizeof(PrxFixPack));

            SceOff pos = pack.newlen;
            myIoLseekPtr(fd, SCE_SEEK_CUR, &pos);

            if(pack.offset < datalen) {
                SHA1Input(&context, (uint8_t *)(data + pack.offset), (unsigned int)pack.oldlen);
            }
        }

        SHA1Result(&context, digest);
        for (int i = 0, j = 0; i < 40; i += 2) {
            sprintf(&sha1[i], "%02x", digest[j++]);
        }

        myIoClose(fd);
        res = 0;
    }

    return res;
}

static PrxFix *prxFix(SceUID modid, const char *file)
{
    PrxFix *fix = NULL;
    SceUID fd = myIoOpen(file, SCE_O_RDONLY, 0);
    if (fd >= 0)
    {
        fix = malloc(sizeof(PrxFix));
        myIoRead(fd, &fix->nums, sizeof(uint32_t));

        fix->hooks = malloc(sizeof(SceUID) * fix->nums);
        for (uint32_t i = 0; i < fix->nums; i++)
        {
            PrxFixPack pack;
            myIoRead(fd, &pack, sizeof(PrxFixPack));
            unsigned char *fixdata = malloc(pack.oldlen);

            memset(fixdata, 0, pack.oldlen);
            myIoRead(fd, fixdata, pack.newlen);

            fix->hooks[i] = taiInjectData(modid, 0, pack.offset, fixdata, pack.oldlen);
            free(fixdata);
        }

        myIoClose(fd);
    }

    return fix;
}

static void prxFixFree(PrxFix *fix)
{
    for (uint32_t i = 0; i < fix->nums; i++) {
        taiInjectRelease(fix->hooks[i]);
    }

    free(fix->hooks);
    free(fix);
}

static tai_hook_ref_t sceSysmoduleLoadModuleRef;
static int sceSysmoduleLoadModulePatched(uint16_t id)
{
    int ret;
    ret = TAI_CONTINUE(int, sceSysmoduleLoadModuleRef, id);
    if (ret >= 0)
    {
        if(SCE_SYSMODULE_AVPLAYER == id)
        {
            hooks[n_hooks++] = taiHookFunctionImport(&sceAvPlayerIoOpenRef, "SceAvPlayer", 0xCAE9ACE6, 0x6C60AC61, sceAvPlayerIoOpenPatched);
            hooks[n_hooks++] = taiHookFunctionImport(&sceAvPlayerIoCloseRef, "SceAvPlayer", 0xF2FF276E, 0xC70B8886, sceAvPlayerIoClosePatched);
            hooks[n_hooks++] = taiHookFunctionImport(&sceAvPlayerIoPreadRef, "SceAvPlayer", 0xCAE9ACE6, 0x52315AD7, sceAvPlayerIoPreadPatched);
            hooks[n_hooks++] = taiHookFunctionImport(&sceAvPlayerIoLseekRef, "SceAvPlayer", 0xCAE9ACE6, 0x99BA173E, sceAvPlayerIoLseekPatched);
        }
    }

    return ret;
}

static tai_hook_ref_t sceSysmoduleUnloadModuleRef;
static int sceSysmoduleUnloadModulePatched(uint16_t id)
{
    int ret;
    ret = TAI_CONTINUE(int, sceSysmoduleUnloadModuleRef, id);
    if (ret >= 0)
    {
        if(SCE_SYSMODULE_AVPLAYER == id)
        {
            taiHookRelease(hooks[--n_hooks], sceAvPlayerIoLseekRef);
            taiHookRelease(hooks[--n_hooks], sceAvPlayerIoPreadRef);
            taiHookRelease(hooks[--n_hooks], sceAvPlayerIoCloseRef);
            taiHookRelease(hooks[--n_hooks], sceAvPlayerIoOpenRef);
        }
    }

    return ret;
}

static void gameVersionError()
{
    void *screen_base;
    SceUID displayblock = sceKernelAllocMemBlock("display", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, 1024*1024*2, NULL);
    sceKernelGetMemBlockBase(displayblock, (void**)&screen_base);

    SceUID fd = myIoOpen("ux0:/" ROOTPATH "/" TITLEID "/version_error.dat", SCE_O_RDONLY, 0777);
    if(fd >= 0)
    {
        int len = 0;
        int max = 2048;
        int size = 960*544*4;

        while(len < size)
        {
            int res;
            int rlen = size - len;
            if(rlen > max) rlen = max;
            res = myIoRead(fd, (char *)screen_base + len, rlen);
            if(res <= 0) break;
            len += rlen;
        }

        SceDisplayFrameBuf framebuf;
        framebuf.size = sizeof(SceDisplayFrameBuf);

        framebuf.pitch = 960;
        framebuf.width = 960;
        framebuf.height = 544;
        framebuf.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8,

        framebuf.base = screen_base;
        sceDisplaySetFrameBuf(&framebuf, SCE_DISPLAY_SETBUF_NEXTFRAME);

        sceKernelDelayThread(5*1000000);
        myIoClose(fd);
    }

    sceKernelFreeMemBlock(displayblock);
}

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp)
{
    tai_module_info_t tai_info = {0};
    tai_info.size = sizeof(tai_module_info_t);
    taiGetModuleInfo(TAI_MAIN_MODULE, &tai_info);

    char sha1[41];
    if(checkPrxFix(tai_info.modid, "ux0:/" ROOTPATH "/" TITLEID "/eboot.bin.fix", sha1) < 0 ||
        0 != strcmp(sha1, CHECKSHA1))
    {
        gameVersionError();
        return SCE_KERNEL_START_SUCCESS;
    }

    mapInit();
    prxfix = prxFix(tai_info.modid, "ux0:/" ROOTPATH "/" TITLEID "/eboot.bin.fix");

    hooks[n_hooks++] = taiHookFunctionImport(&sceIoOpenRef, TAI_MAIN_MODULE, 0xCAE9ACE6, 0x6C60AC61, sceIoOpenPatched);
    hooks[n_hooks++] = taiHookFunctionImport(&sceIoCloseRef, TAI_MAIN_MODULE, 0xF2FF276E, 0xC70B8886, sceIoClosePatched);
    hooks[n_hooks++] = taiHookFunctionImport(&sceIoPreadRef, TAI_MAIN_MODULE, 0xCAE9ACE6, 0x52315AD7, sceIoPreadPatched);
    hooks[n_hooks++] = taiHookFunctionImport(&sceIoGetstatByFdRef, TAI_MAIN_MODULE, 0xCAE9ACE6, 0x57F8CD25, sceIoGetstatByFdPatched);
    hooks[n_hooks++] = taiHookFunctionImport(&sceIoGetstatRef, TAI_MAIN_MODULE, 0xCAE9ACE6, 0xBCA5B623, sceIoGetstatPatched);

    hooks[n_hooks++] = taiHookFunctionImport(&sceSysmoduleLoadModuleRef, TAI_MAIN_MODULE, 0x03FCF19D, 0x79A0160A, sceSysmoduleLoadModulePatched);
    hooks[n_hooks++] = taiHookFunctionImport(&sceSysmoduleUnloadModuleRef, TAI_MAIN_MODULE, 0x03FCF19D, 0x31D87805, sceSysmoduleUnloadModulePatched);

    sceSysmoduleLoadModule(SCE_SYSMODULE_AVPLAYER);
    install = 1;

    return SCE_KERNEL_START_SUCCESS;
}

//游戏关闭时不会执行
int module_stop(SceSize args, void *argp)
{
    if(0 == install) {
        return SCE_KERNEL_STOP_SUCCESS;
    }

    sceSysmoduleUnloadModule(SCE_SYSMODULE_AVPLAYER);

    taiHookRelease(hooks[--n_hooks], sceSysmoduleUnloadModuleRef);
    taiHookRelease(hooks[--n_hooks], sceSysmoduleUnloadModuleRef);

    taiHookRelease(hooks[--n_hooks], sceIoGetstatRef);
    taiHookRelease(hooks[--n_hooks], sceIoGetstatByFdRef);
    taiHookRelease(hooks[--n_hooks], sceIoPreadRef);
    taiHookRelease(hooks[--n_hooks], sceIoCloseRef);
    taiHookRelease(hooks[--n_hooks], sceIoOpenRef);

    for(;;)
    {
        SceUID fd;
        void *param;
        if(0 == mapRoot(&fd, &param))
        {
            myIoClose(fd);
            mapDel(fd);
        }
        else
        {
            break;
        }
    }

    if(prxfix) {
        prxFixFree(prxfix);
        prxfix = NULL;
    }

    mapUninit();
    install = 0;

    return SCE_KERNEL_STOP_SUCCESS;
}
