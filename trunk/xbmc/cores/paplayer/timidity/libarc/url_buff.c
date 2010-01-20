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
#include "url.h"

#define URL_BUFF_SIZE (8*1024)
#define BASESIZE (URL_BUFF_SIZE * 2)
#define BASEMASK (BASESIZE-1)

typedef struct _URL_buff
{
    char common[sizeof(struct _URL)];
    URL reader;
    unsigned char buffer[BASESIZE + URL_BUFF_SIZE]; /* ring buffer */
    int wp;			/* write pointer to the buffer */
    int rp;			/* read pointer from the buffer */
    long pos, posofs;		/* position */
    int weof;
    int eof;
    int autoclose;
} URL_buff;

static long url_buff_read(URL url, void *buff, long n);
static char *url_buff_gets(URL url, char *buff, int n);
static int url_buff_fgetc(URL url);
static long url_buff_seek(URL url, long offset, int whence);
static long url_buff_tell(URL url);
static void url_buff_close(URL url);

URL url_buff_open(URL url, int autoclose)
{
    URL_buff *urlp;

    if((urlp = (URL_buff *)alloc_url(sizeof(URL_buff))) == NULL)
    {
	if(autoclose)
	    url_close(url);
	return NULL;
    }

    /* common members */
    URLm(urlp, type)	  = URL_buff_t;
    URLm(urlp, url_read)  = url_buff_read;
    URLm(urlp, url_gets)  = url_buff_gets;
    URLm(urlp, url_fgetc) = url_buff_fgetc;
    URLm(urlp, url_seek)  = url_buff_seek;
    URLm(urlp, url_tell)  = url_buff_tell;
    URLm(urlp, url_close) = url_buff_close;

    /* private members */
    urlp->reader = url;
    memset(urlp->buffer, 0, sizeof(urlp->buffer));
    urlp->wp = 0;
    urlp->rp = 0;
    if((urlp->posofs = url_tell(url)) == -1)
	urlp->posofs = 0;
    urlp->pos = 0;
    urlp->eof = 0;
    urlp->autoclose = autoclose;

    return (URL)urlp;
}

static void prefetch(URL_buff *urlp)
{
    long i, n;

    n = url_safe_read(urlp->reader, urlp->buffer + urlp->wp, URL_BUFF_SIZE);
    if(n <= 0)
	return;
    urlp->wp += n;
    if(urlp->wp < BASESIZE)
	return;
    if(urlp->wp == BASESIZE)
    {
	urlp->wp = 0;
	return;
    }

    /* urlp->wp > BASESIZE */
    i = urlp->wp - BASESIZE;
    memcpy(urlp->buffer, urlp->buffer + BASESIZE, i);
    urlp->wp = i;
}

static int url_buff_fgetc(URL url)
{
    URL_buff *urlp = (URL_buff *)url;
    int c, r;

    if(urlp->eof)
	return EOF;

    r = urlp->rp;
    if(r == urlp->wp)
    {
	prefetch(urlp);
	if(r == urlp->wp)
	{
	    urlp->eof = 1;
	    return EOF;
	}
    }
    c = urlp->buffer[r];
    urlp->rp = ((r + 1) & BASEMASK);
    urlp->pos++;
    return c;
}

static long url_buff_read(URL url, void *buff, long n)
{
    URL_buff *urlp = (URL_buff *)url;
    char *s = (char *)buff;
    int r, i, j;

    if(urlp->eof)
	return 0;

    r = urlp->rp;
    if(r == urlp->wp)
    {
	prefetch(urlp);
	if(r == urlp->wp)
	{
	    urlp->eof = 1;
	    return EOF;
	}
    }

    /* first fragment */
    i = urlp->wp - r;
    if(i < 0)
	i = BASESIZE - r;
    if(i > n)
	i = n;
    memcpy(s, urlp->buffer + r, i);
    r = ((r + i) & BASEMASK);

    if(i == n || r == urlp->wp || r != 0)
    {
	urlp->rp = r;
	urlp->pos += i;
	return i;
    }

    /* second fragment */
    j = urlp->wp;
    n -= i;
    s += i;
    if(j > n)
	j = n;
    memcpy(s, urlp->buffer, j);
    urlp->rp = j;
    urlp->pos += i + j;

    return i + j;
}

