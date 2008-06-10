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
#ifdef __APPLE__
int xbp_stat(const char * path, struct stat * buf);
int xbp_lstat(const char * path, struct stat * buf);
#else
int xbp__xstat64(int ver, const char *filename, struct stat64 *stat_buf);
int xbp__lxstat64(int ver, const char *filename, struct stat64 *stat_buf);
#endif
void *xbp_dlopen(const char *filename, int flag);
int xbp_dlclose(void *handle);
void *xbp_dlsym(void *handle, const char *symbol);

#ifdef __APPLE__
#define PYTHON_WRAP(func) __py_wrap_##func
#else
#define PYTHON_WRAP(func) __wrap_##func
#endif

FILE *PYTHON_WRAP(fopen64)(const char *path, const char *mode)
{
  return xbp_fopen64(path, mode);
}

DIR* PYTHON_WRAP(opendir)(const char *name)
{
  return xbp_opendir(name);
}

int PYTHON_WRAP(access)(const char* path, int mode)
{
  return xbp_access(path, mode);
}

char* PYTHON_WRAP(getcwd)(char *buf, int size)
{
  return xbp_getcwd(buf, size);
}

int PYTHON_WRAP(chdir)(const char *dirname)
{
  return xbp_chdir(dirname);
}

int PYTHON_WRAP(open)(const char *file, int oflag, ...)
{
  if (oflag & O_CREAT) 
  {
  	va_list args;
	  mode_t mode;
	  va_start(args, oflag);
#ifdef __APPLE__
	  mode = va_arg(args, int);
#else
	  mode = va_arg(args, mode_t);
#endif
	  va_end(args);
	  return xbp_open(file, oflag | O_NONBLOCK, mode);
  } 
  else 
	  return xbp_open(file, oflag | O_NONBLOCK, 0);
}

FILE *PYTHON_WRAP(fopen)(const char *path, const char *mode)
{
  return xbp_fopen(path, mode);
}

FILE *PYTHON_WRAP(freopen)(const char *path, const char *mode, FILE *stream)
{
  return xbp_freopen(path, mode, stream);
}

int PYTHON_WRAP(unlink)(const char *filename)
{
  return xbp_unlink(filename);
}

int PYTHON_WRAP(chmod)(const char *filename, int pmode)
{
  return xbp_chmod(filename, pmode);
}

int PYTHON_WRAP(rmdir)(const char *dirname)
{
  return xbp_rmdir(dirname);
}

int PYTHON_WRAP(utime)(const char *filename, struct utimbuf *times)
{
  return xbp_utime(filename, times);
}

int PYTHON_WRAP(rename)(const char *oldname, const char *newname)
{
  return xbp_rename(oldname, newname);
}

int PYTHON_WRAP(mkdir)(const char *dirname)
{
  return xbp_mkdir(dirname);
}

#ifdef __APPLE__
int PYTHON_WRAP(stat)(const char * path, struct stat * buf)
{
  return xbp_stat(path, buf);
}

int PYTHON_WRAP(lstat)(const char * path, struct stat * buf)
{
  return xbp_lstat(path, buf);
}
#else
int PYTHON_WRAP(__xstat64)(int ver, const char *filename, struct stat64 *stat_buf)
{
  return xbp__xstat64(ver, filename, stat_buf);
}

int PYTHON_WRAP(__lxstat64)(int ver, const char *filename, struct stat64 *stat_buf)
{
  return xbp__lxstat64(ver, filename, stat_buf);
}
#endif

void *PYTHON_WRAP(dlopen)(const char *filename, int flag)
{
  return xbp_dlopen(filename,flag);
}

int PYTHON_WRAP(dlclose)(void *handle)
{
  return xbp_dlclose(handle);
}

void *PYTHON_WRAP(dlsym)(void *handle, const char *symbol)
{
  return xbp_dlsym(handle, symbol);
}

