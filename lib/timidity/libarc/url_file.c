/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <fcntl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

// I'm not sure xbmc maps mmapped IO
#undef HAVE_MMAP

#ifdef __W32__
#include <windows.h>
#endif /* __W32__ */

#include "timidity.h"

#ifdef HAVE_MMAP
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#ifndef MAP_FAILED
#define MAP_FAILED ((caddr_t)-1)
#endif /* MAP_FAILED */

#else
/* mmap is not supported */
#ifdef __W32__
#define try_mmap(fname, size_ret) w32_mmap(fname, size_ret, &hFile, &hMap)
#define munmap(addr, size)        w32_munmap(addr, size, hFile, hMap)
#else
#define try_mmap(dmy1, dmy2) NULL
#define munmap(addr, size) /* Do nothing */
#endif /* __W32__ */
#endif

#include "url.h"
#ifdef __MACOS__
#include "mblock.h"
#endif

#if !defined(__W32__) && !defined(O_BINARY)
#define O_BINARY 0
#endif


typedef struct _URL_file
{
    char common[sizeof(struct _URL)];

    char *mapptr;		/* Non NULL if mmap is success */
    long mapsize;
    long pos;

#ifdef __W32__
    HANDLE hFile, hMap;
#endif /* __W32__ */

    FILE *fp;			/* Non NULL if mmap is failure */
} URL_file;

static int name_file_check(char *url_string);
static long url_file_read(URL url, void *buff, long n);
static char *url_file_gets(URL url, char *buff, int n);
static int url_file_fgetc(URL url);
static long url_file_seek(URL url, long offset, int whence);
static long url_file_tell(URL url);
static void url_file_close(URL url);

struct URL_module URL_module_file =
{
    URL_file_t,			/* type */
    name_file_check,		/* URL checker */
    NULL,			/* initializer */
    url_file_open,		/* open */
    NULL			/* must be NULL */
};

static int name_file_check(char *s)
{
    int i;

    if(IS_PATH_SEP(s[0]))
	return 1;

    if(strncasecmp(s, "file:", 5) == 0)
	return 1;

	if(strncasecmp(s, "filereader:", 10) == 0)
    return 1;

    if(strncasecmp(s, "special:", 8) == 0)
    return 1;

#ifdef __W32__
    /* [A-Za-z]: (for Windows) */
    if((('A' <= s[0] && s[0] <= 'Z') ||
	('a' <= s[0] && s[0] <= 'z')) &&
       s[1] == ':')
	return 1;
#endif /* __W32__ */

    for(i = 0; s[i] && s[i] != ':' && s[i] != '/'; i++)
	;
    if(s[i] == ':' && s[i + 1] == '/')
	return 0;

    return 1;
}

