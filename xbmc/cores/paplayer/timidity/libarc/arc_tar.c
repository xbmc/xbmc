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
#include <ctype.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "timidity.h"
#include "mblock.h"
#include "zip.h"
#include "arc.h"

#define TARBLKSIZ 512
#define TARHDRSIZ 512

static long octal_value(char *s, int len);
static int tar_checksum(char *hdr);

ArchiveEntryNode *next_tar_entry(void)
{
    char hdr[TARHDRSIZ];
    long size, sizeb;
    ArchiveEntryNode *entry;
    URL url;
    int flen;
    int macbin_check;

    url = arc_handler.url;
    macbin_check = (arc_handler.counter == 0);

  retry_read:
    if(!macbin_check)
      {
	if(url_read(url, hdr, TARHDRSIZ) != TARHDRSIZ)
	  return NULL;
      }
    else
      {
	int c = url_getc(url);
	if(c == 0)
	  {
	    url_skip(url, 127);
	    if(arc_handler.isfile)
	      arc_handler.pos += 128;
	    if(url_read(url, hdr, TARHDRSIZ) != TARHDRSIZ)
	      return NULL;
	  }
	else
	  {
	    hdr[0] = c;
	    if(url_read(url, hdr+1, TARHDRSIZ-1) != TARHDRSIZ-1)
	      return NULL;
	  }
      }

    macbin_check = 0;

    if(hdr[0] == '\0')
      return NULL;
    if(!tar_checksum(hdr))
      return NULL;
    size = octal_value(hdr + 124, 12);
    flen = strlen(hdr);
    if(size == 0 && flen > 0 && hdr[flen - 1] == '/')
    {
	if(arc_handler.isfile)
	    arc_handler.pos += TARHDRSIZ;
	goto retry_read;
    }

    entry = new_entry_node(hdr, flen);
    if(entry == NULL)
	return NULL;
    sizeb = (((size) + (TARBLKSIZ-1)) & ~(TARBLKSIZ-1));

    if(arc_handler.isfile)
    {
	arc_handler.pos += TARHDRSIZ;
	entry->comptype = ARCHIVEC_STORED;
	entry->compsize = entry->origsize = size;
	entry->start = arc_handler.pos;
	url_skip(url, sizeb);
	arc_handler.pos += sizeb;
    }
    else
    {
	void *data;
	long n;

	data = url_dump(url, size, &n);
	if(size != n)
	{
	    if(data != NULL)
		free(data);
	    free_entry_node(entry);
	    return NULL;
	}
	entry->cache = arc_compress(data, size, ARC_DEFLATE_LEVEL,
				    &entry->compsize);
	free(data);
	entry->comptype = ARCHIVEC_DEFLATED;
	entry->origsize = size;
	entry->start = 0;
	url_skip(url, sizeb - size);
    }

    return entry;
}

static long octal_value(char *s, int len)
{
    long val;

    while(len > 0 && !isdigit((int)(unsigned char)*s))
    {
	s++;
	len--;
    }

    val = 0;
    while(len > 0 && isdigit((int)(unsigned char)*s))
    {
	val = ((val<<3) | (*s - '0'));
	s++;
	len--;
    }
    return val;
}

static int tar_checksum(char *hdr)
{
    int i;

    long recorded_sum;
    long unsigned_sum;		/* the POSIX one :-) */
    long signed_sum;		/* the Sun one :-( */

    recorded_sum = octal_value(hdr + 148, 8);
    unsigned_sum = 0;
    signed_sum = 0;
    for(i = 0; i < TARBLKSIZ; i++)
    {
	unsigned_sum += 0xFF & hdr[i];
	signed_sum   += hdr[i];
    }

    /* Adjust checksum to count the "chksum" field as blanks.  */
    for(i = 0; i < 8; i++)
    {
	unsigned_sum -= 0xFF & hdr[148 + i];
	signed_sum   -= hdr[i];
    }
    unsigned_sum += ' ' * 8;
    signed_sum   += ' ' * 8;

    return unsigned_sum == recorded_sum || signed_sum == recorded_sum;
}
