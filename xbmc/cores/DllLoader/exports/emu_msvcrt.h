
#ifndef _EMU_MSVCRT_H_
#define _EMU_MSVCRT_H_

typedef void ( *PFV)(void);

extern "C"
{
  char* dll_strdup( const char* str);
  void dll_sleep(unsigned long imSec);
  void InitFiles();
  void dllReleaseAll( );
  void* dllmalloc(size_t size);
  void dllfree( void* pPtr );
  void* dllcalloc( size_t num, size_t size );
  void* dllrealloc( void *memblock, size_t size );
  void dllexit(int iCode);
  void dllabort();
  void* dll__dllonexit(PFV input, PFV ** start, PFV ** end);
  _onexit_t dll_onexit(_onexit_t func);
  void dllputs(const char* szLine);
  void dllprintf( const char *format, ... );
  char *_fullpath(char *absPath, const char *relPath, size_t maxLength);
  FILE *_popen(const char *command, const char *mode);
  int _pclose(FILE *stream);
  FILE* dll_fdopen(int i, const char* file);
  int dll_open(const char* szFileName, int iMode);
  int dll_read(int fd, void* buffer, unsigned int uiSize);
  int dll_write(int fd, const void* buffer, unsigned int uiSize);
  int dll_close(int fd);
  __int64 dll_lseeki64(int fd, __int64 lPos, int iWhence);
  long dll_lseek(int fd, long lPos, int iWhence);
  char* dll_getenv(const char* szKey);
  int dll_fclose (FILE * stream);
  intptr_t dll_findfirst(const char *file, struct _finddata_t *data);
  int dll_findnext(intptr_t f, _finddata_t* data);
  char * dll_fgets (char* pszString, int num , FILE * stream);
  int dll_feof (FILE * stream);
  int dll_fread (void * buffer, size_t size, size_t count, FILE * stream);
  int dll_getc (FILE * stream);
  FILE * dll_fopen (const char * filename, const char * mode);
  int dll_fputc (int character, FILE * stream);
  int dll_fputs (const char * szLine , FILE* stream);
  int dll_fseek ( FILE * stream , long offset , int origin );
  int dll_ungetc (int c, FILE * stream);
  long dll_ftell ( FILE * stream );
  long dll_tell ( int fd );
  __int64 dll_telli64 ( int fd );
  size_t dll_fwrite ( const void * buffer, size_t size, size_t count, FILE * stream );
  int dll_fflush (FILE * stream);
  int dll_ferror (FILE * stream);
  int dll_vfprintf(FILE *stream, const char *format, va_list va);
  int dll_fprintf(FILE* stream , const char * format, ...);
  int dll_fgetpos(FILE* stream, fpos_t* pos);
  int dll_fsetpos(FILE* stream, const fpos_t* pos);
  int dll_fileno(FILE* stream);
  char* dll_strdup( const char* str);
  int dll_initterm(PFV * start, PFV * end);
  HANDLE dll_beginthreadex(LPSECURITY_ATTRIBUTES lpThreadAttributes, DWORD dwStackSize,
                           LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags,
                           LPDWORD lpThreadId);
  int dll_stati64(const char *path, struct _stati64 *buffer);
  int dll_fstat(FILE * stream, struct _stat *buffer);
  int dll_fstati64(FILE * stream, struct _stati64 *buffer);
  int dll_setmode ( int handle, int mode );
  void dllperror(const char* s);
  char* dllstrerror(int iErr);
  int dll_mkdir(const char* dir);
  char* dll_getcwd(char *buffer, int maxlen);
  int dll_putenv(const char* envstring);
  int dll_ctype(int i);
  int dll_system(const char *command);
  void (__cdecl * dll_signal(int sig, void (__cdecl *func)(int)))(int);
  int dll_getpid();
};



#endif // _EMU_MSVCRT_H_