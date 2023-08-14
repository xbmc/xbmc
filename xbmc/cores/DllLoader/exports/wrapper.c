/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

//
// To recreate wrapper.def:
//
// bash# (echo -n "-Wl"; grep __wrap wrapper.c | grep -v bash | sed "s/.*__wrap_//g" | sed "s/(.*//g" | awk '{printf(",-wrap,%s",$0);}') > wrapper.def
//
#include <sys/types.h>
#include <sys/stat.h>
#if !defined(TARGET_ANDROID)
#include <sys/statvfs.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#include <dlfcn.h>

#if defined(TARGET_DARWIN) || defined(TARGET_FREEBSD) || defined(TARGET_ANDROID)
typedef off_t     __off_t;
typedef int64_t   off64_t;
typedef off64_t   __off64_t;
typedef fpos_t    fpos64_t;
#define stat64    stat
#endif

#ifdef TARGET_POSIX
#define _stat stat
#endif

struct mntent;

void* dllmalloc(size_t );
void* dllcalloc( size_t , size_t );
void* dllrealloc(void*, size_t);
void dllfree(void*);

int dll_open(const char* szFileName, int iMode);
int dll_write(int fd, const void* buffer, unsigned int uiSize);
int dll_read(int fd, void* buffer, unsigned int uiSize);
off_t dll_lseek(int fd, __off_t lPos, int iWhence);
__off64_t dll_lseeki64(int fd, __off64_t lPos, int iWhence);
int dll_close(int fd);

FILE * dll_fopen (const char * filename, const char * mode);
FILE* dll_freopen(const char *path, const char *mode, FILE *stream);
FILE* dll_fdopen(int i, const char* file);
int dll_fclose (FILE * stream);
int dll_ferror (FILE * stream);
int dll_feof (FILE * stream);
int dll_fileno(FILE* stream);
void dll_clearerr(FILE* stream);
int dll_fread (void * buffer, size_t size, size_t count, FILE * stream);
size_t dll_fwrite ( const void * buffer, size_t size, size_t count, FILE * stream );
int dll_fflush (FILE * stream);
int dll_fputc (int character, FILE * stream);
int dll_fputs (const char * szLine , FILE* stream);
int dll_putc(int c, FILE *stream);
int dll_fseek ( FILE * stream , long offset , int origin );
int dll_fseek64(FILE *stream, off64_t offset, int origin);
long dll_ftell(FILE *stream);
off64_t dll_ftell64(FILE *stream);
void dll_rewind(FILE* stream);
int dll_fgetpos(FILE* stream, fpos_t* pos);
int dll_fgetpos64(FILE *stream, fpos64_t *pos);
int dll_fsetpos(FILE* stream, const fpos_t* pos);
int dll_fsetpos64(FILE* stream, const fpos64_t* pos);
DIR* dll_opendir(const char* name);
struct dirent* dll_readdir(DIR* dirp);
int dll_closedir(DIR* dirp);
void dll_rewinddir(DIR* dirp);
int dll_fprintf(FILE* stream , const char * format, ...);
int dllprintf(const char *format, ...);
int dll_vfprintf(FILE *stream, const char *format, va_list va);
int dll_fgetc (FILE* stream);
char * dll_fgets (char* pszString, int num , FILE * stream);
int dll_getc (FILE * stream);
int dll_ungetc (int c, FILE * stream);
int dll_ioctl(int d, unsigned long int request, va_list va);
int dll_stat(const char *path, struct _stat *buffer);
int dll_stat64(const char *path, struct stat64 *buffer);
void dll_flockfile(FILE *file);
int dll_ftrylockfile(FILE *file);
void dll_funlockfile(FILE *file);
int dll_fstat64(int fd, struct stat64 *buf);
int dll_fstat(int fd, struct _stat *buf);
FILE* dll_popen(const char *command, const char *mode);
void* dll_dlopen(const char *filename, int flag);
int dll_setvbuf(FILE *stream, char *buf, int type, size_t size);
struct mntent *dll_getmntent(FILE *fp);
struct mntent* dll_getmntent_r(FILE* fp, struct mntent* result, char* buffer, int bufsize);

void *__wrap_dlopen(const char *filename, int flag)
{
  return dlopen(filename, flag);
}

FILE *__wrap_popen(const char *command, const char *mode)
{
  return dll_popen(command, mode);
}

void* __wrap_calloc( size_t num, size_t size )
{
  return dllcalloc(num, size);
}

void* __wrap_malloc(size_t size)
{
  return dllmalloc(size);
}

