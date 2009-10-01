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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <signal.h> /* for SIGALRM */

#include "timidity.h"
#include "common.h"
#include "url.h"
#include "net.h"

#define NNTP_OK_ID '2'
#define MAX_LINE_BUFF 1024
#define ALARM_TIMEOUT 10
/* #define DEBUG */

#ifdef URL_NEWS_XOVER_SUPPORT
static char *xover_commands[] = {URL_NEWS_XOVER_SUPPORT, NULL};
#else
static char *xover_commands[] = {NULL};
#endif /* URL_NEWS_XOVER_SUPPORT */

static VOLATILE int timeout_flag = 1;

typedef struct _URL_newsgroup
{
    char common[sizeof(struct _URL)];

    FILE *fp;
    SOCKET fd;
    int first, last;
    int minID, maxID;
    int xover;
    int eof;
    char *name;
} URL_newsgroup;

static int name_newsgroup_check(char *url_string);
static long url_newsgroup_read(URL url, void *buff, long n);
static char *url_newsgroup_gets(URL url, char *buff, int n);
static void url_newsgroup_close(URL url);

struct URL_module URL_module_newsgroup =
{
    URL_newsgroup_t,
    name_newsgroup_check,
    NULL,
    url_newsgroup_open,
    NULL
};

static int name_newsgroup_check(char *s)
{
    if(strncmp(s, "news://", 7) == 0 && strchr(s, '@') == NULL)
	return 1;
    return 0;
}

/*ARGSUSED*/
static void timeout(int sig)
{
    timeout_flag = 1;
}

URL url_newsgroup_open(char *name)
{
    URL_newsgroup *url;
    SOCKET fd;
    char *host, *p, *urlname;
    unsigned short port;
    char buff[BUFSIZ], group[256], *range;
    int n;

#ifdef DEBUG
    printf("url_newsgroup_open(%s)\n", name);
#endif /* DEBUG */

    if((urlname = safe_strdup(name)) == NULL)
	return NULL;
    n = strlen(urlname);
    while(n > 0 && urlname[n - 1] == '/')
	urlname[--n] = '\0';

    url = (URL_newsgroup *)alloc_url(sizeof(URL_newsgroup));
    if(url == NULL)
    {
	url_errno = errno;
	free(urlname);
	errno = url_errno;
	return NULL;
    }

    /* common members */
    URLm(url, type)      = URL_newsgroup_t;
    URLm(url, url_read)  = url_newsgroup_read;
    URLm(url, url_gets)  = url_newsgroup_gets;
    URLm(url, url_fgetc) = NULL;
    URLm(url, url_seek)  = NULL;
    URLm(url, url_tell)  = NULL;
    URLm(url, url_close) = url_newsgroup_close;

    /* private members */
    url->fd = (SOCKET)-1;
    url->fp = NULL;
    url->xover = -1;
    url->eof = 0;
    url->first = url->last = 0;
    url->minID = url->maxID = 0;
    url->name = urlname;

    if(strncmp(name, "news://", 7) == 0)
	name += 7;

    strncpy(buff, name, sizeof(buff) - 1);
    buff[sizeof(buff) - 1] = '\0';

    host = buff;
    for(p = host; *p && *p != ':' && *p != '/'; p++)
	;
    if(*p == ':')
    {
	*p++ = '\0'; /* terminate `host' string */
	port = atoi(p);
	p = strchr(p, '/');
	if(p == NULL)
	{
	    url_errno = URLERR_CANTOPEN;
	    errno = ENOENT;
	    url_newsgroup_close((URL)url);
	    return NULL;
	}
    }
    else
	port = 119;
    *p++ = '\0'; /* terminate `host' string */
    strncpy(group, p, sizeof(group) - 1);
    group[sizeof(group) - 1] = '\0';

    if((range = strchr(group, '/')) != NULL)
	*range++ = '\0';

#ifdef DEBUG
    printf("group: %s\n", group);
#endif /* DEBUG */

#ifdef DEBUG
    printf("open(host=`%s', port=`%d')\n", host, port);
#endif /* DEBUG */

#ifdef __W32__
    timeout_flag = 0;
    fd = open_socket(host, port);
#else
    timeout_flag = 0;
    signal(SIGALRM, timeout);
    alarm(ALARM_TIMEOUT);
    url->fd = fd = open_socket(host, port);
    alarm(0);
    signal(SIGALRM, SIG_DFL);
#endif /* __W32__ */

    if(fd == (SOCKET)-1)
    {
	VOLATILE_TOUCH(timeout_flag);
#ifdef ETIMEDOUT
	if(timeout_flag)
	    errno = ETIMEDOUT;
#endif /* ETIMEDOUT */
	if(errno)
	    url_errno = errno;
	else
	{
	    url_errno = URLERR_CANTOPEN;
	    errno = ENOENT;
	}
	url_newsgroup_close((URL)url);
	return NULL;
    }

    if((url->fp = socket_fdopen(fd, "rb")) == NULL)
    {
	url_errno = errno;
	closesocket(fd);
	url_newsgroup_close((URL)url);
	errno = url_errno;
	return NULL;
    }

    if(socket_fgets(buff, sizeof(buff), url->fp) == NULL)
    {
	url_newsgroup_close((URL)url);
	return NULL;
    }

#ifdef DEBUG
    printf("Connect status: %s", buff);
#endif /* DEBUG */

    if(buff[0] != NNTP_OK_ID)
    {
	url_newsgroup_close((URL)url);
	url_errno = URLERR_CANTOPEN;
	errno = ENOENT;
	return NULL;
    }

    sprintf(buff, "GROUP %s\r\n", group);

#ifdef DEBUG
    printf("CMD> %s", buff);
#endif /* DEBUG */

    socket_write(fd, buff, (long)strlen(buff));
    if(socket_fgets(buff, sizeof(buff), url->fp) == NULL)
    {
	url_newsgroup_close((URL)url);
	url_errno = URLERR_CANTOPEN;
	errno = ENOENT;
	return NULL;
    }

#ifdef DEBUG
    printf("CMD< %s", buff);
#endif /* DEBUG */

    if(buff[0] != NNTP_OK_ID)
    {
	url_newsgroup_close((URL)url);
	url_errno = URLERR_CANTOPEN;
	errno = ENOENT;
	return NULL;
    }

    p = buff + 4;
    if(*p == '0') /* No article */
	url->eof = 1;
    p++;
    while('0' <= *p && *p <= '9')
	p++;
    while(*p == ' ')
	p++;
    url->first = url->minID = atoi(p);
    while('0' <= *p && *p <= '9')
	p++;
    while(*p == ' ')
	p++;
    url->last = url->maxID = atoi(p);

    if(range != NULL)
    {
	if('0' <= *range && *range <= '9')
	{
	    url->first = atoi(range);
	    if(url->first < url->minID)
		url->first = url->minID;
	}
	if((range = strchr(range, '-')) != NULL)
	{
	    range++;
	    if('0' <= *range && *range <= '9')
	    {
		url->last = atoi(range);
		if(url->last > url->maxID)
		    url->last = url->maxID;
	    }
	}
    }

    return (URL)url;
}

