#include <taihen.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/io/fcntl.h> 
#include <string.h>
#include "map.h"

#define TITLEID "PCSG00838"
#define ROOTPATH "utawarerumono"

static SceUID hooks[16];
static int n_hooks = 0;

static tai_hook_ref_t ksceKernelStartPreloadedModulesRef;
static int ksceKernelStartPreloadedModulesPatched(SceUID pid)
{
    int res = TAI_CONTINUE(int, ksceKernelStartPreloadedModulesRef, pid);

    char titleid[32];
    ksceKernelGetProcessTitleId(pid, titleid, sizeof(titleid));
    if(0 == strcmp(titleid, TITLEID))
    {
        ksceKernelLoadStartModuleForPid(pid, "ux0:" ROOTPATH "/" TITLEID "/eboot.suprx", 0, NULL, 0, NULL, NULL);
    }

    return res;
}

static tai_hook_ref_t ksceModulemgrUnloadProcessRef;
static int ksceModulemgrUnloadProcessPatched(SceUID pid)
{
    char titleid[32];
    ksceKernelGetProcessTitleId(pid, titleid, sizeof(titleid));
    if(0 == strcmp(titleid, TITLEID))
    {
        //必须释放
        //否则可能导致 PSVITA 重启
        for(;;)
        {
            SceUID fd;
            void *param;
            if(0 == mapRootForPid(pid, &fd, &param))
            {
                ksceIoClose(fd);
                mapDel(pid, fd);
            }
            else
            {
                break;
            }
        }
    }

    return TAI_CONTINUE(int, ksceModulemgrUnloadProcessRef, pid);
}

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp)
{
    mapInit();
    hooks[n_hooks++] = taiHookFunctionExportForKernel(KERNEL_PID, &ksceKernelStartPreloadedModulesRef, "SceKernelModulemgr", 0xC445FA63, 0x432DCC7A, ksceKernelStartPreloadedModulesPatched);
    hooks[n_hooks++] = taiHookFunctionImportForKernel(KERNEL_PID, &ksceModulemgrUnloadProcessRef, "SceProcessmgr", 0xC445FA63, 0x0E33258E, ksceModulemgrUnloadProcessPatched);
    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp)
{
    taiHookReleaseForKernel(hooks[--n_hooks], ksceModulemgrUnloadProcessRef);
    taiHookReleaseForKernel(hooks[--n_hooks], ksceKernelStartPreloadedModulesRef);

    for(;;)
    {
        void *param;
        SceUID pid, fd;
        if(0 == mapRoot(&pid, &fd, &param))
        {
            ksceIoClose(fd);
            mapDel(pid, fd);
        }
        else
        {
            break;
        }
    }

    mapUninit();
    return SCE_KERNEL_STOP_SUCCESS;
}
