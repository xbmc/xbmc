
#ifndef FILE_H_
#define FILE_H_

#include <stdint.h>
#include <unistd.h>

//#ifdef __LINUX__
#define file_open file_open_linux
#define _FILE_OFFSET_BITS 64
#define __USE_LARGEFILE64
#define DIR_SEP "/"
//#endif

#define file_close(X) X->close(X)
#define file_seek(X,Y,Z) X->seek(X,Y,Z)
#define file_tell(X) X->tell(X)
#define file_eof(X) X->eof(X)
#define file_read(X,Y,Z) X->read(X,Y,Z)
#define file_write(X,Y,Z) X->write(X,Y,Z)

typedef struct file FILE_H;
struct file
{
    void* internal;
    void (*close)(FILE_H *file);
    int64_t (*seek)(FILE_H *file, int64_t offset, int32_t origin);
    int64_t (*tell)(FILE_H *file);
    int (*eof)(FILE_H *file);
    int (*read)(FILE_H *file, uint8_t *buf, int64_t size);
    int (*write)(FILE_H *file, uint8_t *buf, int64_t size);
};

extern FILE_H *file_open_linux(const char* filename, const char *mode);

#endif /* FILE_H_ */