#ifdef HAVE_MMAP
static char *try_mmap(char *path, long *size)
{
    int fd;
    char *p;
    struct stat st;

    errno = 0;
    fd = open(path, O_RDONLY | O_BINARY);
    if(fd < 0)
	return NULL;

    if(fstat(fd, &st) < 0)
    {
	int save_errno = errno;
	close(fd);
	errno = save_errno;
	return NULL;
    }

    p = (char *)mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if(p == (char *)MAP_FAILED)
    {
	int save_errno = errno;
	close(fd);
	errno = save_errno;
	return NULL;
    }
    close(fd);
    *size = (long)st.st_size;
    return p;
}
#elif defined(__W32__)
static void *w32_mmap(char *fname, long *size_ret, HANDLE *hFilePtr, HANDLE *hMapPtr)
{
    void *map;

    *hFilePtr = CreateFile(fname, GENERIC_READ, FILE_SHARE_READ	, NULL,
			   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(*hFilePtr == INVALID_HANDLE_VALUE)
	return NULL;
    *size_ret = GetFileSize(*hFilePtr, NULL);
    if(*size_ret == 0xffffffff)
    {
	CloseHandle(*hFilePtr);
	return NULL;
    }
    *hMapPtr = CreateFileMapping(*hFilePtr, NULL, PAGE_READONLY,
				 0, 0, NULL);
    if(*hMapPtr == NULL)
    {
	CloseHandle(*hFilePtr);
	return NULL;
    }
    map = MapViewOfFile(*hMapPtr, FILE_MAP_READ, 0, 0, 0);
    if(map == NULL)
    {
	CloseHandle(*hMapPtr);
	CloseHandle(*hFilePtr);
	return NULL;
    }
    return map;
}
static void w32_munmap(void *ptr, long size, HANDLE hFile, HANDLE hMap)
{
    UnmapViewOfFile(ptr);
    CloseHandle(hMap);
    CloseHandle(hFile);
}
#endif /* HAVE_MMAP */

URL url_file_open(char *fname)
{
    URL_file *url;
    char *mapptr;		/* Non NULL if mmap is success */
    long mapsize;
    FILE *fp;			/* Non NULL if mmap is failure */
#ifdef __W32__
    HANDLE hFile, hMap;
#endif /* __W32__ */

#ifdef DEBUG
    printf("url_file_open(%s)\n", fname);
#endif /* DEBUG */

    if(!strcmp(fname, "-"))
    {
	mapptr = NULL;
	mapsize = 0;
	fp = stdin;
	goto done;
    }

    if(strncasecmp(fname, "file:", 5) == 0)
	fname += 5;
    if(*fname == '\0')
    {
	url_errno = errno = ENOENT;
	return NULL;
    }
    fname = url_expand_home_dir(fname);

    fp = NULL;
    mapsize = 0;
    errno = 0;
    mapptr = try_mmap(fname, &mapsize);
    if(errno == ENOENT || errno == EACCES)
    {
	url_errno = errno;
	return NULL;
    }

#ifdef DEBUG
    if(mapptr != NULL)
	printf("mmap - success. size=%d\n", mapsize);
#ifdef HAVE_MMAP
    else
	printf("mmap - failure.\n");
#endif
#endif /* DEBUG */


    if(mapptr == NULL)
    {
#ifdef __MACOS__
	char *cnvname;
	MBlockList pool;
	init_mblock(&pool);
	cnvname = (char *)strdup_mblock(&pool, fname);
	mac_TransPathSeparater(fname, cnvname);
	fp = fopen(cnvname, "rb");
	reuse_mblock(&pool);
	if( fp==NULL ){ /*try original name*/
		fp = fopen(fname, "rb");
	}
#else
	fp = fopen(fname, "rb");
#endif
	if(fp == NULL)
	{
	    url_errno = errno;
	    return NULL;
	}
    }

  done:
    url = (URL_file *)alloc_url(sizeof(URL_file));
    if(url == NULL)
    {
	url_errno = errno;
	if(mapptr)
	    munmap(mapptr, mapsize);
	if(fp && fp != stdin)
	    fclose(fp);
	errno = url_errno;
	return NULL;
    }

    /* common members */
    URLm(url, type)      = URL_file_t;
    URLm(url, url_read)  = url_file_read;
    URLm(url, url_gets)  = url_file_gets;
    URLm(url, url_fgetc) = url_file_fgetc;
    URLm(url, url_close) = url_file_close;
    if(fp == stdin)
    {
	URLm(url, url_seek) = NULL;
	URLm(url, url_tell) = NULL;
    }
    else
    {
	URLm(url, url_seek) = url_file_seek;
	URLm(url, url_tell) = url_file_tell;
    }

    /* private members */
    url->mapptr = mapptr;
    url->mapsize = mapsize;
    url->pos = 0;
    url->fp = fp;
#ifdef __W32__
    url->hFile = hFile;
    url->hMap = hMap;
#endif /* __W32__ */

    return (URL)url;
}

static long url_file_read(URL url, void *buff, long n)
{
    URL_file *urlp = (URL_file *)url;

    if(urlp->mapptr != NULL)
    {
	if(urlp->pos + n > urlp->mapsize)
	    n = urlp->mapsize - urlp->pos;
	memcpy(buff, urlp->mapptr + urlp->pos, n);
	urlp->pos += n;
    }
    else
    {
	if((n = (long)fread(buff, 1, n, urlp->fp)) == 0)
	{
	    if(ferror(urlp->fp))
	    {
		url_errno = errno;
		return -1;
	    }
	    return 0;
	}
    }
    return n;
}

char *url_file_gets(URL url, char *buff, int n)
{
    URL_file *urlp = (URL_file *)url;

    if(urlp->mapptr != NULL)
    {
	long s;
	char *nlp, *p;

	if(urlp->mapsize == urlp->pos)
	    return NULL;
	if(n <= 0)
	    return buff;
	if(n == 1)
	{
	    *buff = '\0';
	    return buff;
	}
	n--; /* for '\0' */
	s = urlp->mapsize - urlp->pos;
	if(s > n)
	    s = n;
	p = urlp->mapptr + urlp->pos;
	nlp = (char *)memchr(p, url_newline_code, s);
	if(nlp != NULL)
	    s = nlp - p + 1;
	memcpy(buff, p, s);
	buff[s] = '\0';
	urlp->pos += s;
	return buff;
    }

    return fgets(buff, n, urlp->fp);
}

int url_file_fgetc(URL url)
{
    URL_file *urlp = (URL_file *)url;

    if(urlp->mapptr != NULL)
    {
	if(urlp->mapsize == urlp->pos)
	    return EOF;
	return urlp->mapptr[urlp->pos++] & 0xff;
    }

#ifdef getc
    return getc(urlp->fp);
#else
    return fgetc(urlp->fp);
#endif /* getc */
}

static void url_file_close(URL url)
{
    URL_file *urlp = (URL_file *)url;

    if(urlp->mapptr != NULL)
    {
#ifdef __W32__
	HANDLE hFile = urlp->hFile;
	HANDLE hMap = urlp->hMap;
#endif /* __W32__ */
	munmap(urlp->mapptr, urlp->mapsize);
    }
    if(urlp->fp != NULL)
    {
	if(urlp->fp == stdin)
	    rewind(stdin);
	else
	    fclose(urlp->fp);
    }
    free(url);
}

static long url_file_seek(URL url, long offset, int whence)
{
    URL_file *urlp = (URL_file *)url;
    long ret;

    if(urlp->mapptr == NULL)
	return fseek(urlp->fp, offset, whence);
    ret = urlp->pos;
    switch(whence)
    {
      case SEEK_SET:
	urlp->pos = offset;
	break;
      case SEEK_CUR:
	urlp->pos += offset;
	break;
      case SEEK_END:
	  urlp->pos = urlp->mapsize + offset;
	break;
    }
    if(urlp->pos > urlp->mapsize)
	urlp->pos = urlp->mapsize;
    else if(urlp->pos < 0)
	urlp->pos = 0;

    return ret;
}

static long url_file_tell(URL url)
{
    URL_file *urlp = (URL_file *)url;

    return urlp->mapptr ? urlp->pos : ftell(urlp->fp);
}
