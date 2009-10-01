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

typedef struct _NewsConnection
{
    char *host;
    unsigned short port;
    FILE *fp;
    SOCKET fd;
    struct _NewsConnection *next;
    int status; /* -1, 0, 1 */
} NewsConnection;

#define NNTP_OK_ID '2'
#define ALARM_TIMEOUT 10
/* #define DEBUG */

static VOLATILE int timeout_flag = 1;
static NewsConnection *connection_cache;
static int connection_cache_flag = URL_NEWS_CONN_NO_CACHE;

typedef struct _URL_news
{
    char common[sizeof(struct _URL)];

    NewsConnection *news;
    int status; /* for detection '\r?\n.\r?\n'
		 *                1  2 34  5
		 */
    int eof;
} URL_news;

enum
{
    ARTICLE_STATUS_0,
    ARTICLE_STATUS_1,
    ARTICLE_STATUS_2,
    ARTICLE_STATUS_3,
    ARTICLE_STATUS_4
};

static int name_news_check(char *url_string);
static long url_news_read(URL url, void *buff, long n);
static int url_news_fgetc(URL url);
static void url_news_close(URL url);

struct URL_module URL_module_news =
{
    URL_news_t,
    name_news_check,
    NULL,
    url_news_open,
    NULL
};

static int name_news_check(char *s)
{
    if(strncmp(s, "news://", 7) == 0 && strchr(s, '@') != NULL)
	return 1;
    return 0;
}

/*ARGSUSED*/
static void timeout(int sig)
{
    timeout_flag = 1;
}

static void close_news_server(NewsConnection *news)
{
    if(news->fd != (SOCKET)-1)
    {
	socket_write(news->fd, "QUIT\r\n", 6);
	closesocket(news->fd);
    }
    if(news->fp != NULL)
	socket_fclose(news->fp);
    free(news->host);
    news->status = -1;
}

static NewsConnection *open_news_server(char *host, unsigned short port)
{
    NewsConnection *p;
    char buff[512];

    for(p = connection_cache; p != NULL; p = p->next)
    {
	if(p->status == 0 && strcmp(p->host, host) == 0 && p->port == port)
	{
	    p->status = 1;
	    return p;
	}
    }
    for(p = connection_cache; p != NULL; p = p->next)
	if(p->status == -1)
	    break;
    if(p == NULL)
    {
	if((p = (NewsConnection *)safe_malloc(sizeof(NewsConnection))) == NULL)
	    return NULL;
	p->next = connection_cache;
	connection_cache = p;
	p->status = -1;
    }

    if((p->host = safe_strdup(host)) == NULL)
	return NULL;
    p->port = port;

#ifdef __W32__
    timeout_flag = 0;
    p->fd = open_socket(host, port);
#else
    timeout_flag = 0;
    signal(SIGALRM, timeout);
    alarm(ALARM_TIMEOUT);
    p->fd = open_socket(host, port);
    alarm(0);
    signal(SIGALRM, SIG_DFL);
#endif /* __W32__ */

    if(p->fd == (SOCKET)-1)
    {
	int save_errno;

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
#ifdef DEBUG
	perror(host);
#endif /* DEBUG */

	save_errno = errno;
	free(p->host);
	errno = save_errno;

	return NULL;
    }

    if((p->fp = socket_fdopen(p->fd, "rb")) == NULL)
    {
	url_errno = errno;
	closesocket(p->fd);
	free(p->host);
	errno = url_errno;
	return NULL;
    }

    buff[0] = '\0';
    if(socket_fgets(buff, sizeof(buff), p->fp) == NULL)
    {
	url_errno = errno;
	closesocket(p->fd);
	socket_fclose(p->fp);
	free(p->host);
	errno = url_errno;
	return NULL;
    }

#ifdef DEBUG
    printf("Connect status: %s", buff);
#endif /* DEBUG */

    if(buff[0] != NNTP_OK_ID)
    {
	closesocket(p->fd);
	socket_fclose(p->fp);
	free(p->host);
	url_errno = URLERR_CANTOPEN;
	errno = ENOENT;
	return NULL;
    }
    p->status = 1;
    return p;
}

int url_news_connection_cache(int flag)
{
    NewsConnection *p;
    int oldflag;

    oldflag = connection_cache_flag;

    switch(flag)
    {
      case URL_NEWS_CONN_NO_CACHE:
      case URL_NEWS_CONN_CACHE:
	connection_cache_flag = flag;
	break;
      case URL_NEWS_CLOSE_CACHE:
	for(p = connection_cache; p != NULL; p = p->next)
	    if(p->status == 0)
		close_news_server(p);
	break;
      case URL_NEWS_GET_FLAG:
	break;
    }
    return oldflag;
}