char *url_newsgroup_name(URL url)
{
    if(url->type != URL_newsgroup_t)
	return NULL;
    return ((URL_newsgroup *)url)->name;
}

static void url_newsgroup_close(URL url)
{
    URL_newsgroup *urlp = (URL_newsgroup *)url;
    int save_errno = errno;
    if(urlp->fd != (SOCKET)-1)
    {
	socket_write(urlp->fd, "QUIT\r\n", 6);
	closesocket(urlp->fd);
    }
    if(urlp->fp != NULL)
	socket_fclose(urlp->fp);
    if(urlp->name != NULL)
	free(urlp->name);
    free(url);
    errno = save_errno;
}

static long url_newsgroup_read(URL url, void *buff, long n)
{
    char *p;

    p = url_newsgroup_gets(url, (char *)buff, n);
    if(p == NULL)
	return 0;
    return (long)strlen(p);
}

static char *url_newsgroup_gets(URL url, char *buff, int n)
{
    URL_newsgroup *urlp = (URL_newsgroup *)url;
    char linebuff[MAX_LINE_BUFF], *p, numbuf[32];
    int i, j, nump;
    int find_first;

    if(urlp->eof || n <= 0)
	return NULL;
    if(n == 1)
    {
	buff[0] = '\0';
	return buff;
    }

    find_first = 0;
    if(urlp->xover == -1)
    {
	urlp->xover = 0;
	for(i = 0; xover_commands[i] != NULL; i++)
	{
	    sprintf(linebuff, "%s %d-%d\r\n", xover_commands[i],
		    urlp->first, urlp->last);
#ifdef DEBUG
	    printf("CMD> %s", linebuff);
#endif /* DEBUG */
	    socket_write(urlp->fd, linebuff, (long)strlen(linebuff));
	    if(socket_fgets(linebuff, sizeof(linebuff), urlp->fp) == NULL)
	    {
		urlp->eof = 1;
		return NULL;
	    }
#ifdef DEBUG
	    printf("CMD< %s", linebuff);
#endif /* DEBUG */
	    if(linebuff[0] == NNTP_OK_ID)
	    {
		urlp->xover = 1;
		break;
	    }
	}
	if(!urlp->xover)
	    find_first = 1;
    }
    else if(!urlp->xover)
	socket_write(urlp->fd, "NEXT\r\n", 6);

  next_read:
    if(find_first)
    {
	for(i = urlp->first; i <= urlp->last; i++)
	{
	    sprintf(linebuff, "STAT %d\r\n", i);
#ifdef DEBUG
	    printf("CMD> %s", linebuff);
#endif /* DEBUG */
	    socket_write(urlp->fd, linebuff, (long)strlen(linebuff));
	    if(socket_fgets(linebuff, sizeof(linebuff), urlp->fp) == NULL)
	    {
		urlp->eof = 1;
		return NULL;
	    }
#ifdef DEBUG
	    printf("CMD< %s", linebuff);
#endif /* DEBUG */
	    if(atoi(linebuff) != 423)
		break;
	}
	if(i > urlp->last)
	{
	    urlp->eof = 1;
	    return NULL;
	}
	find_first = 0;
    }
    else
    {
	if(socket_fgets(linebuff, sizeof(linebuff), urlp->fp) == NULL)
	{
	    urlp->eof = 1;
	    return NULL;
	}

	i = strlen(linebuff);
	if(i > 0 && linebuff[i - 1] != '\n')
	{
	    int c;

	    do
	    {
		c = socket_fgetc(urlp->fp);
	    } while(c != '\n' && c != EOF);
	}
    }
    p = linebuff;
#ifdef DEBUG
    printf("line: %s", linebuff);
#endif /* DEBUG */

    if(urlp->xover == 0)
    {
	if(p[0] != '2')
	{
	    if(strncmp(p, "421", 3) == 0)
	    {
		urlp->eof = 1;
		return NULL;
	    }

	    socket_write(urlp->fd, "NEXT\r\n", 6);
	    goto next_read;
	}

	p += 3;
	while(*p == ' ' || *p == '\t')
	    p++;
	nump = 0;
	i = atoi(p);
	if(i > urlp->last)
	{
	    urlp->eof = 1;
	    return NULL;
	}
	if(i == urlp->last)
	    urlp->eof = 1;
	while('0' <= *p && *p <= '9' && nump < sizeof(numbuf))
	    numbuf[nump++] = *p++;
	if(nump == 0)
	{
	    socket_write(urlp->fd, "NEXT\r\n", 6);
	    goto next_read;
	}

	if((p = strchr(linebuff, '<')) == NULL)
	{
	    socket_write(urlp->fd, "NEXT\r\n", 6);
	    goto next_read;
	}
    }
    else
    {
	int i;

	if(linebuff[0] == '.')
	{
	    urlp->eof = 1;
	    return NULL;
	}

	nump = 0;
	while('0' <= linebuff[nump] && linebuff[nump] <= '9'
	      && nump < sizeof(numbuf))
	{
	    numbuf[nump] = linebuff[nump];
	    nump++;
	}

	for(i = 0; i < 4; i++)
	{
	    p = strchr(p, '\t');
	    if(p == NULL)
		goto next_read;
	    p++;
	}
    }

    if(*p == '<')
	p++;

    i = j = 0;
    while(j < n - 2 && j < nump)
    {
	buff[j] = numbuf[j];
	j++;
    }

    buff[j++] = ' ';
    while(j < n - 1 && p[i] && p[i] != '>' && p[i] != ' ' && p[i] != '\t')
    {
	buff[j] = p[i];
	i++;
	j++;
    }
    buff[j] = '\0';
    return buff;
}

#ifdef NEWSGROUP_MAIN
void *safe_malloc(int n) { return malloc(n); }
void *safe_realloc(void *p, int n) { return realloc(p, n); }
void main(int argc, char **argv)
{
    URL url;
    char buff[BUFSIZ];

    if(argc != 2)
    {
	fprintf(stderr, "Usage: %s news-URL\n", argv[0]);
	exit(1);
    }
    if((url = url_newsgroup_open(argv[1])) == NULL)
    {
	fprintf(stderr, "Can't open news group: %s\n", argv[1]);
	exit(1);
    }

    while(url_gets(url, buff, sizeof(buff)) != NULL)
	puts(buff);
    url_close(url);
    exit(0);
}
#endif /* NEWSGROUP_MAIN */