static long url_buff_tell(URL url)
{
    URL_buff *urlp = (URL_buff *)url;

    return urlp->pos + urlp->posofs;
}

static char *url_buff_gets(URL url, char *buff, int maxsiz)
{
    URL_buff *urlp = (URL_buff *)url;
    int c, r, w;
    long len, maxlen;
    int newline = url_newline_code;
    unsigned char *bp;

    if(urlp->eof)
	return NULL;

    maxlen = maxsiz - 1;
    if(maxlen == 0)
	*buff = '\0';
    if(maxlen <= 0)
	return buff;
    len = 0;
    r = urlp->rp;
    w = urlp->wp;
    bp = urlp->buffer;

    do
    {
	if(r == w)
	{
	    urlp->wp = w;
	    prefetch(urlp);
	    w = urlp->wp;
	    if(r == w)
	    {
		urlp->eof = 1;
		if(len == 0)
		    return NULL;
		buff[len] = '\0';
		urlp->pos += len;
		urlp->rp = r;
		return buff;
	    }
	}
	c = bp[r];
	buff[len++] = c;
	r = ((r + 1) & BASEMASK);
    } while(c != newline && len < maxlen);
    buff[len] = '\0';
    urlp->pos += len;
    urlp->rp = r;
    return buff;
}

static long url_buff_seek(URL url, long offset, int whence)
{
    URL_buff *urlp = (URL_buff *)url;
    long ret, diff, n;
    int r, w, filled, i;

    ret = urlp->pos + urlp->posofs;
    switch(whence)
    {
      case SEEK_SET:
	diff = offset - ret;
	break;
      case SEEK_CUR:
	diff = offset;
	break;
      case SEEK_END:
	if(!urlp->eof)
	    while(url_buff_fgetc(url) != EOF)
		;
	diff = offset;
	break;
      default:
	url_errno = errno = EPERM;
	return -1;
    }

    if(diff == 0)
    {
	urlp->eof = 0; /* To be more read */
	return ret;
    }

    n = 0;			/* number of bytes to move */
    r = urlp->rp;		/* read pointer */
    w = urlp->wp;		/* write pointer */

    if(diff > 0)
    {
	while(diff > 0)
	{
	    if(r == w)
	    {
		urlp->wp = w;
		prefetch(urlp);
		w = urlp->wp;
		if(r == w)
		{
		    urlp->eof = 1;
		    urlp->pos += n;
		    urlp->rp = r;
		    return ret;
		}
	    }

	    i = w - r;
	    if(i < 0)
		i = BASESIZE - r;
	    if(i > diff)
		i = diff;
	    n += i;
	    diff -= i;
	    r = ((r + i) & BASEMASK);
	}
	urlp->pos += n;
	urlp->rp = r;
	urlp->eof = 0; /* To be more read */
	return ret;
    }

    /* diff < 0 */

    diff = -diff;
    filled = r - w;
    if(filled <= 0)
	filled = BASEMASK + filled;
    filled--;
    if(filled > urlp->pos)
	filled = urlp->pos;

    if(filled < diff)
    {
	url_errno = errno = EPERM;
	return -1;
    }

    /* back `rp' by `diff' */
    r -= diff;
    if(r < 0)
	r += BASESIZE;
    urlp->rp = r;
    urlp->pos -= diff;
    urlp->eof = 0; /* To be more read */
    return ret;
}

static void url_buff_close(URL url)
{
    URL_buff *urlp = (URL_buff *)url;
    if(urlp->autoclose)
	url_close(urlp->reader);
    free(url);
}
