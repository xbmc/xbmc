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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <signal.h> /* for SIGALRM */

#include "timidity.h"
#include "url.h"
#include "net.h"

/* supported PASV mode only */

#define ALARM_TIMEOUT 10
static VOLATILE int timeout_flag = 0;

#ifdef FTP_PROXY_HOST
char *url_ftp_proxy_host = FTP_PROXY_HOST
unsigned short url_ftp_proxy_port = FTP_PROXY_PORT
#else
char *url_ftp_proxy_host = NULL;
unsigned short url_ftp_proxy_port;
#endif /* FTP_PROXY_HOST */


typedef struct _URL_ftp
{
    char common[sizeof(struct _URL)];
    FILE *datafp;
    FILE *ctlifp;
    FILE *ctlofp;
    int abor;
} URL_ftp;

static int name_ftp_check(char *url_string);
static long url_ftp_read(URL url, void *buff, long size);
static char *url_ftp_gets(URL url, char *buff, int n);
static int url_ftp_fgetc(URL url);
static void url_ftp_close(URL url);
static int guess_errno(char *msg);

struct URL_module URL_module_ftp =
{
    URL_ftp_t,
    name_ftp_check,
    NULL,
    url_ftp_open,
    NULL
};

static int name_ftp_check(char *s)
{
    if(strncmp(s, "ftp://", 6) == 0)
	return 1;
    return 0;
}

static int ftp_cmd(URL_ftp *url, char *buff, char *rspns)
{
#ifdef DEBUG
    printf("FTP<%s", buff);
#endif
    errno = 0;
    if(socket_fwrite(buff, (long)strlen(buff), url->ctlofp) <= 0)
    {
	url_ftp_close((URL)url);
	if(errno)
	    url_errno = errno;
	else
	    url_errno = errno = ENOENT;
	return -1;
    }
    socket_fflush(url->ctlofp);
    do
    {
	errno = 0;
	if(socket_fgets(buff, BUFSIZ, url->ctlifp) == NULL)
	{
	    url_ftp_close((URL)url);
	    if(errno)
		url_errno = errno;
	    else
		url_errno = errno = ENOENT;
	    return -1;
	}
#ifdef DEBUG
	printf("FTP>%s", buff);
#endif
	if(strncmp(buff, rspns, 3) != 0)
	{
	    url_ftp_close((URL)url);
	    url_errno = errno = guess_errno(buff);
	    return -1;
	}
    } while(buff[3] == '-');
    return 0;
}

/*ARGSUSED*/
static void timeout(int sig)
{
    timeout_flag = 1;
}

static int guess_errno(char *msg)
{
    if(strncmp(msg, "550", 3) != 0)
	return ENOENT;
    if((msg = strchr(msg, ':')) == NULL)
	return ENOENT;
    msg++;
    if(*msg == ' ')
	msg++;
    if(strncmp(msg, "No such file or directory", 25) == 0)
	return ENOENT;
    if(strncmp(msg, "Permission denied", 17) == 0)
	return EACCES;
    if(strncmp(msg, "HTTP/1.0 500", 12) == 0) /* Proxy Error */
	return ENOENT;
    return ENOENT;
}

