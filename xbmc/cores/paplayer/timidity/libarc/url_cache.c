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

#include "timidity.h"
#include "url.h"
#include "memb.h"

typedef struct _URL_cache
{
    char common[sizeof(struct _URL)];
    URL reader;
    int memb_ok;
    MemBuffer b;
    long pos;
    int autoclose;
} URL_cache;

static long url_cache_read(URL url, void *buff, long n);
static int url_cache_fgetc(URL url);
static long url_cache_seek(URL url, long offset, int whence);
static long url_cache_tell(URL url);
static void url_cache_close(URL url);

URL url_cache_open(URL url, int autoclose)
{
    URL_cache *urlp;

    if(url->type == URL_cache_t && autoclose)
    {
	if(((URL_cache *)url)->memb_ok)
	    delete_memb(&((URL_cache *)url)->b);
	urlp = (URL_cache *)url;
	url = urlp->reader;
    }
    else
    {
	if((urlp = (URL_cache *)alloc_url(sizeof(URL_cache))) == NULL)
	{
	    if(autoclose)
		url_close(url);
	    return NULL;
	}
    }

    /* common members */
    URLm(urlp, type)	  = URL_cache_t;
    URLm(urlp, url_read)  = url_cache_read;
    URLm(urlp, url_gets)  = NULL;
    URLm(urlp, url_fgetc) = url_cache_fgetc;
    URLm(urlp, url_seek)  = url_cache_seek;
    URLm(urlp, url_tell)  = url_cache_tell;
    URLm(urlp, url_close) = url_cache_close;

    /* private members */
    urlp->reader = url;
    urlp->memb_ok = 1;
    init_memb(&urlp->b);
    urlp->pos = 0;
    urlp->autoclose = autoclose;

    return (URL)urlp;
}

void url_cache_disable(URL url)
{
    if(url->type == URL_cache_t)
	url->url_seek = NULL;
}

void url_cache_detach(URL url)
{
    if(url != NULL && url->type == URL_cache_t)
    {
	URL_cache *urlp = (URL_cache *)url;
	if(urlp->autoclose && urlp->reader != NULL)
	    url_close(urlp->reader);
	urlp->reader = NULL;
    }
}

static long url_cache_read(URL url, void *buff, long n)
{
    URL_cache *urlp = (URL_cache *)url;
    MemBuffer *b = &urlp->b;

    if(!urlp->memb_ok)
    {
	if(urlp->reader == NULL)
	    return 0;
	n = url_read(urlp->reader, buff, n);
	if(n > 0)
	    urlp->pos += n;
	return n;
    }

    if(urlp->pos < b->total_size)
    {
	if(n > b->total_size - urlp->pos)
	    n = b->total_size - urlp->pos;
	urlp->pos += read_memb(b, buff, n);
	return n;
    }

    if(url->url_seek == NULL)
    {
	delete_memb(b);
	urlp->memb_ok = 0;
	if(urlp->reader == NULL)
	    return 0;
	n = url_read(urlp->reader, buff, n);
	if(n > 0)
	    urlp->pos += n;
	return n;
    }

    if(urlp->reader == NULL)
	return 0;
    n = url_read(urlp->reader, buff, n);
    if(n > 0)
    {
	push_memb(b, buff, n);
	b->cur = b->tail;
	b->cur->pos = b->cur->size;
	urlp->pos += n;
    }
    return n;
}

static int url_cache_fgetc(URL url)
{
    URL_cache *urlp = (URL_cache *)url;
    MemBuffer *b = &urlp->b;
    char c;
    int i;

    if(!urlp->memb_ok)
    {
	if(urlp->reader == NULL)
	    return EOF;
	if((i = url_getc(urlp->reader)) == EOF)
	    return EOF;
	urlp->pos++;
	return i;
    }

    if(urlp->pos < b->total_size)
    {
	read_memb(b, &c, 1);
	urlp->pos++;
	return (int)(unsigned char)c;
    }

    if(url->url_seek == NULL)
    {
	delete_memb(b);
	urlp->memb_ok = 0;
	if(urlp->reader == NULL)
	    return EOF;
	if((i = url_getc(urlp->reader)) == EOF)
	    return EOF;
	urlp->pos++;
	return i;
    }

    if(urlp->reader == NULL)
	return EOF;
    if((i = url_getc(urlp->reader)) == EOF)
	return EOF;
    c = (char)i;
    push_memb(b, &c, 1);
    b->cur = b->tail;
    b->cur->pos = b->cur->size;
    urlp->pos++;
    return i;
}

static long url_cache_seek(URL url, long offset, int whence)
{
    URL_cache *urlp = (URL_cache *)url;
    MemBuffer *b = &urlp->b;
    long ret, newpos, n, s;

    ret = urlp->pos;
    switch(whence)
    {
      case SEEK_SET:
	newpos = offset;
	break;
      case SEEK_CUR:
	newpos = ret + offset;
	break;
      case SEEK_END:
	while(url_cache_fgetc(url) != EOF)
	    ;
	newpos = b->total_size + whence;
	break;
      default:
	url_errno = errno = EPERM;
	return -1;
    }
    if(newpos < 0)
	newpos = 0;
    n = newpos - ret;

    if(n < 0)
    {
	rewind_memb(b);
	n = newpos;
	urlp->pos = 0;
    }

    s = skip_read_memb(b, n);
    urlp->pos += s;
    while(s++ < n && url_cache_fgetc(url) != EOF)
	;
    return ret;
}

static long url_cache_tell(URL url)
{
    return ((URL_cache *)url)->pos;
}

static void url_cache_close(URL url)
{
    URL_cache *urlp = (URL_cache *)url;
    if(urlp->autoclose && urlp->reader != NULL)
	url_close(urlp->reader);
    if(urlp->memb_ok)
	delete_memb(&urlp->b);
    free(urlp);
}
