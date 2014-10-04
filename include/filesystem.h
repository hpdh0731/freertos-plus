#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include <stdint.h>
#include <hash-djb2.h>

#define MAX_FS 16
#define OPENFAIL (-1)

typedef int (*fs_open_t)(void * opaque, const char * fname, int flags, int mode);
typedef void (*fs_list_t)(void * opaque, char * path, char * buf);

/* Need to be called before using any other fs functions */
__attribute__((constructor)) void fs_init();

int register_fs(const char * mountpoint, fs_open_t callback, fs_list_t callback_list, void * opaque);
int fs_open(const char * path, int flags, int mode);
void fs_list(char *, char *);

#endif