URL url_ftp_open(char *name)
{
    URL_ftp *url;
    SOCKET fd;
    char *p, *host, *path;
    unsigned short port;
    char buff[BUFSIZ];
    char path_buff[1024], host_buff[1024];
    int n;
    char *passwd;
    char *user;

#ifdef DEBUG
    printf("url_ftp_open(%s)\n", name);
#endif /* DEBUG */

    passwd = user_mailaddr;
    user   = "anonymous";

    url = (URL_ftp *)alloc_url(sizeof(URL_ftp));
    if(url == NULL)
    {
	url_errno = errno;
	return NULL;
    }

    /* common members */
    URLm(url, type)      = URL_ftp_t;
    URLm(url, url_read)  = url_ftp_read;
    URLm(url, url_gets)  = url_ftp_gets;
    URLm(url, url_fgetc) = url_ftp_fgetc;
    URLm(url, url_seek)  = NULL;
    URLm(url, url_tell)  = NULL;
    URLm(url, url_close) = url_ftp_close;

    /* private members */
    url->datafp = NULL;
    url->ctlifp = NULL;
    url->ctlofp = NULL;
    url->abor = 0;

    if(url_ftp_proxy_host != NULL)
    {
	/* proxy */
	host = url_ftp_proxy_host;
	port = url_ftp_proxy_port;
    }
    else
    {
	/* not proxy */
	if(strncmp(name, "ftp://", 6) == 0)
	    name += 6;
	strncpy(buff, name, sizeof(buff));
	buff[sizeof(buff) - 1] = '\0';

	strncpy(host_buff, buff, sizeof(host_buff));
	host_buff[sizeof(host_buff) - 1] = '\0';
	host = host_buff;

	if((p = strchr(host, '/')) == NULL)
	{
	    url_ftp_close((URL)url);
	    url_errno = URLERR_IURLF;
	    errno = ENOENT;
	    return NULL;
	}

	port = 21;
	*p = '\0';
	strncpy(path_buff, name + strlen(host), sizeof(path_buff));
	path_buff[sizeof(path_buff) - 1] = '\0';
	path = path_buff;

	/* check user:password@host */
	p = strchr(host, '@');
	if(p != NULL)
	{
	    user = host;
	    host = p;
	    *host++ = '\0';
	    if((passwd = strchr(user, ':')) == NULL)
		passwd = user_mailaddr;
	    else
		*passwd++ = '\0';
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

	if(fd < 0)
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
	    url_ftp_close((URL)url);
	    return NULL;
	}

	if((url->ctlifp = socket_fdopen(fd, "rb")) == NULL)
	{
	    url_ftp_close((URL)url);
	    url_errno = errno;
	    return NULL;
	}

	if((url->ctlofp = socket_fdopen(fd, "wb")) == NULL)
	{
	    url_ftp_close((URL)url);
	    url_errno = errno;
	    return NULL;
	}

	if(socket_fgets(buff, BUFSIZ, url->ctlifp) == NULL)
	{
	    url_ftp_close((URL)url);
	    url_errno = URLERR_CANTOPEN;
	    errno = ENOENT;
	    return NULL;
	}

	if(strncmp(buff, "220 ", 4) != 0)
	{
	    url_ftp_close((URL)url);
	    url_errno = URLERR_CANTOPEN;
	    errno = ENOENT;
	    return NULL;
	}

	/* login */
	sprintf(buff, "USER %s\r\n", user);
	if(ftp_cmd(url, buff, "331") < 0)
	    return NULL;

	/* password */
	if(passwd == NULL)
	    sprintf(buff, "PASS Unknown@liburl.a\r\n");
	else
	    sprintf(buff, "PASS %s\r\n", passwd);
	if(ftp_cmd(url, buff, "230") < 0)
	    return NULL;

	/* CWD */
	if(path[1] == '0')
	    /* Here is root */;
	else
	{
	    path++; /* skip '/' */
	    while((p = strchr(path, '/')) != NULL)
	    {
		*p = '\0';
		sprintf(buff, "CWD %s\r\n", path);
		if(ftp_cmd(url, buff, "250") < 0)
		    return NULL;
		path = p + 1;
	    }
	    if(!*path)
	    {
		url_ftp_close((URL)url);
		url_errno = URLERR_IURLF;
		errno = ENOENT;
		return NULL;
	    }
	}

	/* TYPE I */
	strcpy(buff, "TYPE I\r\n");
	if(ftp_cmd(url, buff, "200") < 0)
	    return NULL;

	/* PASV */
	strcpy(buff, "PASV\r\n");
	if(ftp_cmd(url, buff, "227") < 0)
	    return NULL;

	/* Parse PASV
	 * 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)
	 */
	p = buff + 4;

	while(*p && (*p < '0' || *p > '9'))
	    p++;
	if(*p == '\0')
	{
	    url_ftp_close((URL)url);
	    url_errno = URLERR_CANTOPEN;
	    errno = ENOENT;
	    return NULL;
	}
	host = p;
	n = 0; /* number of commas */
	while(n < 4)
	{
	    if((p = strchr(p, ',')) == NULL)
	    {
		url_ftp_close((URL)url);
		url_errno = URLERR_CANTOPEN;
		errno = ENOENT;
		return NULL;
	    }
	    *p = '.';
	    n++;
	}
	*p++ = '\0';

	port = atoi(p) * 256;
	if((p = strchr(p, ',')) == NULL)
	{
	    url_ftp_close((URL)url);
	    url_errno = URLERR_CANTOPEN;
	    errno = ENOENT;
	    return NULL;
	}
	port += atoi(p + 1);

	/* RETR */
	socket_fwrite("RETR ", 5, url->ctlofp);
	socket_fwrite(path, (long)strlen(path), url->ctlofp);
	socket_fwrite("\r\n", 2, url->ctlofp);
	socket_fflush(url->ctlofp);

#ifdef DEBUG
	printf("FTP>RETR %s\r\n", path);
#endif /* DEBUG */
    }

    /* Open data connection. */
#ifdef DEBUG
    printf("open(host=`%s', port=`%d')\n", host, port);
#endif /* DEBUG */

    if((fd = open_socket(host, port)) < 0)
    {
	url_ftp_close((URL)url);
	if(errno)
	    url_errno = errno;
	else
	    url_errno = errno = ENOENT;
	return NULL;
    }
    if((url->datafp = socket_fdopen(fd, "rb")) == NULL)
    {
	url_errno = errno;
	closesocket(fd);
	url_ftp_close((URL)url);
	errno = url_errno;
	return NULL;
    }

    if(url_ftp_proxy_host != NULL)
    {
	/* proxy */
	sprintf(buff, "GET %s HTTP/1.0\r\n", name);
	socket_write(fd, buff, (long)strlen(buff));
#ifdef DEBUG
	printf("FTP<%s", buff);
#endif /* DEBUG */

	if(url_user_agent)
	{
	    sprintf(buff, "User-Agent: %s\r\n", url_user_agent);
	    socket_write(fd, buff, (long)strlen(buff));
#ifdef DEBUG
	    printf("FTP<%s", buff);
#endif /* DEBUG */
	}
	socket_write(fd, "\r\n", 2);
	errno = 0;
	if(socket_fgets(buff, BUFSIZ, url->datafp) == NULL)
	{
	    if(errno == 0)
		errno = ENOENT;
	    url_errno = errno;
	    url_ftp_close((URL)url);
	    return NULL;
	}
#ifdef DEBUG
	printf("FTP>%s", buff);
#endif /* DEBUG */

	p = buff;
	if(strncmp(p, "HTTP/1.0 ", 9) == 0 || strncmp(p, "HTTP/1.1 ", 9) == 0)
	    p += 9;
	if(strncmp(p, "200", 3) != 0) /* Not success */
	{
	    url_ftp_close((URL)url);
	    url_errno = errno = guess_errno(buff);
	    return NULL;
	}

	/* Skip mime header */
	while(socket_fgets(buff, BUFSIZ, url->datafp) != NULL)
	{
	    if(buff[0] == '\n' || (buff[0] == '\r' && buff[1] == '\n'))
		break; /* end of heaer */
#ifdef DEBUG
	    printf("FTP>%s", buff);
#endif /* DEBUG */
	}
    }
    else
    {
	/* not proxy */
	if(socket_fgets(buff, BUFSIZ, url->ctlifp) == NULL)
	{
	    url_ftp_close((URL)url);
	    url_errno = errno;
	    return NULL;
	}

#ifdef DEBUG
	printf("FTP<%s", buff);
#endif /* DEBUG */

	if(strncmp(buff, "150", 3) != 0)
	{
	    url_ftp_close((URL)url);
	    url_errno = errno = guess_errno(buff);
	    return NULL;
	}
	url->abor = 1;
    }

#ifdef __W32__
    return url_buff_open((URL)url, 1);
#else
    return (URL)url;
#endif /* __W32__ */
}

