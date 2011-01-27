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

typedef struct _URL_uudecode
{
    char common[sizeof(struct _URL)];
    URL reader;
    long rpos;
    int beg, end, eof;
    unsigned char decodebuf[128];
    int autoclose;
} URL_uudecode;

static long url_uudecode_read(URL url, void *buff, long n);
static int  url_uudecode_fgetc(URL url);
static long url_uudecode_tell(URL url);
static void url_uudecode_close(URL url);

URL url_uudecode_open(URL reader, int autoclose)
{
    URL_uudecode *url;

    url = (URL_uudecode *)alloc_url(sizeof(URL_uudecode));
    if(url == NULL)
    {
	if(autoclose)
	    url_close(reader);
	url_errno = errno;
	return NULL;
    }

    /* common members */
    URLm(url, type)      = URL_uudecode_t;
    URLm(url, url_read)  = url_uudecode_read;
    URLm(url, url_gets)  = NULL;
    URLm(url, url_fgetc) = url_uudecode_fgetc;
    URLm(url, url_seek)  = NULL;
    URLm(url, url_tell)  = url_uudecode_tell;
    URLm(url, url_close) = url_uudecode_close;

    /* private members */
    url->reader = reader;
    url->rpos = 0;
    url->beg = 0;
    url->end = 0;
    url->eof = 0;
    memset(url->decodebuf, 0, sizeof(url->decodebuf));
    url->autoclose = autoclose;

    return (URL)url;
}

#define	DEC(c)	(((c) - ' ') & 077)		/* single character decode */
static int uudecodeline(URL_uudecode *urlp)
{
    unsigned char inbuf[BUFSIZ], *p, *q, ch;
    int n;

    if(url_gets(urlp->reader, (char *)inbuf, sizeof(inbuf)) == NULL)
    {
	urlp->eof = 1;
	return 1;
    }

    if((n = DEC(*inbuf)) <= 0)
    {
	urlp->eof = 1;
	return 1;
    }

    if(uudecode_unquote_html)
    {
	int i, j, len;

	len = strlen((char *)inbuf);
	while(len > 0 &&
	      (inbuf[len - 1] == '\r' || inbuf[len - 1] == '\n' ||
	       inbuf[len - 1] == '\t' || inbuf[len - 1] == ' '))
	    inbuf[--len] = '\0';
	if(n * 4 != (len - 1) * 3)
	{
	    /* &lt;/&gt;/&amp; */
	    i = j = 0;
	    while(i < len - 3)
		if(inbuf[i] != '&')
		    inbuf[j++] = inbuf[i++];
		else
		{
		    i++;
		    if(strncmp((char *)inbuf + i, "lt;", 3) == 0)
		    {
			inbuf[j++] = '<';
			i += 3;
		    }
		    else if(strncmp((char *)inbuf + i, "gt;", 3) == 0)
		    {
			inbuf[j++] = '>';
			i += 3;
		    }
		    else if(strncmp((char *)inbuf + i, "amp;", 4) == 0)
		    {
			inbuf[j++] = '&';
			i += 4;
		    }
		    else
			inbuf[j++] = '&';
		}
	    while(i < len)
		inbuf[j++] = inbuf[i++];
	    inbuf[j++] = '\0';
	}
    }

    p = inbuf + 1;
    q = urlp->decodebuf;
    for(; n > 0; p += 4, n -= 3)
    {
	if(n >= 3)
	{
	    ch = DEC(p[0]) << 2 | DEC(p[1]) >> 4;
	    *q++ = ch;
	    ch = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
	    *q++ = ch;
	    ch = DEC(p[2]) << 6 | DEC(p[3]);
	    *q++ = ch;
	}
	else
	{
	    if(n >= 1)
	    {
		ch = DEC(p[0]) << 2 | DEC(p[1]) >> 4;
		*q++ = ch;
	    }
	    if(n >= 2)
	    {
		ch = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
		*q++ = ch;
	    }
	    if(n >= 3)
	    {
		ch = DEC(p[2]) << 6 | DEC(p[3]);
		*q++ = ch;
	    }
	}
    }
    urlp->rpos += urlp->beg;
    urlp->beg = 0;
    urlp->end = q - urlp->decodebuf;
    return 0;
}

static long url_uudecode_read(URL url, void *buff, long size)
{
    URL_uudecode *urlp = (URL_uudecode *)url;
    unsigned char *p = (unsigned char *)buff;
    long n;

    if(urlp->eof)
	return 0;

    n = 0;
    while(n < size)
    {
	int i;

	if(urlp->beg == urlp->end)
	    if(uudecodeline(urlp))
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

static int url_uudecode_fgetc(URL url)
{
    URL_uudecode *urlp = (URL_uudecode *)url;

    if(urlp->eof)
	return EOF;
    if(urlp->beg == urlp->end)
	if(uudecodeline(urlp))
	    return EOF;

    return (int)urlp->decodebuf[urlp->beg++];
}

static long url_uudecode_tell(URL url)
{
    URL_uudecode *urlp = (URL_uudecode *)url;

    return urlp->rpos + urlp->beg;
}

static void url_uudecode_close(URL url)
{
    URL_uudecode *urlp = (URL_uudecode *)url;

    if(urlp->autoclose)
	url_close(urlp->reader);
    free(url);
}

#ifdef UUDECODE_MAIN
void main(int argc, char** argv)
{
    URL uudecoder;
    char buff[256], *filename;
    int c;

    if(argc != 2)
    {
	fprintf(stderr, "Usage: %s uu-filename\n", argv[0]);
	exit(1);
    }
    filename = argv[1];

    if((uudecoder = url_file_open(filename)) == NULL)
    {
	perror(argv[1]);
	exit(1);
    }

    for(;;)
    {
	if(url_readline(uudecoder, buff, sizeof(buff)) == EOF)
	{
	    fprintf(stderr, "%s: Not a hqx-file\n", filename);
	    url_close(uudecoder);
	    exit(1);
	}
	if(strncmp(buff, "begin ", 6) == 0)
	    break;
    }

    uudecoder = url_uudecode_open(uudecoder, 1);
#if UUDECODE_MAIN
    while((c = url_getc(uudecoder)) != EOF)
	putchar(c);
#else
    while((c = url_read(uudecoder, buff, sizeof(buff))) > 0)
	write(1, buff, c);
#endif
    url_close(uudecoder);
    exit(0);
}

#endif /* UUDECODE_MAIN */
