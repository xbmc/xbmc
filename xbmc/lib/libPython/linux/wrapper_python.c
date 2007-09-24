#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <utime.h>
#include <fcntl.h>

FILE * xbp_fopen64(const char * filename, const char * mode);
char* xbp_getcwd(char *buf, int size);
int xbp_chdir(const char *dirname);
int xbp_access(const char *path, int mode);
int xbp_unlink(const char *filename);
int xbp_chmod(const char *filename, int pmode);
int xbp_rmdir(const char *dirname);
int xbp_utime(const char *filename, struct utimbuf *times);
int xbp_rename(const char *oldname, const char *newname);
int xbp_mkdir(const char *dirname);
int xbp_open(const char *filename, int oflag, int pmode);
FILE* xbp_fopen(const char *filename, const char *mode);
FILE* xbp_freopen(const char *path, const char *mode, FILE *stream);
FILE* xbp_fopen64(const char *filename, const char *mode);
DIR *xbp_opendir(const char *name);
int xbp__xstat64(int ver, const char *filename, struct stat64 *stat_buf);
int xbp__lxstat64(int ver, const char *filename, struct stat64 *stat_buf);
void *xbp_dlopen(const char *filename, int flag);
int xbp_dlclose(void *handle);
void *xbp_dlsym(void *handle, const char *symbol);

FILE *__wrap_fopen64(const char *path, const char *mode)
{
  return xbp_fopen64(path, mode);
}

DIR* __wrap_opendir(const char *name)
{
  return xbp_opendir(name);
}

int __wrap_access(const char* path, int mode)
{
  return xbp_access(path, mode);
}

char* __wrap_getcwd(char *buf, int size)
{
  return xbp_getcwd(buf, size);
}

int __wrap_chdir(const char *dirname)
{
  return xbp_chdir(dirname);
}

int __wrap_open(const char *file, int oflag, ...)
{
  if (oflag & O_CREAT) 
  {
  	va_list args;
	  mode_t mode;
	  va_start(args, oflag);
	  mode = va_arg(args, mode_t);
	  va_end(args);
	  return xbp_open(file, oflag | O_NONBLOCK, mode);
  } 
  else 
	  return xbp_open(file, oflag | O_NONBLOCK, 0);
}

FILE *__wrap_fopen(const char *path, const char *mode)
{
  return xbp_fopen(path, mode);
}

FILE *__wrap_freopen(const char *path, const char *mode, FILE *stream)
{
  return xbp_freopen(path, mode, stream);
}

int __wrap_unlink(const char *filename)
{
  return xbp_unlink(filename);
}

int __wrap_chmod(const char *filename, int pmode)
{
  return xbp_chmod(filename, pmode);
}

int __wrap_rmdir(const char *dirname)
{
  return xbp_rmdir(dirname);
}

int __wrap_utime(const char *filename, struct utimbuf *times)
{
  return xbp_utime(filename, times);
}

int __wrap_rename(const char *oldname, const char *newname)
{
  return xbp_rename(oldname, newname);
}

int __wrap_mkdir(const char *dirname)
{
  return xbp_mkdir(dirname);
}

int __wrap___xstat64(int ver, const char *filename, struct stat64 *stat_buf)
{
  return xbp__xstat64(ver, filename, stat_buf);
}

int __wrap___lxstat64(int ver, const char *filename, struct stat64 *stat_buf)
{
  return xbp__lxstat64(ver, filename, stat_buf);
}

void *__wrap_dlopen(const char *filename, int flag)
{
  return xbp_dlopen(filename,flag);
}

int __wrap_dlclose(void *handle)
{
  return xbp_dlclose(handle);
}

void *__wrap_dlsym(void *handle, const char *symbol)
{
  return xbp_dlsym(handle, symbol);
}

