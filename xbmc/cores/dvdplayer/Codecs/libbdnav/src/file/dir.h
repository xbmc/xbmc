
#ifndef DIR_H_
#define DIR_H_

#include <stdint.h>

#define dir_open dir_open_posix

#define dir_close(X) X->close(X)
#define dir_read(X,Y) X->read(X,Y)

// Our dirent struct only contains the parts we care about.
typedef struct
{
    char    d_name[256];
} DIRENT;

typedef struct dir DIR_H;
struct dir
{
    void* internal;
    void (*close)(DIR_H *dir);
    int (*read)(DIR_H *dir, DIRENT *entry);
};

extern DIR_H *dir_open_posix(const char* dirname);

#endif /* DIR_H_ */
