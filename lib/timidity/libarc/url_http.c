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
#include "url.h"
#include "net.h"

/* #define DEBUG */

#ifdef HTTP_PROXY_HOST
char *url_http_proxy_host = HTTP_PROXY_HOST;
unsigned short url_http_proxy_port = HTTP_PROXY_PORT;
#else
char *url_http_proxy_host = NULL;
unsigned short url_http_proxy_port;
#endif /* HTTP_PROXY_HOST */


#define REQUEST_OFFSET 16

#define ALARM_TIMEOUT 10
static VOLATILE int timeout_flag = 1;


typedef struct _URL_http
{
    char common[sizeof(struct _URL)];

    FILE *fp;
} URL_http;

static int name_http_check(char *url_string);
static long url_http_read(URL url, void *buff, long n);
static char *url_http_gets(URL url, char *buff, int n);
static int url_http_fgetc(URL url);
static void url_http_close(URL url);

struct URL_module URL_module_http =
{
    URL_http_t,
    name_http_check,
    NULL,
    url_http_open,
    NULL
};

static int name_http_check(char *s)
{
    if(strncmp(s, "http://", 7) == 0)
	return 1;
    return 0;
}

/*ARGSUSED*/
static void timeout(int sig)
{
    timeout_flag = 1;
}

URL url_http_open(char *name)
{
    URL_http *url;
    SOCKET fd;
    char *host, *path = NULL, *p;
    unsigned short port;
    char buff[BUFSIZ];
    char wwwserver[256];
    int n;

#ifdef DEBUG
    printf("url_http_open(%s)\n", name);
#endif /* DEBUG */

    url = (URL_http *)alloc_url(sizeof(URL_http));
    if(url == NULL)
    {
	url_errno = errno;
	return NULL;
    }

    /* common members */
    URLm(url, type)      = URL_http_t;
    URLm(url, url_read)  = url_http_read;
    URLm(url, url_gets)  = url_http_gets;
    URLm(url, url_fgetc) = url_http_fgetc;
    URLm(url, url_seek)  = NULL;
    URLm(url, url_tell)  = NULL;
    URLm(url, url_close) = url_http_close;

    /* private members */
    url->fp = NULL;

    if(url_http_proxy_host)
    {
	char *q;
	int len;

	host = url_http_proxy_host;
	port = url_http_proxy_port;

	p = name;
	if(strncmp(p, "http://", 7) == 0)
	    p += 7;
	for(q = p; *q && *q != ':' && *q != '/'; q++)
	    ;
	len = q - p;
	if(len >= sizeof(wwwserver) - 1) { /* What?? */
	    strcpy(wwwserver, "localhost");
	} else {
	    strncpy(wwwserver, p, len);
	}
    }
    else
    {
	if(strncmp(name, "http://", 7) == 0)
	    name += 7;
	n = strlen(name);
	if(n + REQUEST_OFFSET >= BUFSIZ)
	{
	    url_http_close((URL)url);
	    url_errno = URLERR_URLTOOLONG;
	    errno = ENOENT;
	    return NULL;
	}

	memcpy(buff, name, n + 1);

	host = buff;
	for(p = host; *p && *p != ':' && *p != '/'; p++)
	    ;
	if(*p == ':')
	{
	    char *pp;

	    *p++ = '\0'; /* terminate `host' string */
	    port = atoi(p);
	    pp = strchr(p, '/');
	    if(pp == NULL)
		p[0] = '\0';
	    else
		p = pp;
	}
	else
	    port = 80;
	path = p;

	if(*path == '\0')
	    *(path + 1) = '\0';

	*path = '\0'; /* terminate `host' string */
	strncpy(wwwserver, host, sizeof(wwwserver));
    }

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
    fd = open_socket(host, port);
    alarm(0);
    signal(SIGALRM, SIG_DFL);
#endif /* __W32__ */

    if(fd  == (SOCKET)-1)
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
	url_http_close((URL)url);
	return NULL;
    }

    if((url->fp = socket_fdopen(fd, "rb")) == NULL)
    {
	url_errno = errno;
	closesocket(fd);
	url_http_close((URL)url);
	errno = url_errno;
	return NULL;
    }

    if(url_http_proxy_host)
	sprintf(buff, "GET %s HTTP/1.0\r\n", name);
    else
    {
	*path = '/';
	sprintf(buff, "GET %s HTTP/1.0\r\n", path);
    }
    socket_write(fd, buff, (long)strlen(buff));

#ifdef DEBUG
    printf("HTTP<%s", buff);
#endif /* DEBUG */

    if(url_user_agent)
    {
	sprintf(buff, "User-Agent: %s\r\n", url_user_agent);
	socket_write(fd, buff, (long)strlen(buff));
#ifdef DEBUG
	printf("HTTP<%s", buff);
#endif /* DEBUG */
    }

    /* Host field */
    sprintf(buff, "Host: %s\r\n", wwwserver);
    socket_write(fd, buff, (long)strlen(buff));
#ifdef DEBUG
    printf("HTTP<%s", buff);
#endif /* DEBUG */

    /* End of header */
    socket_write(fd, "\r\n", 2);
    socket_shutdown(fd, 1);

    if(socket_fgets(buff, BUFSIZ, url->fp) == NULL)
    {
	if(errno)
	    url_errno = errno;
	else
	{
	    url_errno = URLERR_CANTOPEN;
	    errno = ENOENT;
	}
	url_http_close((URL)url);
	return NULL;
    }

#ifdef DEBUG
    printf("HTTP>%s", buff);
#endif /* DEBUG */

    p = buff;
    if(strncmp(p, "HTTP/1.0 ", 9) == 0 || strncmp(p, "HTTP/1.1 ", 9) == 0)
	p += 9;
    if(strncmp(p, "200", 3) != 0) /* Not success */
    {
	url_http_close((URL)url);
	url_errno = errno = ENOENT;
	return NULL;
    }

    /* Skip mime header */
    while(socket_fgets(buff, BUFSIZ, url->fp) != NULL)
    {
	if(buff[0] == '\n' || (buff[0] == '\r' && buff[1] == '\n'))
	    break; /* end of heaer */
#ifdef DEBUG
	printf("HTTP>%s", buff);
#endif /* DEBUG */
    }

#ifdef __W32__
    return url_buff_open((URL)url, 1);
#else
    return (URL)url;
#endif /* __W32__ */
}

static void url_http_close(URL url)
{
    URL_http *urlp = (URL_http *)url;
    int save_errno = errno;
    if(urlp->fp != NULL)
	socket_fclose(urlp->fp);
    free(url);
    errno = save_errno;
}

static long url_http_read(URL url, void *buff, long n)
{
    URL_http *urlp = (URL_http *)url;
    return socket_fread(buff, n, urlp->fp);
}

static char *url_http_gets(URL url, char *buff, int n)
{
    URL_http *urlp = (URL_http *)url;
    return socket_fgets(buff, n, urlp->fp);
}

static int url_http_fgetc(URL url)
{
    URL_http *urlp = (URL_http *)url;
    int n;
    unsigned char c;

    n = socket_fread(&c, 1, urlp->fp);
    if(n <= 0)
    {
	if(errno)
	    url_errno = errno;
	return EOF;
    }
    return (int)c;
}
