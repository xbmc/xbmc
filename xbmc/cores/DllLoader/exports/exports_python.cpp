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

#define IS_STD_STREAM(stream) (stream == stdin || stream == stdout || stream == stderr)

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

bool is_root_drive(const char* path)
{
  int pathlen = strlen(path);
  if (pathlen == 2 || pathlen == 3)
  {
		if (path[0] == 'C' ||
		    path[0] == 'E' ||
		    path[0] == 'F' ||
		    path[0] == 'F' ||
		    path[0] == 'Q' ||
		    path[0] == 'S' ||
		  	path[0] == 'T' ||
		  	path[0] == 'U' ||
		  	path[0] == 'V' ||
		  	path[0] == 'Y' ||
		  	path[0] == 'Z')
		{
		  return true;
		}
  }
  return false;
}

char* full_path_strdup(const char* path)
{
  char* result = NULL;
  
  int iLen = strlen(path);
  
  if (iLen > 2)
  {
  	if (path[1] != ':')
		{
		  result = (char*)malloc(MAX_PATH);
		  xbp_getcwd(result, MAX_PATH);
		  
		  // add '\\'
			char* t = strchr(result, '\\');
			if (t[1] == 0) t[0] = 0;
			strcat(result, "\\");
			
			// append path
			strcat(result, path);
		}
  }
  
  if (result == NULL) result = strdup(path);
  return result;
}
	
int xbp_stat(const char *path, struct stat *buffer)
{
  if (is_root_drive(path))
  {
			buffer->st_dev = 4294967280;
			buffer->st_ino = 0;
			buffer->st_mode = 16895;
			buffer->st_nlink = 1;
			buffer->st_uid = 0;
			buffer->st_gid = 0;
			buffer->st_rdev = 4294967280;
			buffer->st_size = 0;
			buffer->st_atime = 1000000000;
			buffer->st_mtime = 1000000000;
			buffer->st_ctime = 1000000000;
			return 0;
  }
  
  char* p = full_path_strdup(path);
  CORRECT_SEP_STR(p);
  int res = stat(p, buffer);
  free(p);
  return res;
}

