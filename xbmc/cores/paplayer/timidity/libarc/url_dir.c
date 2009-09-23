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
#include "timidity.h"
#include "common.h"
#include "url.h"

#ifdef __W32READDIR__
#include "readdir.h"
# define NAMLEN(dirent) strlen((dirent)->d_name)
#elif __MACOS__
# include "mac_readdir.h"
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#endif

#ifdef URL_DIR_CACHE_ENABLE
#include <sys/stat.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include "strtab.h"

#ifndef S_ISDIR
#define S_ISDIR(mode)   (((mode)&0xF000) == 0x4000)
#endif /* S_ISDIR */

#ifdef unix
#define INODE_AVAILABLE
#endif /* unix */

struct dir_cache_t
{
    char **fnames;
#ifdef INODE_AVAILABLE
    dev_t dev;
    ino_t ino;
#else
    char *dirname;
#endif
    time_t dir_mtime;
    struct dir_cache_t *next;
};
static struct dir_cache_t *dir_cache = NULL;

#ifdef INODE_AVAILABLE
#define REMOVE_CACHE_ENT(p, isalloced) if(isalloced) free(p); else (p)->ino = 0
#else
#define REMOVE_CACHE_ENT(p, isalloced) free((p)->dirname); if(isalloced) free(p); else p->dirname = NULL
#endif /* INODE_AVAILABLE */

static struct dir_cache_t *scan_cached_files(struct dir_cache_t *p,
					     struct stat *s,
					     char *dirname)
{
    StringTable stab;
    DIR *dirp;
    struct dirent *d;
    int allocated;

    if(p == NULL)
    {
	if((p = (struct dir_cache_t *)safe_malloc(sizeof(struct dir_cache_t))) ==
	   NULL)
	    return NULL;
	allocated = 1;
    } else
	allocated = 0;

    /* save directory information */
#ifdef INODE_AVAILABLE
    p->ino = s->st_ino;
    p->dev = s->st_dev;
#else
    p->dirname = safe_strdup(dirname);
#endif /* INODE_AVAILABLE */
    p->dir_mtime = s->st_mtime;

    if((dirp = opendir(dirname)) == NULL)
    {
	url_errno = errno;
	REMOVE_CACHE_ENT(p, allocated);
	errno = url_errno;
	return NULL;
    }

    init_string_table(&stab);
    while((d = readdir(dirp)) != NULL)
    {
	int dlen;

#ifdef INODE_AVAILABLE
	if(d->d_ino == 0)
	    continue;
#endif
	if((dlen = NAMLEN(d)) == 0)
	    continue;

	/* put into string table */
	if(put_string_table(&stab, d->d_name, dlen) == NULL)
	{
	    url_errno = errno;
	    delete_string_table(&stab);
	    REMOVE_CACHE_ENT(p, allocated);
	    closedir(dirp);
	    errno = url_errno;
	    return NULL;
	}
    }
    closedir(dirp);

    /* make string array */
    p->fnames = make_string_array(&stab);
    if(p->fnames == NULL)
    {
	url_errno = errno;
	delete_string_table(&stab);
	REMOVE_CACHE_ENT(p, allocated);
	errno = url_errno;
	return NULL;
    }
    return p;
}

static struct dir_cache_t *read_cached_files(char *dirname)
{
    struct dir_cache_t *p, *q;
    struct stat s;

    if(stat(dirname, &s) < 0)
	return NULL;
    if(!S_ISDIR(s.st_mode))
    {
	errno = url_errno = ENOTDIR;
	return NULL;
    }

    q = NULL;
    for(p = dir_cache; p; p = p->next)
    {
#ifdef INODE_AVAILABLE
	if(p->ino == 0)
#else
	if(p->dirname == NULL)
#endif /* INODE_AVAILABLE */
	{
	    /* Entry is removed.
	     * Save the entry to `q' which is reused for puting in new entry.
	     */
	    if(q != NULL)
		q = p;
	    continue;
	}

#ifdef INODE_AVAILABLE
	if(s.st_dev == p->dev && s.st_ino == p->ino)
#else
	if(strcmp(p->dirname, dirname) == 0)
#endif /* INODE_AVAILABLE */

	{
	    /* found */
	    if(p->dir_mtime == s.st_mtime)
		return p;

	    /* Directory entry is updated */
	    free(p->fnames[0]);
	    free(p->fnames);
#ifndef INODE_AVAILABLE
	    free(p->dirname);
#endif /* !INODE_AVAILABLE */
	    return scan_cached_files(p, &s, dirname);
	}
    }
    /* New directory */
    if((p = scan_cached_files(q, &s, dirname)) == NULL)
	return NULL;
    p->next = dir_cache;
    dir_cache = p;
    return p;
}
#endif /* URL_DIR_CACHE_ENABLE */

typedef struct _URL_dir
{
    char common[sizeof(struct _URL)];
#ifdef URL_DIR_CACHE_ENABLE
    char **fptr;
#else
    DIR *dirp;
    struct dirent *d;
#endif /* URL_DIR_CACHE_ENABLE */

    char *ptr;
    int len;
    long total;
    char *dirname;
    int endp;
} URL_dir;

static int name_dir_check(char *url_string);
static long url_dir_read(URL url, void *buff, long n);
static char *url_dir_gets(URL url, char *buff, int n);
static long url_dir_tell(URL url);
static void url_dir_close(URL url);

struct URL_module URL_module_dir =
{
    URL_dir_t,			/* type */
    name_dir_check,		/* URL checker */
    NULL,			/* initializer */
    url_dir_open,		/* open */
    NULL			/* must be NULL */
};

