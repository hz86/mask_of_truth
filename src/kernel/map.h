#ifndef MAP_H
#define MAP_H

#include <taihen.h>

#ifdef __cplusplus
extern "C" {
#endif

int mapInit();
int mapRootForPid(SceUID pid, SceUID *fd, void **param);
int mapRoot(SceUID *pid, SceUID *fd, void **param);
int mapSet(SceUID pid, SceUID fd, void *param);
int mapDel(SceUID pid, SceUID fd);
int mapUninit();

#ifdef __cplusplus
}
#endif

#endif
