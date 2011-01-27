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

#define DECODEBUFSIZ BUFSIZ

typedef struct _URL_qsdecode
{
    char common[sizeof(struct _URL)];
    URL reader;
    long rpos;
    int beg, end, eof, eod;
    unsigned char decodebuf[DECODEBUFSIZ];
    int autoclose;
} URL_qsdecode;

static long url_qsdecode_read(URL url, void *buff, long n);
static int  url_qsdecode_fgetc(URL url);
static long url_qsdecode_tell(URL url);
static void url_qsdecode_close(URL url);

URL url_qsdecode_open(URL reader, int autoclose)
{
    URL_qsdecode *url;

    url = (URL_qsdecode *)alloc_url(sizeof(URL_qsdecode));
    if(url == NULL)
    {
	if(autoclose)
	    url_close(reader);
	url_errno = errno;
	return NULL;
    }

    /* common members */
    URLm(url, type)      = URL_qsdecode_t;
    URLm(url, url_read)  = url_qsdecode_read;
    URLm(url, url_gets)  = NULL;
    URLm(url, url_fgetc) = url_qsdecode_fgetc;
    URLm(url, url_seek)  = NULL;
    URLm(url, url_tell)  = url_qsdecode_tell;
    URLm(url, url_close) = url_qsdecode_close;

    /* private members */
    url->reader = reader;
    url->rpos = 0;
    url->beg = 0;
    url->end = 0;
    url->eof = url->eod = 0;
    memset(url->decodebuf, 0, sizeof(url->decodebuf));
    url->autoclose = autoclose;

    return (URL)url;
}

static int qsdecode(URL_qsdecode *urlp)
{
    int n;
    unsigned char *p;
    URL url;

    if(urlp->eod)
    {
	urlp->eof = 1;
	return 1;
    }

    p = urlp->decodebuf;
    url = urlp->reader;
    n = 0;
    while(n < DECODEBUFSIZ)
    {
	int c1, c2;

	if((c1 = url_getc(url)) == EOF)
	    break;
	if(c1 != '=')
	{
	    p[n++] = c1;
	    continue;
	}

	/* quoted character */
      next_quote:
	if((c1 = url_getc(url)) == EOF)
	    break;
	if(c1 == '\n')
	    continue;
	if(c1 == '\r')
	{
	    if((c1 = url_getc(url)) == EOF)
		break;
	    if(c1 == '\n')
		continue;
	    if(c1 == '=')
		goto next_quote;
	    p[n++] = c1;
	    continue;
	}
	if((c2 = url_getc(url)) == EOF)
	    break;
	if('0' <= c1 && c1 <= '9')
	    c1 -= '0';
	else if('A' <= c1 && c1 <= 'F')
	    c1 -= 'A' - 10;
	else
	    c1 = 0;
	if('0' <= c2 && c2 <= '9')
	    c2 -= '0';
	else if('A' <= c2 && c2 <= 'F')
	    c2 -= 'A' - 10;
	else
	    c2 = 0;
	p[n++] = (c1 << 4 | c2);
    }

    urlp->rpos += urlp->beg;
    urlp->beg = 0;
    urlp->end = n;

    if(n < DECODEBUFSIZ)
	urlp->eod = 1;

    if(n == 0)
    {
	urlp->eof = 1;
	return 1;
    }


    return 0;
}

static long url_qsdecode_read(URL url, void *buff, long size)
{
    URL_qsdecode *urlp = (URL_qsdecode *)url;
    unsigned char *p = (unsigned char *)buff;
    long n;

    if(urlp->eof)
	return 0;

    n = 0;
    while(n < size)
    {
	int i;

	if(urlp->beg == urlp->end)
	    if(qsdecode(urlp))
		break;
	i = urlp->end - urlp->beg;
	if(i > size - n)
	    i = size - n;
	memcpy(p + n, urlp->decodebuf + urlp->beg, i);
	n += i;
	urlp->beg += i;
    }
    return n;
}

static int url_qsdecode_fgetc(URL url)
{
    URL_qsdecode *urlp = (URL_qsdecode *)url;

    if(urlp->eof)
	return EOF;
    if(urlp->beg == urlp->end)
	if(qsdecode(urlp))
	    return EOF;

    return (int)urlp->decodebuf[urlp->beg++];
}

static long url_qsdecode_tell(URL url)
{
    URL_qsdecode *urlp = (URL_qsdecode *)url;

    return urlp->rpos + urlp->beg;
}

static void url_qsdecode_close(URL url)
{
    URL_qsdecode *urlp = (URL_qsdecode *)url;

    if(urlp->autoclose)
	url_close(urlp->reader);
    free(url);
}

#ifdef QSDECODE_MAIN
void main(int argc, char** argv)
{
    URL qsdecoder;
    char buff[256], *filename;
    int c;

    if(argc != 2)
    {
	fprintf(stderr, "Usage: %s qs-filename\n", argv[0]);
	exit(1);
    }
    filename = argv[1];

    if((qsdecoder = url_file_open(filename)) == NULL)
    {
	perror(argv[1]);
	url_close(qsdecoder);
	exit(1);
    }

    qsdecoder = url_qsdecode_open(qsdecoder, 1);
#if QSDECODE_MAIN
    while((c = url_getc(qsdecoder)) != EOF)
	putchar(c);
#else
    while((c = url_read(qsdecoder, buff, sizeof(buff))) > 0)
	write(1, buff, c);
#endif
    url_close(qsdecoder);
    exit(0);
}

#endif /* QSDECODE_MAIN */