void* __wrap_realloc( void *memblock, size_t size )
{
  return dllrealloc(memblock, size);
}

void __wrap_free( void* pPtr )
{
  dllfree(pPtr);
}

int __wrap_open(const char *file, int oflag, ...)
{
  return dll_open(file, oflag);
}

int __wrap_open64(const char *file, int oflag, ...)
{
  return dll_open(file, oflag);
}

int __wrap_close(int fd)
{
   return dll_close(fd);
}

ssize_t __wrap_write(int fd, const void *buf, size_t count)
{
  return dll_write(fd, buf, count);
}

ssize_t __wrap_read(int fd, void *buf, size_t count)
{
  return dll_read(fd, buf, count);
}

__off_t __wrap_lseek(int filedes, __off_t offset, int whence)
{
  return dll_lseek(filedes, offset, whence);
}

__off64_t __wrap_lseek64(int filedes, __off64_t offset, int whence)
{
  __off64_t seekRes = dll_lseeki64(filedes, offset, whence);
  return seekRes;
}

int __wrap_fclose(FILE *fp)
{
  return dll_fclose(fp);
}

int __wrap_ferror(FILE *stream)
{
  return dll_ferror(stream);
}

void __wrap_clearerr(FILE *stream)
{
  dll_clearerr(stream);
}

int __wrap_feof(FILE *stream)
{
  return dll_feof(stream);
}

int __wrap_fileno(FILE *stream)
{
  return dll_fileno(stream);
}

FILE *__wrap_fopen(const char *path, const char *mode)
{
  return dll_fopen(path, mode);
}

FILE *__wrap_fopen64(const char *path, const char *mode)
{
  return dll_fopen(path, mode);
}

FILE *__wrap_fdopen(int filedes, const char *mode)
{
  return dll_fdopen(filedes, mode);
}

FILE *__wrap_freopen(const char *path, const char *mode, FILE *stream)
{
  return dll_freopen(path, mode, stream);
}

size_t __wrap_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  return dll_fread(ptr, size, nmemb, stream);
}

size_t __wrap_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  return dll_fwrite(ptr, size, nmemb, stream);
}

int __wrap_fflush(FILE *stream)
{
  return dll_fflush(stream);
}

int __wrap_fputc(int c, FILE *stream)
{
  return dll_fputc(c, stream);
}

int __wrap_fputs(const char *s, FILE *stream)
{
  return dll_fputs(s, stream);
}

int __wrap__IO_putc(int c, FILE *stream)
{
  return dll_putc(c, stream);
}

int __wrap_fseek(FILE *stream, long offset, int whence)
{
  return dll_fseek(stream, offset, whence);
}

int __wrap_fseeko64(FILE *stream, off64_t offset, int whence)
{
  return dll_fseek64(stream, offset, whence);
}

long __wrap_ftell(FILE *stream)
{
  return dll_ftell(stream);
}

off64_t __wrap_ftello64(FILE *stream)
{
  return dll_ftell64(stream);
}

void __wrap_rewind(FILE *stream)
{
  dll_rewind(stream);
}

int __wrap_fgetpos(FILE *stream, fpos_t *pos)
{
  return dll_fgetpos(stream, pos);
}

int __wrap_fgetpos64(FILE *stream, fpos64_t *pos)
{
  return dll_fgetpos64(stream, pos);
}

int __wrap_fsetpos(FILE *stream, fpos_t *pos)
{
  return dll_fsetpos(stream, pos);
}

int __wrap_fsetpos64(FILE *stream, fpos64_t *pos)
{
  return dll_fsetpos64(stream, pos);
}

DIR * __wrap_opendir(const char *name)
{
  return dll_opendir(name);
}

struct dirent * __wrap_readdir(DIR* dirp)
{
  return dll_readdir(dirp);
}

struct dirent * __wrap_readdir64(DIR* dirp)
{
  return dll_readdir(dirp);
}

int __wrap_closedir(DIR* dirp)
{
  return dll_closedir(dirp);
}

void __wrap_rewinddir(DIR* dirp)
{
  dll_rewinddir(dirp);
}

int __wrap_fprintf(FILE *stream, const char *format, ...)
{
    int res;
    va_list va;
    va_start(va, format);
    res = dll_vfprintf(stream, format, va);
    va_end(va);
    return res;
}

int __wrap_vfprintf(FILE *stream, const char *format, va_list ap)
{
  return dll_vfprintf(stream, format, ap);
}

int __wrap_printf(const char *format, ...)
{
    int res;
    va_list va;
    va_start(va, format);
    res = dll_vfprintf(stdout, format, va);
    va_end(va);
    return res;
}

