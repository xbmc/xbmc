#include "config.h"
#include "xb_util.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <xtl.h>
#endif
#include <stdio.h>
#include <io.h>

static char smb_current_dir[MAX_PATH];

int ftruncate(int fd, long size)
{
	return chsize(fd,size);
}

int putenv(const char* val)
{
	return 0;
}

void SMB_Output(const char* pszFormat, ...)
{
	va_list argList;
	char m_pszBuffer[1024];

	va_start( argList, pszFormat );
	vsprintf( m_pszBuffer, pszFormat, argList );
	OutputDebugString( m_pszBuffer );
	va_end( argList );
}

/*
 * Defines gettimeofday
 *
 * Based on timeval.h by Wu Yongwei
 * gettimeofday is always called by samba with NULL for *tz
 */
#define EPOCHFILETIME (116444736000000000i64)

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME        ft;
  LARGE_INTEGER   li;
  __int64         t;

  if (tv)
  {
      GetSystemTimeAsFileTime(&ft);
      li.LowPart  = ft.dwLowDateTime;
      li.HighPart = ft.dwHighDateTime;
      t  = li.QuadPart;       /* In 100-nanosecond intervals */
      t -= EPOCHFILETIME;     /* Offset to the Epoch time */
      t /= 10;                /* In microseconds */
      tv->tv_sec  = (long)(t / 1000000);
      tv->tv_usec = (long)(t % 1000000);
  }

  return 0;
}

char *smb_getcwd(char *buf, int size) {
	if (buf == NULL) {
		buf = (char *)malloc(size);
	}
	strcpy(buf, smb_current_dir);
	return buf;
}

int smb_chdir(const char *path) {
	if (strlen(path) > MAX_PATH) {
		return -1;
	}
	strcpy(smb_current_dir, path);
	return 0;
}

int strcasecmp(const char *s1, const char *s2)
{
  while (*s1 && *s2 && (tolower(*s1)==tolower(*s2)))
	{
    s1++;
    s2++;
  }
  return (tolower(*s1) - tolower(*s2));
}

int smb_getgid()
{
	return 1;
}

int smb_getuid()
{
	return 1;
}

int smb_getpid()
{
	return 1;
}

void smb_setgid(int id){}
void smb_setuid(int id){}

int waitpid(int pid,int *status, int options)
{
	return 0;
}

int fork()
{
	return 1;
}

struct group *getgrent(void)
{
	return NULL;
}

void setgrent(void){}

void endgrent(void){}

int getgroups(int size, int list[])
{
	return -1;
}

int setgroups(unsigned int size, const int *list)
{
	return -1;
}

struct passwd *getpwent(void)
{
	return NULL;
}

void setpwent(void) {}

void endpwent(void) {}

struct passwd *getpwnam(const char * name)
{
	return NULL;
}

struct passwd *getpwuid(uid_t uid)
{
	return NULL;
}

struct group *getgrnam(const char *name)
{
	return NULL;
}

struct group *getgrgid(gid_t gid)
{
	return NULL;
}

#ifndef _WIN32
int random()
{
	BYTE nr;
	XNetRandom(&nr, 31);
	return nr;
}
#endif

void charset_CP437_init(void)
{
}

void charset_CP850_init(void)
{
}

/*
#define ftruncate chsize

static int __truncate (const char *fname, int distance)
{
 int i = -1;
 HANDLE hf = CreateFile
  (  fname,
     GENERIC_READ | GENERIC_WRITE,
     FILE_SHARE_READ | FILE_SHARE_WRITE,
     NULL,
     OPEN_EXISTING,
     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
     NULL);
 if (hf != INVALID_HANDLE_VALUE)
 {
   if (SetFilePointer (hf, distance, NULL, FILE_BEGIN) != 0xFFFFFFFF)
   {
     if (SetEndOfFile (hf))
     { i = 0; }
   }
 CloseHandle (hf);
 }
 return i;
}
*/