int xbp__stati64(const char *path, struct _stati64 *buffer)
{
  if (is_root_drive(path))
  {
			buffer->st_dev = 4294967280;
			buffer->st_ino = 0;
			buffer->st_mode = 16895;
			buffer->st_nlink = 1;
			buffer->st_uid = 0;
			buffer->st_gid = 0;
			buffer->st_rdev = 4294967280;
			buffer->st_size = 0;
			buffer->st_atime = 1000000000;
			buffer->st_mtime = 1000000000;
			buffer->st_ctime = 1000000000;
			return 0;
  }
  
  char* p = full_path_strdup(path);
  CORRECT_SEP_STR(p);
  int res = _stati64(p, buffer);
  free(p);
  return res;
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

int dll_vfprintf(FILE *stream, const char *format, va_list argptr);
int xbp_vfprintf(FILE *stream, const char *format, va_list argptr)
{
  if (IS_STD_STREAM(stream)) return dll_vfprintf(stream, format, argptr);

  return vfprintf(stream, format, argptr);
}

int dll_fprintf(FILE *stream, const char *format, ...);
int xbp_fprintf(FILE *stream, const char *format, ...)
{
  int res;
  va_list va;
  va_start(va, format);
  
  res = xbp_vfprintf(stream, format, va);

  va_end(va);
  
  return res;
}

int dll_fputs(const char *string, FILE *stream);
int xbp_fputs(const char *string, FILE *stream)
{
  if (IS_STD_STREAM(stream)) return dll_fputs(string, stream);
  return fputs(string, stream);
}

int dll_fputc(int c, FILE *stream);
int xbp_fputc(int c, FILE *stream)
{
  if (IS_STD_STREAM(stream)) return dll_fputc(c, stream);
  return fputc(c, stream);
}


size_t xbp_fwrite(const void *buffer, size_t size, size_t count, FILE *stream)
{
  if (IS_STD_STREAM(stream))
  {
    CLog::Log(LOGERROR, "xbp_fwrite called with stdout / stderr as param");
    return 0;
  }
  
  return fwrite(buffer, size, count, stream);
}

int xbp_write(int fd, const void *buffer, unsigned int count)
{
  int res = write(fd, buffer, count);
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

  //char* p = strdup(filename);
  //CORRECT_SEP_STR(p);
  FILE* res = fopen(cName, mode);
  //free(p);
  return res;
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

FILE* xbp__fdopen(int fd, const char *mode)
{
  return _fdopen(fd, mode);
}

dll_putc(int c, FILE *stream);
int xbp_putc(int c, FILE *stream)
{
  if (IS_STD_STREAM(stream)) return dll_fputc(c, stream);
  return fputc(c, stream);
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

} // extern "C"

void export_xbp()
{
  g_dlls.xbp.AddExport("xbp_fgetc", (unsigned long)fgetc);
  g_dlls.xbp.AddExport("xbp_clearerr", (unsigned long)clearerr);
  g_dlls.xbp.AddExport("xbp__stati64", (unsigned long)xbp__stati64);
  g_dlls.xbp.AddExport("xbp__fstati64", (unsigned long)_fstati64);
  g_dlls.xbp.AddExport("xbp__tempnam", (unsigned long)xbp__tempnam);
  g_dlls.xbp.AddExport("xbp_tmpfile", (unsigned long)tmpfile);
  g_dlls.xbp.AddExport("xbp_tmpnam", (unsigned long)tmpnam);
  g_dlls.xbp.AddExport("xbp__wfopen", (unsigned long)xbp__wfopen);
  g_dlls.xbp.AddExport("xbp__get_osfhandle", (unsigned long)_get_osfhandle);
  g_dlls.xbp.AddExport("xbp_vfprintf", (unsigned long)xbp_vfprintf);
  g_dlls.xbp.AddExport("xbp_unlink", (unsigned long)xbp_unlink);
  g_dlls.xbp.AddExport("xbp_access", (unsigned long)xbp_access);
  g_dlls.xbp.AddExport("xbp_chdir", (unsigned long)xbp_chdir);
  g_dlls.xbp.AddExport("xbp_chmod", (unsigned long)xbp_chmod);
  g_dlls.xbp.AddExport("xbp_rmdir", (unsigned long)xbp_rmdir);
  g_dlls.xbp.AddExport("xbp_umask", (unsigned long)umask);
  g_dlls.xbp.AddExport("xbp_utime", (unsigned long)xbp_utime);
  g_dlls.xbp.AddExport("xbp_dup", (unsigned long)dup);
  g_dlls.xbp.AddExport("xbp_dup2", (unsigned long)dup2);
  g_dlls.xbp.AddExport("xbp_rename", (unsigned long)xbp_rename);
  g_dlls.xbp.AddExport("xbp__commit", (unsigned long)_commit);
  g_dlls.xbp.AddExport("xbp__setmode", (unsigned long)_setmode);
  g_dlls.xbp.AddExport("xbp_fopen", (unsigned long)xbp_fopen);
  g_dlls.xbp.AddExport("xbp_ungetc", (unsigned long)ungetc);
  g_dlls.xbp.AddExport("xbp_fflush", (unsigned long)fflush);
  g_dlls.xbp.AddExport("xbp_fwrite", (unsigned long)fwrite);
  g_dlls.xbp.AddExport("xbp_fprintf", (unsigned long)xbp_fprintf);
  g_dlls.xbp.AddExport("xbp_fread", (unsigned long)fread);
  g_dlls.xbp.AddExport("xbp_fputs", (unsigned long)xbp_fputs);
  g_dlls.xbp.AddExport("xbp_getc", (unsigned long)getc);
  g_dlls.xbp.AddExport("xbp_setvbuf", (unsigned long)setvbuf);
  g_dlls.xbp.AddExport("xbp_fsetpos", (unsigned long)fsetpos);
  g_dlls.xbp.AddExport("xbp__lseek", (unsigned long)lseek);
  g_dlls.xbp.AddExport("xbp_ftell", (unsigned long)ftell);
  g_dlls.xbp.AddExport("xbp_fgets", (unsigned long)fgets);
  g_dlls.xbp.AddExport("xbp_fseek", (unsigned long)fseek);
  g_dlls.xbp.AddExport("xbp_putc", (unsigned long)xbp_putc);
  g_dlls.xbp.AddExport("xbp_write", (unsigned long)xbp_write);
  g_dlls.xbp.AddExport("xbp_read", (unsigned long)read);
  g_dlls.xbp.AddExport("xbp_close", (unsigned long)close);
  g_dlls.xbp.AddExport("xbp_mkdir", (unsigned long)xbp_mkdir);
  g_dlls.xbp.AddExport("xbp_open", (unsigned long)xbp_open);
  g_dlls.xbp.AddExport("xbp__fdopen", (unsigned long)xbp__fdopen);
  g_dlls.xbp.AddExport("xbp_lseek", (unsigned long)lseek);
  g_dlls.xbp.AddExport("xbp_fstat", (unsigned long)fstat);
  g_dlls.xbp.AddExport("xbp_stat", (unsigned long)xbp_stat);
  g_dlls.xbp.AddExport("xbp__lseeki64", (unsigned long)_lseeki64);
  g_dlls.xbp.AddExport("xbp_fileno", (unsigned long)fileno);
  g_dlls.xbp.AddExport("xbp_fputc", (unsigned long)xbp_fputc);
  g_dlls.xbp.AddExport("xbp_rewind", (unsigned long)rewind);
  g_dlls.xbp.AddExport("xbp_fclose", (unsigned long)fclose);
  g_dlls.xbp.AddExport("xbp_isatty", (unsigned long)isatty);
  g_dlls.xbp.AddExport("xbp_getcwd", (unsigned long)xbp_getcwd);
  g_dlls.xbp.AddExport("xbp_FindClose", (unsigned long)xbp_FindClose);
  g_dlls.xbp.AddExport("xbp_FindFirstFile", (unsigned long)xbp_FindFirstFile);
  g_dlls.xbp.AddExport("xbp_FindNextFile", (unsigned long)xbp_FindNextFile);
}
