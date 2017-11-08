#ifndef MYIO_H
#define MYIO_H

#include <taihen.h>
#include <psp2/io/fcntl.h> 
#include <psp2/io/stat.h> 

#ifdef __cplusplus
extern "C" {
#endif

SceUID myIoOpen(const char *file, int flags, SceMode mode);
void myIoLseekPtr(SceUID fd, int whence, SceOff *offset);
int myIoRead(SceUID fd, void *data, SceSize size);
int myIoWrite(SceUID fd, const void *data, SceSize size);
int myIoGetstatByFd(SceUID fd, SceIoStat *stat);
int myIoClose(SceUID fd);

#ifdef __cplusplus
}
#endif

#endif
