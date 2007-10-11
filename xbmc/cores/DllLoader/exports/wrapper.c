//
// To recreate wrapper.def:
// 
// bash# (echo -n "-Wl"; grep __wrap wrapper.c | grep -v bash | sed "s/.*__wrap_//g" | sed "s/(.*//g" | awk '{printf(",-wrap,%s",$0);}') > wrapper.def
//
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

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
long dll_ftell ( FILE * stream );
void dll_rewind(FILE* stream);
int dll_fgetpos(FILE* stream, fpos_t* pos);
int dll_fsetpos(FILE* stream, const fpos_t* pos);
int dll_fprintf(FILE* stream , const char * format, ...);
int dll_vfprintf(FILE *stream, const char *format, va_list va);
int dll_fgetc (FILE* stream);
char * dll_fgets (char* pszString, int num , FILE * stream);
int dll_getc (FILE * stream);
int dll_ungetc (int c, FILE * stream);

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

__off_t __wrap_lseek(int fildes, __off_t offset, int whence)
{
  return dll_lseek(fildes, offset, whence);
}

__off64_t __wrap_lseek64(int fildes, __off64_t offset, int whence)
{
  __off64_t seekRes = dll_lseeki64(fildes, offset, whence);
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
  return dll_clearerr(stream);
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

FILE *__wrap_fdopen(int fildes, const char *mode)
{
  return dll_fdopen(fildes, mode);
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

long __wrap_ftell(FILE *stream)
{
  return dll_ftell(stream);
}

void __wrap_rewind(FILE *stream)
{
  dll_rewind(stream);
}

int __wrap_fgetpos(FILE *stream, fpos_t *pos)
{
  return dll_fgetpos(stream, pos);
}

int __wrap_fsetpos(FILE *stream, fpos_t *pos)
{
  return dll_fsetpos(stream, pos);
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

int __wrap_ungetc(int c, FILE *stream)
{
  return dll_ungetc(c, stream);
}
