#include <string.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <unistd.h>
#include "fio.h"
#include "filesystem.h"
#include "romfs.h"
#include "osdebug.h"
#include "hash-djb2.h"

struct romfs_fds_t {
    const uint8_t * file;
    uint32_t cursor;
};

static struct romfs_fds_t romfs_fds[MAX_FDS];

static uint32_t get_unaligned(const uint8_t * d) {
    return ((uint32_t) d[0]) | ((uint32_t) (d[1] << 8)) | ((uint32_t) (d[2] << 16)) | ((uint32_t) (d[3] << 24));
}


void romfs_list(void * opaque, char * path, char * buf){
	const uint8_t * meta = opaque;
	int count = 0;
	int dest = -1;
	if(strlen(path) == 0 || (strlen(path) == 1 && path[0] == '/'))	{
		dest = 1;
		count = 0;
	}

		while( 1 )
		{
			if(get_unaligned(meta) == 0)
			{
				count++;
				
				
				if(dest >= 0)
				{
					strcat(buf,"(");
					strncat(buf,meta+12, get_unaligned(meta+8));
					strcat(buf,")\t");
				}
				else
				{
					path++;
					char *tmp = path;
					while(path[0] != '\0' && path[0] != '/')	path++;
					char name[32];
					if(get_unaligned(meta+4) == hash_djb2(tmp,path-tmp))
					{
						dest = 1;
						count = 0;
					}
				}
				meta += 12+get_unaligned(meta+8);
			}
			else if(get_unaligned(meta) == 1)
			{

				if(!get_unaligned(meta+8)) break;
				if(dest > 0 && count == 0){
					strncat(buf,meta+16, get_unaligned(meta+12));
					strcat(buf,"\t");
				}
				meta += get_unaligned(meta+12) + get_unaligned(meta+8) + 16;
			}
			else if(get_unaligned(meta) == 2){
				meta += 4;
				count--;
				if(count < 0)	break;
			}
		}

}



static ssize_t romfs_read(void * opaque, void * buf, size_t count) {
    struct romfs_fds_t * f = (struct romfs_fds_t *) opaque;
    const uint8_t * size_p = f->file - 4;
    uint32_t size = get_unaligned(size_p);
    
    if ((f->cursor + count) > size)
        count = size - f->cursor;

    memcpy(buf, f->file + f->cursor, count);
    f->cursor += count;

    return count;
}

static off_t romfs_seek(void * opaque, off_t offset, int whence) {
    struct romfs_fds_t * f = (struct romfs_fds_t *) opaque;
    const uint8_t * size_p = f->file - 4;
    uint32_t size = get_unaligned(size_p);
    uint32_t origin;
    
    switch (whence) {
    case SEEK_SET:
        origin = 0;
        break;
    case SEEK_CUR:
        origin = f->cursor;
        break;
    case SEEK_END:
        origin = size;
        break;
    default:
        return -1;
    }

    offset = origin + offset;

    if (offset < 0)
        return -1;
    if (offset > size)
        offset = size;

    f->cursor = offset;

    return offset;
}

const uint8_t * romfs_get_file_by_hash(const uint8_t * romfs, uint32_t h, uint32_t * len) {
    const uint8_t * meta;

    for (meta = romfs; get_unaligned(meta) && get_unaligned(meta + 4); meta += get_unaligned(meta + 4) + 8) {
        if (get_unaligned(meta) == h) {
            if (len) {
                *len = get_unaligned(meta + 4);
            }
            return meta + 8;
        }
    }

    return NULL;
}

static int romfs_open(void * opaque, const char * path, int flags, int mode) {
    uint32_t h = hash_djb2((const uint8_t *) path, -1);
    const uint8_t * romfs = (const uint8_t *) opaque;
    const uint8_t * file;
    int r = -1;

    file = romfs_get_file_by_hash(romfs, h, NULL);

    if (file) {
        r = fio_open(romfs_read, NULL, romfs_seek, NULL, NULL);
        if (r > 0) {
            romfs_fds[r].file = file;
            romfs_fds[r].cursor = 0;
            fio_set_opaque(r, romfs_fds + r);
        }
    }
    return r;
}

void register_romfs(const char * mountpoint, const uint8_t * romfs) {
//    DBGOUT("Registering romfs `%s' @ %p\r\n", mountpoint, romfs);
    register_fs(mountpoint, romfs_open, romfs_list, (void *) romfs);
}
