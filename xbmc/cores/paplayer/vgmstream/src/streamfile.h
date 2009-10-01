/*
* streamfile.h - definitions for buffered file reading with STREAMFILE
*/

#ifndef _STREAMFILE_H
#define _STREAMFILE_H

#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "streamtypes.h"
#include "util.h"

#if defined(__MSVCRT__) || defined(_MSC_VER) || defined(XBMC)
#define fseeko fseek
#define ftello ftell
#endif
#if defined(__MSVCRT__) || defined(_MSC_VER) 
#include <io.h>
#define dup _dup
#ifdef fileno
#undef fileno
#endif
#define fileno _fileno
#define fdopen _fdopen
#endif

#define STREAMFILE_DEFAULT_BUFFER_SIZE 0x400

typedef struct _STREAMFILE {
    size_t (*read)(struct _STREAMFILE *,uint8_t * dest, off_t offset, size_t length);
    size_t (*get_size)(struct _STREAMFILE *);
    off_t (*get_offset)(struct _STREAMFILE *);    
    // for dual-file support
    void (*get_name)(struct _STREAMFILE *,char *name,size_t length);
    // for when the "name" is encoded specially, this is the actual user
    // visible name
    void (*get_realname)(struct _STREAMFILE *,char *name,size_t length);
    struct _STREAMFILE * (*open)(struct _STREAMFILE *,const char * const filename,size_t buffersize);

    void (*close)(struct _STREAMFILE *);
#ifdef PROFILE_STREAMFILE
    size_t (*get_bytes_read)(struct _STREAMFILE *);
    int (*get_error_count)(struct _STREAMFILE *);

#endif
} STREAMFILE;

/* close a file, destroy the STREAMFILE object */
static inline void close_streamfile(STREAMFILE * streamfile) {
    streamfile->close(streamfile);
}

/* read from a file
*
* returns number of bytes read
*/
static inline size_t read_streamfile(uint8_t * dest, off_t offset, size_t length, STREAMFILE * streamfile) {
    return streamfile->read(streamfile,dest,offset,length);
}

/* return file size */
static inline size_t get_streamfile_size(STREAMFILE * streamfile) {
    return streamfile->get_size(streamfile);
}

#ifdef PROFILE_STREAMFILE
/* return how many bytes we read into buffers */
static inline size_t get_streamfile_bytes_read(STREAMFILE * streamfile) {
    if (streamfile->get_bytes_read)
        return streamfile->get_bytes_read(streamfile);
    else
        return 0;
}

/* return how many times we encountered a read error */
static inline int get_streamfile_error_count(STREAMFILE * streamfile) {
    if (streamfile->get_error_count)
        return streamfile->get_error_count(streamfile);
    else
        return 0;
}
#endif

/* Sometimes you just need an int, and we're doing the buffering.
* Note, however, that if these fail to read they'll return -1,
* so that should not be a valid value or there should be some backup. */
static inline int16_t read_16bitLE(off_t offset, STREAMFILE * streamfile) {
    uint8_t buf[2];

    if (read_streamfile(buf,offset,2,streamfile)!=2) return -1;
    return get_16bitLE(buf);
}
static inline int16_t read_16bitBE(off_t offset, STREAMFILE * streamfile) {
    uint8_t buf[2];

    if (read_streamfile(buf,offset,2,streamfile)!=2) return -1;
    return get_16bitBE(buf);
}
static inline int32_t read_32bitLE(off_t offset, STREAMFILE * streamfile) {
    uint8_t buf[4];

    if (read_streamfile(buf,offset,4,streamfile)!=4) return -1;
    return get_32bitLE(buf);
}
static inline int32_t read_32bitBE(off_t offset, STREAMFILE * streamfile) {
    uint8_t buf[4];

    if (read_streamfile(buf,offset,4,streamfile)!=4) return -1;
    return get_32bitBE(buf);
}

static inline int8_t read_8bit(off_t offset, STREAMFILE * streamfile) {
    uint8_t buf[1];

    if (read_streamfile(buf,offset,1,streamfile)!=1) return -1;
    return buf[0];
}

/* open file with a set buffer size, create a STREAMFILE object
*
* Returns pointer to new STREAMFILE or NULL if open failed
*/
STREAMFILE * open_stdio_streamfile_buffer(const char * const filename, size_t buffersize);

/* open file with a default buffer size, create a STREAMFILE object
*
* Returns pointer to new STREAMFILE or NULL if open failed
*/
static inline STREAMFILE * open_stdio_streamfile(const char * const filename) {
    return open_stdio_streamfile_buffer(filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
}

size_t get_streamfile_dos_line(int dst_length, char * dst, off_t offset,
                STREAMFILE * infile, int *line_done_ptr);

#endif