URL url_news_open(char *name)
{
    URL_news *url;
    char *host, *p;
    unsigned short port;
    char buff[BUFSIZ], messageID[256];
    int check_timeout;
    int i;

#ifdef DEBUG
    printf("url_news_open(%s)\n", name);
#endif /* DEBUG */

    url = (URL_news *)alloc_url(sizeof(URL_news));
    if(url == NULL)
    {
	url_errno = errno;
	return NULL;
    }

    /* common members */
    URLm(url, type)      = URL_news_t;
    URLm(url, url_read)  = url_news_read;
    URLm(url, url_gets)  = NULL;
    URLm(url, url_fgetc) = url_news_fgetc;
    URLm(url, url_seek)  = NULL;
    URLm(url, url_tell)  = NULL;
    URLm(url, url_close) = url_news_close;

    /* private members */
    url->news = NULL;
    url->status = ARTICLE_STATUS_2;
    url->eof = 0;

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
	    url_news_close((URL)url);
	    return NULL;
	}
    }
    else
	port = 119;
    *p++ = '\0'; /* terminate `host' string */
    if(*p == '<')
	p++;
    strncpy(messageID, p, sizeof(messageID) - 1);
    messageID[sizeof(messageID) - 1] = '\0';
    i = strlen(messageID);
    if(i > 0 && messageID[i - 1] == '>')
	messageID[i - 1] = '\0';

#ifdef DEBUG
    printf("messageID: <%s>\n", messageID);
#endif /* DEBUG */

#ifdef DEBUG
    printf("open(host=`%s', port=`%d')\n", host, port);
#endif /* DEBUG */

    if((url->news = open_news_server(host, port)) == NULL)
    {
	url_news_close((URL)url);
	return NULL;
    }

    check_timeout = 1;
  retry_article:

    sprintf(buff, "ARTICLE <%s>\r\n", messageID);

#ifdef DEBUG
    printf("CMD> %s", buff);
#endif /* DEBUG */

    socket_write(url->news->fd, buff, (long)strlen(buff));
    buff[0] = '\0';
    if(socket_fgets(buff, sizeof(buff), url->news->fp) == NULL)
    {
	if(check_timeout)
	{
	    check_timeout = 0;
	    close_news_server(url->news);
	    if((url->news = open_news_server(host, port)) != NULL)
		goto retry_article;
	}
	url_news_close((URL)url);
	url_errno = URLERR_CANTOPEN;
	errno = ENOENT;
	return NULL;
    }

#ifdef DEBUG
    printf("CMD< %s", buff);
#endif /* DEBUG */

    if(buff[0] != NNTP_OK_ID)
    {
	if(check_timeout && strncmp(buff, "503", 3) == 0)
	{
	    check_timeout = 0;
	    close_news_server(url->news);
	    if((url->news = open_news_server(host, port)) != NULL)
		goto retry_article;
	}
	url_news_close((URL)url);
	url_errno = errno = ENOENT;
	return NULL;
    }
    return (URL)url;
}

static void url_news_close(URL url)
{
    URL_news *urlp = (URL_news *)url;
    NewsConnection *news = urlp->news;
    int save_errno = errno;

    if(news != NULL)
    {
	if(connection_cache_flag == URL_NEWS_CONN_CACHE)
	    news->status = 0;
	else
	    close_news_server(news);
    }
    free(url);

    errno = save_errno;
}

static long url_news_read(URL url, void *buff, long size)
{
    char *p = (char *)buff;
    long n;
    int c;

    n = 0;
    while(n < size)
    {
	if((c = url_news_fgetc(url)) == EOF)
	    break;
	p[n++] = c;
    }
    return n;
}

static int url_news_fgetc(URL url)
{
    URL_news *urlp = (URL_news *)url;
    NewsConnection *news = urlp->news;
    int c;

    if(urlp->eof)
	return EOF;
    if((c = socket_fgetc(news->fp)) == EOF)
    {
	urlp->eof = 1;
	return EOF;
    }

    switch(urlp->status)
    {
      case ARTICLE_STATUS_0:
	if(c == '\r')
	    urlp->status = ARTICLE_STATUS_1;
	else if(c == '\n')
	    urlp->status = ARTICLE_STATUS_2;
	break;

      case ARTICLE_STATUS_1:
	if(c == '\n')
	    urlp->status = ARTICLE_STATUS_2;
	else
	    urlp->status = ARTICLE_STATUS_0;
	break;

      case ARTICLE_STATUS_2:
	if(c == '.')
	    urlp->status = ARTICLE_STATUS_3;
	else
	    urlp->status = ARTICLE_STATUS_0;
	break;

      case ARTICLE_STATUS_3:
	if(c == '\r')
	    urlp->status = ARTICLE_STATUS_4;
	else if(c == '\n')
	    urlp->eof = 1;
	else
	    urlp->status = ARTICLE_STATUS_0;
	break;

      case ARTICLE_STATUS_4:
	if(c == '\n')
	    urlp->eof = 1;
	break;
    }

    return c;
}

#ifdef NEWS_MAIN
void main(int argc, char **argv)
{
    URL url;
    char buff[BUFSIZ];
    int c;

    if(argc != 2)
    {
	fprintf(stderr, "Usage: %s news-URL\n", argv[0]);
	exit(1);
    }
    if((url = url_news_open(argv[1])) == NULL)
    {
	fprintf(stderr, "Can't open news group: %s\n", argv[1]);
	exit(1);
    }

#if NEWS_MAIN
    while((c = url_getc(url)) != EOF)
	putchar(c);
#else
    while((c = url_read(url, buff, sizeof(buff))) > 0)
	write(1, buff, c);
#endif
    url_close(url);
    exit(0);
}
#endif /* NEWS_MAIN */