static long url_ftp_read(URL url, void *buff, long n)
{
    URL_ftp *urlp = (URL_ftp *)url;

    n = socket_fread(buff, n, urlp->datafp);
    if(n <= 0)
	urlp->abor = 0;
    return n;
}

static char *url_ftp_gets(URL url, char *buff, int n)
{
    URL_ftp *urlp = (URL_ftp *)url;

    buff = socket_fgets(buff, n, urlp->datafp);
    if(buff == NULL)
	urlp->abor = 0;
    return buff;
}

static int url_ftp_fgetc(URL url)
{
    URL_ftp *urlp = (URL_ftp *)url;
    int n;
    unsigned char c;

    n = socket_fread(&c, 1, urlp->datafp);
    if(n <= 0)
    {
	urlp->abor = 0;
	if(errno)
	    url_errno = errno;
	return EOF;
    }
    return (int)c;
}

static void url_ftp_close(URL url)
{
    URL_ftp *urlp = (URL_ftp *)url;
    int save_errno = errno;

    if(urlp->datafp != NULL)
	socket_fclose(urlp->datafp);
    else
	urlp->abor = 0;
    if(urlp->ctlofp != NULL)
    {
	if(urlp->abor)
	    socket_fwrite("ABOR\r\n", 6, urlp->ctlofp);
	socket_fwrite("QUIT\r\n", 6, urlp->ctlofp);
	socket_fflush(urlp->ctlofp);
	socket_fclose(urlp->ctlofp);
    }
    if(urlp->ctlifp != NULL)
	socket_fclose(urlp->ctlifp);
    free(url);
    errno = save_errno;
}
