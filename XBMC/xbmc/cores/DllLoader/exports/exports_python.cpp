#include "..\..\..\stdafx.h"
#include <io.h>
#include <direct.h>
#include <sys/stat.h>
#include <sys/utime.h>

#include "..\DllLoaderContainer.h"

// file io functions
#define CORRECT_SEP_STR(str) \
    int iSize_##str = strlen(str); \
    for (int pos = 0; pos < iSize_##str; pos++) if (str[pos] == '/') str[pos] = '\\';

#define CORRECT_SEP_WSTR(str) \
    int iSize_##str = wcslen(str); \
    for (int pos = 0; pos < iSize_##str; pos++) if (str[pos] == '/') str[pos] = '\\';

#include "..\dll_tracker_file.h"
#include "emu_msvcrt.h"

extern "C"
{

static char xbp_cw_dir[MAX_PATH] = "Q:\\python";

char* xbp_getcwd(char *buf, int size)
{
	if (buf == NULL) buf = (char *)malloc(size);
	strcpy(buf, xbp_cw_dir);
	return buf;
}

int xbp_chdir(const char *dirname)
{
  if (strlen(dirname) > MAX_PATH) return -1;

  strcpy(xbp_cw_dir, dirname);
  CORRECT_SEP_STR(xbp_cw_dir);

  return 0;
}

char* xbp__tempnam(const char *dir, const char *prefix)
{
  char* p = strdup(dir);
  CORRECT_SEP_STR(p);
  char* res = _tempnam(p, prefix);
  free(p);
  return res;
}

int xbp_unlink(const char *filename)
{
  char* p = strdup(filename);
  CORRECT_SEP_STR(p);
  int res = unlink(p);
  free(p);
  return res;
}

int xbp_access(const char *path, int mode)
{
  char* p = strdup(path);
  CORRECT_SEP_STR(p);
  int res = access(p, mode);
  free(p);
  return res;
}

int xbp_chmod(const char *filename, int pmode)
{
  char* p = strdup(filename);
  CORRECT_SEP_STR(p);
  int res = chmod(p, pmode);
  free(p);
  return res;
}

int xbp_rmdir(const char *dirname)
{
  char* p = strdup(dirname);
  CORRECT_SEP_STR(p);
  int res = rmdir(p);
  free(p);
  return res;
}

int xbp_utime(const char *filename, struct utimbuf *times)
{
  char* p = strdup(filename);
  CORRECT_SEP_STR(p);
  int res = utime(p, times);
  free(p);
  return res;
}

int xbp_rename(const char *oldname, const char *newname)
{
  char* o = strdup(oldname);
  char* n = strdup(newname);
  CORRECT_SEP_STR(o);
  CORRECT_SEP_STR(n);
  int res = rename(o, n);
  free(o);
  free(n);
  return res;
}

int xbp_mkdir(const char *dirname)
{
  char* p = strdup(dirname);
  CORRECT_SEP_STR(p);
  int res = mkdir(p);
  free(p);
  return res;
}

int xbp_open(const char *filename, int oflag, int pmode)
{
  char* p = strdup(filename);
  CORRECT_SEP_STR(p);
  int res = open(p, oflag, pmode);
  free(p);
  return res;
}

FILE* xbp_fopen(const char *filename, const char *mode)
{
	//convert '/' to '\\'
	char cName[1024];
	char* p;

	strcpy(cName, filename);
	CORRECT_SEP_STR(cName);

	//for each "\\..\\" remove the directory before it
	while(p = strstr(cName, "\\..\\"))
	{
		char* file = p + 3;
		*p = '\0';
		*strrchr(cName, '\\') = '\0';
		strcat(cName, file);
	}

  // don't use emulated files, they do not work in python yet
  return fopen(cName, mode);
}

int xbp_fclose(FILE* stream)
{
  return dll_fclose(stream);
}

int xbp_close(int fd)
{
  return dll_close(fd);
}

FILE* xbp__wfopen(const wchar_t *filename, const wchar_t *mode)
{
  CLog::Log(LOGERROR, "xbp__wfopen = untested");
  wchar_t* p = wcsdup(filename);
  CORRECT_SEP_WSTR(p);
  FILE* res = _wfopen(p, mode);
  free(p);
  return res;
}

BOOL xbp_FindClose(HANDLE hFindFile)
{
  return FindClose(hFindFile);
}

HANDLE xbp_FindFirstFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
  char* p = strdup(lpFileName);
  CORRECT_SEP_STR(p);
  
  // change default \\*.* into \\* which the xbox is using
  char* e = strrchr(p, '.');
  if (e != NULL && strlen(e) > 1 && e[1] == '*')
  {
    e[0] = '\0';
  }
  
  HANDLE res = FindFirstFile(p, lpFindFileData);
  free(p);
  return res;
}

BOOL xbp_FindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
{
  return FindNextFile(hFindFile, lpFindFileData);
}

long xbp__get_osfhandle(int fd)
{
  return _get_osfhandle(fd);
}

int xbp_dup(int fd)
{
  return dup(fd);
}

int xbp_dup2(int fd1, int fd2)
{
  return dup2(fd1, fd2);
}

} // extern "C"

void export_xbp()
{
  g_dlls.xbp.AddExport("xbp_fgetc", (unsigned long)dll_fgetc);
  g_dlls.xbp.AddExport("xbp_clearerr", (unsigned long)dll_clearerr);
  //g_dlls.xbp.AddExport("xbp__stati64", (unsigned long)xbp__stati64);
  g_dlls.xbp.AddExport("xbp__fstati64", (unsigned long)dll_fstati64);
  g_dlls.xbp.AddExport("xbp__tempnam", (unsigned long)xbp__tempnam);
  g_dlls.xbp.AddExport("xbp_tmpfile", (unsigned long)tmpfile);
  g_dlls.xbp.AddExport("xbp_tmpnam", (unsigned long)tmpnam);
  g_dlls.xbp.AddExport("xbp__wfopen", (unsigned long)xbp__wfopen);
  g_dlls.xbp.AddExport("xbp__get_osfhandle", (unsigned long)xbp__get_osfhandle);
  //g_dlls.xbp.AddExport("xbp_vfprintf", (unsigned long)xbp_vfprintf);
  g_dlls.xbp.AddExport("xbp_unlink", (unsigned long)xbp_unlink);
  g_dlls.xbp.AddExport("xbp_access", (unsigned long)xbp_access);
  g_dlls.xbp.AddExport("xbp_chdir", (unsigned long)xbp_chdir);
  g_dlls.xbp.AddExport("xbp_chmod", (unsigned long)xbp_chmod);
  g_dlls.xbp.AddExport("xbp_rmdir", (unsigned long)xbp_rmdir);
  g_dlls.xbp.AddExport("xbp_umask", (unsigned long)umask);
  g_dlls.xbp.AddExport("xbp_utime", (unsigned long)xbp_utime);
  g_dlls.xbp.AddExport("xbp_dup", (unsigned long)xbp_dup);
  g_dlls.xbp.AddExport("xbp_dup2", (unsigned long)xbp_dup2);
  g_dlls.xbp.AddExport("xbp_rename", (unsigned long)xbp_rename);
  g_dlls.xbp.AddExport("xbp__commit", (unsigned long)_commit);
  g_dlls.xbp.AddExport("xbp__setmode", (unsigned long)dll_setmode);
  g_dlls.xbp.AddExport("xbp_fopen", (unsigned long)xbp_fopen);
  g_dlls.xbp.AddExport("xbp_ungetc", (unsigned long)dll_ungetc);
  g_dlls.xbp.AddExport("xbp_fflush", (unsigned long)dll_fflush);
  g_dlls.xbp.AddExport("xbp_fwrite", (unsigned long)dll_fwrite);
  //g_dlls.xbp.AddExport("xbp_fprintf", (unsigned long)xbp_fprintf);
  g_dlls.xbp.AddExport("xbp_fread", (unsigned long)dll_fread);
  //g_dlls.xbp.AddExport("xbp_fputs", (unsigned long)xbp_fputs);
  g_dlls.xbp.AddExport("xbp_getc", (unsigned long)dll_getc);
  g_dlls.xbp.AddExport("xbp_setvbuf", (unsigned long)setvbuf);
  g_dlls.xbp.AddExport("xbp_fsetpos", (unsigned long)dll_fsetpos);
  g_dlls.xbp.AddExport("xbp_fgetpos", (unsigned long)dll_fgetpos);
  g_dlls.xbp.AddExport("xbp__lseek", (unsigned long)dll_lseek);
  g_dlls.xbp.AddExport("xbp_ftell", (unsigned long)dll_ftell);
  //g_dlls.xbp.AddExport("xbp_ftell64", (unsigned long)ftell64);
  g_dlls.xbp.AddExport("xbp_fgets", (unsigned long)dll_fgets);
  g_dlls.xbp.AddExport("xbp_fseek", (unsigned long)dll_fseek);
  //g_dlls.xbp.AddExport("xbp_putc", (unsigned long)xbp_putc);
  //g_dlls.xbp.AddExport("xbp_write", (unsigned long)xbp_write);
  g_dlls.xbp.AddExport("xbp_read", (unsigned long)dll_read);
  g_dlls.xbp.AddExport("xbp_close", (unsigned long)xbp_close);
  g_dlls.xbp.AddExport("xbp_mkdir", (unsigned long)xbp_mkdir);
  g_dlls.xbp.AddExport("xbp_open", (unsigned long)xbp_open);
  //g_dlls.xbp.AddExport("xbp__fdopen", (unsigned long)xbp__fdopen);
  g_dlls.xbp.AddExport("xbp_lseek", (unsigned long)dll_lseek);
  g_dlls.xbp.AddExport("xbp_fstat", (unsigned long)dll_fstat);
  //g_dlls.xbp.AddExport("xbp_stat", (unsigned long)xbp_stat);
  g_dlls.xbp.AddExport("xbp__lseeki64", (unsigned long)_lseeki64);
  //g_dlls.xbp.AddExport("xbp_fileno", (unsigned long)xbp_fileno);
  //g_dlls.xbp.AddExport("xbp_fputc", (unsigned long)xbp_fputc);
  g_dlls.xbp.AddExport("xbp_rewind", (unsigned long)rewind);
  g_dlls.xbp.AddExport("xbp_fclose", (unsigned long)xbp_fclose);
  g_dlls.xbp.AddExport("xbp_isatty", (unsigned long)isatty);
  g_dlls.xbp.AddExport("xbp_getcwd", (unsigned long)xbp_getcwd);
  //g_dlls.xbp.AddExport("xbp_getenv", (unsigned long)xbp_getenv);
  //g_dlls.xbp.AddExport("xbp_putenv", (unsigned long)xbp_putenv);
  g_dlls.xbp.AddExport("xbp_FindClose", (unsigned long)xbp_FindClose);
  g_dlls.xbp.AddExport("xbp_FindFirstFile", (unsigned long)xbp_FindFirstFile);
  g_dlls.xbp.AddExport("xbp_FindNextFile", (unsigned long)xbp_FindNextFile);
}
