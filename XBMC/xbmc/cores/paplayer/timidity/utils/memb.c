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
#include "memb.h"

void init_memb(MemBuffer *b)
{
    memset(b, 0, sizeof(MemBuffer));
}

void delete_memb(MemBuffer *b)
{
    reuse_mblock(&b->pool);
    memset(b, 0, sizeof(MemBuffer));
}

void rewind_memb(MemBuffer *b)
{
    if(b->head != NULL)
    {
	b->cur = b->head;
	b->cur->pos = 0;
    }
}

void push_memb(MemBuffer *b, char *buff, long buff_size)
{
    b->total_size += buff_size;
    if(b->head == NULL)
    {
	b->head = b->tail = b->cur =
	    (MemBufferNode *)new_segment(&b->pool, MIN_MBLOCK_SIZE);
	b->head->next = NULL;
	b->head->size = b->head->pos = 0;
    }
    while(buff_size > 0)
    {
	long n;
	MemBufferNode *p;

	p = b->tail;
	n = (long)(MEMBASESIZE - p->size);
	if(n == 0)
	{
	    p = (MemBufferNode *)new_segment(&b->pool, MIN_MBLOCK_SIZE);
	    b->tail->next = p;
	    b->tail = p;
	    p->next = NULL;
	    p->size = p->pos = 0;
	    n = MEMBASESIZE;
	}
	if(n > buff_size)
	    n = buff_size;
	memcpy(p->base + p->size, buff, n);
	p->size += n;
	buff_size -= n;
	buff += n;
    }
}

long read_memb(MemBuffer *b, char *buff, long buff_size)
{
    long n;

    if(b->head == NULL)
	return 0;
    if(b->cur == NULL)
	rewind_memb(b);
    if(b->cur->next == NULL && b->cur->pos == b->cur->size)
	return 0;

    n = 0;
    while(n < buff_size)
    {
	long i;
	MemBufferNode *p;

	p = b->cur;
	if(p->pos == p->size)
	{
	    if(p->next == NULL)
		break;
	    b->cur = p->next;
	    b->cur->pos = 0;
	    continue;
	}

	i = p->size - p->pos;
	if(i > buff_size - n)
	    i = buff_size - n;
	memcpy(buff + n, p->base + p->pos, i);
	n += i;
	p->pos += i;
    }
    return n;
}

long skip_read_memb(MemBuffer *b, long size)
{
    long n;

    if(size <= 0 || b->head == NULL)
	return 0;
    if(b->cur == NULL)
	rewind_memb(b);
    if(b->cur->next == NULL && b->cur->pos == b->cur->size)
	return 0;

    n = 0;
    while(n < size)
    {
	long i;
	MemBufferNode *p;

	p = b->cur;
	if(p->pos == p->size)
	{
	    if(p->next == NULL)
		break;
	    b->cur = p->next;
	    b->cur->pos = 0;
	    continue;
	}

	i = p->size - p->pos;
	if(i > size - n)
	    i = size - n;
	n += i;
	p->pos += i;
    }
    return n;
}

typedef struct _URL_memb
{
    char common[sizeof(struct _URL)];
    MemBuffer *b;
    long pos;
    int autodelete;
} URL_memb;

static long url_memb_read(URL url, void *buff, long n);
static int url_memb_fgetc(URL url);
static long url_memb_seek(URL url, long offset, int whence);
static long url_memb_tell(URL url);
static void url_memb_close(URL url);

URL memb_open_stream(MemBuffer *b, int autodelete)
{
    URL_memb *url;

    url = (URL_memb *)alloc_url(sizeof(URL_memb));
    if(url == NULL)
    {
	if(autodelete)
	    delete_memb(b);
	url_errno = errno;
	return NULL;
    }

    /* common members */
    URLm(url, type)      = URL_extension_t;
    URLm(url, url_read)  = url_memb_read;
    URLm(url, url_gets)  = NULL;
    URLm(url, url_fgetc) = url_memb_fgetc;
    URLm(url, url_seek)  = url_memb_seek;
    URLm(url, url_tell)  = url_memb_tell;
    URLm(url, url_close) = url_memb_close;

    /* private members */
    url->b = b;
    url->pos = 0;
    url->autodelete = autodelete;

    rewind_memb(b);
    return (URL)url;
}

static long url_memb_read(URL url, void *buff, long n)
{
    URL_memb *urlp = (URL_memb *)url;
    if((n = read_memb(urlp->b, buff, n)) > 0)
	urlp->pos += n;
    return n;
}

static int url_memb_fgetc(URL url)
{
    URL_memb *urlp = (URL_memb *)url;
    MemBuffer *b = urlp->b;
    MemBufferNode *p;

    p = b->cur;
    if(p == NULL)
	return EOF;
    while(p->pos == p->size)
    {
	if(p->next == NULL)
	    return EOF;
	p = b->cur = p->next;
	p->pos = 0;
    }
    urlp->pos++;
    return (int)((unsigned char *)p->base)[p->pos++];
}

static long url_memb_seek(URL url, long offset, int whence)
{
    URL_memb *urlp = (URL_memb *)url;
    MemBuffer *b = urlp->b;
    long ret, newpos = 0, n;

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
	newpos = b->total_size + offset;
	break;
    }
    if(newpos < 0)
	newpos = 0;
    else if(newpos > b->total_size)
	newpos = b->total_size;
    n = newpos - ret;
    if(n < 0)
    {
	rewind_memb(b);
	n = newpos;
	urlp->pos = 0;
    }

    urlp->pos += skip_read_memb(b, n);
    return ret;
}

static long url_memb_tell(URL url)
{
    return ((URL_memb *)url)->pos;
}

static void url_memb_close(URL url)
{
    URL_memb *urlp = (URL_memb *)url;
    if(urlp->autodelete)
    {
	delete_memb(urlp->b);
	free(urlp->b);
    }
    free(url);
}