int __wrap_fgetc(FILE *stream)
{
  return dll_fgetc(stream);
}

char *__wrap_fgets(char *s, int size, FILE *stream)
{
  return dll_fgets(s, size, stream);
}

int __wrap__IO_getc(FILE *stream)
{
  return dll_getc(stream);
}

int __wrap__IO_getc_unlocked(FILE *stream)
{
  return dll_getc(stream);
}

int __wrap_getc_unlocked(FILE *stream)
{
  return dll_getc(stream);
}

int __wrap_ungetc(int c, FILE *stream)
{
  return dll_ungetc(c, stream);
}

int __wrap_getc(FILE *stream)
{
  return dll_getc(stream);
}

int __wrap_ioctl(int d, unsigned long int request, ...)
{
    int res;
    va_list va;
    va_start(va, request);
    res = dll_ioctl(d, request, va);
    va_end(va);
    return res;
}

int __wrap__stat(const char *path, struct _stat *buffer)
{
  return dll_stat(path, buffer);
}

int __wrap_stat(const char *path, struct _stat *buffer)
{
  return dll_stat(path, buffer);
}

int __wrap_stat64(const char* path, struct stat64* buffer)
{
  return dll_stat64(path, buffer);
}

int __wrap___xstat(int __ver, const char *__filename, struct stat *__stat_buf)
{
  return dll_stat(__filename, __stat_buf);
}

int __wrap___xstat64(int __ver, const char *__filename, struct stat64 *__stat_buf)
{
  return dll_stat64(__filename, __stat_buf);
}

int __wrap___lxstat64(int __ver, const char *__filename, struct stat64 *__stat_buf)
{
  return dll_stat64(__filename, __stat_buf);
}

void __wrap_flockfile(FILE *file)
{
    dll_flockfile(file);
}

int __wrap_ftrylockfile(FILE *file)
{
    return dll_ftrylockfile(file);
}

void __wrap_funlockfile(FILE *file)
{
    dll_funlockfile(file);
}

int __wrap___fxstat64(int ver, int fd, struct stat64 *buf)
{
  return dll_fstat64(fd, buf);
}

int __wrap___fxstat(int ver, int fd, struct stat *buf)
{
  return dll_fstat(fd, buf);
}

int __wrap_fstat(int fd, struct _stat *buf)
{
  return dll_fstat(fd, buf);
}

int __wrap_fstat64(int fd, struct stat64* buf)
{
  return dll_fstat64(fd, buf);
}

int __wrap_setvbuf(FILE *stream, char *buf, int type, size_t size)
{
   return dll_setvbuf(stream, buf, type, size);
}

struct mntent *__wrap_getmntent(FILE *fp)
{
#ifdef TARGET_POSIX
  return dll_getmntent(fp);
#endif
  return NULL;
}

struct mntent* __wrap_getmntent_r(FILE* fp, struct mntent* result, char* buffer, int bufsize)
{
#ifdef TARGET_POSIX
  return dll_getmntent_r(fp, result, buffer, bufsize);
#endif
  return NULL;
}

// GCC 4.3 in Ubuntu 8.10 defines _FORTIFY_SOURCE=2 which means, that fread, read etc
// are actually #defines which are inlined when compiled with -O. Those defines
// actually call __*chk (for example, __fread_chk). We need to bypass this whole
// thing to actually call our wrapped functions.
#if _FORTIFY_SOURCE > 1

size_t __wrap___fread_chk(void * ptr, size_t ptrlen, size_t size, size_t n, FILE * stream)
{
  return dll_fread(ptr, size, n, stream);
}

int __wrap___printf_chk(int flag, const char *format, ...)
{
  int res;
  va_list va;
  va_start(va, format);
  res = dll_vfprintf(stdout, format, va);
  va_end(va);
  return res;
}

int __wrap___vfprintf_chk(FILE* stream, int flag, const char *format, va_list ap)
{
  return dll_vfprintf(stream, format, ap);
}

int __wrap___fprintf_chk(FILE * stream, int flag, const char *format, ...)
{
  int res;
  va_list va;
  va_start(va, format);
  res = dll_vfprintf(stream, format, va);
  va_end(va);
  return res;
}

char *__wrap___fgets_chk(char *s, size_t size, int n, FILE *stream)
{
  return dll_fgets(s, n, stream);
}

size_t __wrap___read_chk(int fd, void *buf, size_t nbytes, size_t buflen)
{
  return dll_read(fd, buf, nbytes);
}

#endif
