#include "osdebug.h"
#include "filesystem.h"
#include "fio.h"

#include <stdint.h>
#include <string.h>
#include <hash-djb2.h>

#define MAX_FS 16

struct fs_t {
    uint32_t hash;
    fs_open_t cb;
	fs_list_t cb_list;
    void * opaque;
	const char * name;
};

static struct fs_t fss[MAX_FS];

__attribute__((constructor)) void fs_init() {
    memset(fss, 0, sizeof(fss));
}


void fs_list(char * buf, char * path)
{

	buf[0] = '\0';
	int i;
	if(strlen(path) == 0 || (path[0] == '/' && strlen(path) == 1))
	{
		for (i = 0; i < MAX_FS; i++) {
		    if (!fss[i].cb) break;
			strcat((char *)buf,fss[i].name);
			strcat((char *)buf," ");
    	}
	}
	else
	{
		while (path[0] == '/')	path++;
		char * slash = path;
		while (slash[0] != '/' && slash[0] != '\0')
			slash++;
		uint32_t hash = hash_djb2((const uint8_t *) path, slash - path);

		for (i = 0; i < MAX_FS; i++) {
			if(hash == fss[i].hash)	fss[i].cb_list(fss[i].opaque,slash,buf);
		}
	}
}


int register_fs(const char * mountpoint, fs_open_t callback, fs_list_t callback_list, void * opaque) {
    int i;
    //DBGOUT("register_fs(\"%s\", %p, %p)\r\n", mountpoint, callback, opaque);
    
    for (i = 0; i < MAX_FS; i++) {
        if (!fss[i].cb) {
            fss[i].hash = hash_djb2((const uint8_t *) mountpoint, -1);
            fss[i].cb = callback;
			fss[i].cb_list = callback_list;
            fss[i].opaque = opaque;
			fss[i].name = mountpoint;
            return 0;
        }
    }
    
    return -1;
}

int fs_open(const char * path, int flags, int mode) {
    const char * slash;
    uint32_t hash;
    int i;
//    DBGOUT("fs_open(\"%s\", %i, %i)\r\n", path, flags, mode);
    
    while (path[0] == '/')
        path++;
    
    slash = strchr(path, '/');
    
    if (!slash)
        return -2;

    hash = hash_djb2((const uint8_t *) path, slash - path);
    path = slash + 1;

    for (i = 0; i < MAX_FS; i++) {
        if (fss[i].hash == hash)
            return fss[i].cb(fss[i].opaque, path, flags, mode);
    }
    
    return -2;
}
