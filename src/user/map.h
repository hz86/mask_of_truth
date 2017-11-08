#ifndef MAP_H
#define MAP_H

#include <taihen.h>

#ifdef __cplusplus
extern "C" {
#endif

int mapInit();
int mapRoot(SceUID *fd, void **param);
int mapGet(SceUID fd, void **param);
int mapSet(SceUID fd, void *param);
int mapDel(SceUID fd);
int mapUninit();

#ifdef __cplusplus
}
#endif

#endif