static int name_dir_check(char *url_string)
{
    if(strncasecmp(url_string, "dir:", 4) == 0)
	return 1;
    url_string = pathsep_strrchr(url_string);
    return url_string != NULL && *(url_string + 1) == '\0';
}

#ifdef URL_DIR_CACHE_ENABLE
URL url_dir_open(char *dname)
{
    struct dir_cache_t *d;
    URL_dir *url;
    int dlen;

    if(dname == NULL)
	dname = ".";
    else
    {
	if(strncasecmp(dname, "dir:", 4) == 0)
	    dname += 4;
	if(*dname == '\0')
	    dname = ".";
	else
	    dname = url_expand_home_dir(dname);
    }
    dname = safe_strdup(dname);

    /* Remove tail of path sep. */
    dlen = strlen(dname);
    while(dlen > 0 && IS_PATH_SEP(dname[dlen - 1]))
	dlen--;
    dname[dlen] = '\0';
    if(dlen == 0)
	strcpy(dname, PATH_STRING); /* root */

    d = read_cached_files(dname);
    if(d == NULL)
    {
	free(dname);
	return NULL;
    }

    url = (URL_dir *)alloc_url(sizeof(URL_dir));
    if(url == NULL)
    {
	url_errno = errno;
	free(dname);
	errno = url_errno; /* restore errno */
	return NULL;
    }

    /* common members */
    URLm(url, type)      = URL_dir_t;
    URLm(url, url_read)  = url_dir_read;
    URLm(url, url_gets)  = url_dir_gets;
    URLm(url, url_fgetc) = NULL;
    URLm(url, url_seek)  = NULL;
    URLm(url, url_tell)  = url_dir_tell;
    URLm(url, url_close) = url_dir_close;

    /* private members */
    url->fptr = d->fnames;
    url->ptr = NULL;
    url->len = 0;
    url->total = 0;
    url->dirname = dname;
    url->endp = 0;

    return (URL)url;
}
#else
URL url_dir_open(char *dname)
{
    URL_dir *url;
    DIR *dirp;
    int dlen;

    if(dname == NULL)
	dname = ".";
    else
    {
	if(strncasecmp(dname, "dir:", 4) == 0)
	    dname += 4;
	if(*dname == '\0')
	    dname = ".";
	else
	    dname = url_expand_home_dir(dname);
    }
    dname = safe_strdup(dname);

    /* Remove tail of path sep. */
    dlen = strlen(dname);
    while(dlen > 0 && IS_PATH_SEP(dname[dlen - 1]))
	dlen--;
    dname[dlen] = '\0';
    if(dlen == 0)
	strcpy(dname, PATH_STRING); /* root */

    if((dirp = opendir(dname)) == NULL)
    {
	url_errno = errno;
	free(dname);
	errno = url_errno;
	return NULL;
    }

    url = (URL_dir *)alloc_url(sizeof(URL_dir));
    if(url == NULL)
    {
	url_errno = errno;
	closedir(dirp);
	free(dname);
	errno = url_errno;
	return NULL;
    }

    /* common members */
    URLm(url, type)      = URL_dir_t;
    URLm(url, url_read)  = url_dir_read;
    URLm(url, url_gets)  = url_dir_gets;
    URLm(url, url_fgetc) = NULL;
    URLm(url, url_seek)  = NULL;
    URLm(url, url_tell)  = url_dir_tell;
    URLm(url, url_close) = url_dir_close;

    /* private members */
    url->dirp = dirp;
    url->d = NULL;
    url->ptr = NULL;
    url->len = 0;
    url->total = 0;
    url->dirname = dname;
    url->endp = 0;

    return (URL)url;
}
#endif /* URL_DIR_CACHE_ENABLE */

static long url_dir_tell(URL url)
{
    return ((URL_dir *)url)->total;
}

char *url_dir_name(URL url)
{
    if(url->type != URL_dir_t)
	return NULL;
    return ((URL_dir *)url)->dirname;
}

static void url_dir_close(URL url)
{
    URL_dir *urlp = (URL_dir *)url;
#ifndef URL_DIR_CACHE_ENABLE
    closedir(urlp->dirp);
#endif
    free(urlp->dirname);
    free(urlp);
}

static long url_dir_read(URL url, void *buff, long n)
{
    char *p;

    p = url_dir_gets(url, (char *)buff, (int)n);
    if(p == NULL)
	return 0;
    return (long)strlen(p);
}

static char *url_dir_gets(URL url, char *buff, int n)
{
    URL_dir *urlp = (URL_dir *)url;
    int i;

    if(urlp->endp)
	return NULL;
    if(n <= 0)
	return buff;
    if(n == 1)
    {
	*buff = '\0';
	return buff;
    }
    n--; /* for '\0' */;
    for(;;)
    {
	if(urlp->len > 0)
	{
	    i = urlp->len;
	    if(i > n)
		i = n;
	    memcpy(buff, urlp->ptr, i);
	    buff[i] = '\0';
	    urlp->len -= i;
	    urlp->ptr += i;
	    urlp->total += i;
	    return buff;
	}

#ifdef URL_DIR_CACHE_ENABLE
	if(*urlp->fptr == NULL)
	{
	    urlp->endp = 1;
	    return NULL;
	}
	urlp->ptr = *urlp->fptr;
	urlp->fptr++;
	urlp->len = strlen(urlp->ptr);
#else
	do
	    if((urlp->d = readdir(urlp->dirp)) == NULL)
	    {
		urlp->endp = 1;
		return NULL;
	    }
        while (
#ifdef INODE_AVAILABLE
	       urlp->d->d_ino == 0 ||
#endif /* INODE_AVAILABLE */
               NAMLEN(urlp->d) == 0);
	urlp->ptr = urlp->d->d_name;
	urlp->len = NAMLEN(urlp->d);
#endif
    }
}
