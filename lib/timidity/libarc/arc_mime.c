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
#include "mblock.h"
#include "zip.h"
#include "arc.h"

extern char *safe_strdup(const char *);

#ifndef MAX_CHECK_LINES
#define MAX_CHECK_LINES 1024
#endif /* MAX_CHECK_LINES */

struct StringStackElem
{
    struct StringStackElem *next;
    char str[1];		/* variable length */
};

struct StringStack
{
    struct StringStackElem *elem;
    MBlockList pool;
};

static void init_string_stack(struct StringStack *stk);
static void push_string_stack(struct StringStack *stk, char *str, int len);
static char *top_string_stack(struct StringStack *stk);
static void pop_string_stack(struct StringStack *stk);
static void delete_string_stack(struct StringStack *stk);

struct MIMEHeaderStream
{
    URL url;
    char *field;
    char *value;
    char *line;
    int bufflen;
    int eof;
    MBlockList pool;
};

static void init_mime_stream(struct MIMEHeaderStream *hdr, URL url);
static int  next_mime_header(struct MIMEHeaderStream *hdr);
static void end_mime_stream(struct MIMEHeaderStream *hdr);
static int seek_next_boundary(URL url, char *boundary, long *endpoint);
static int whole_read_line(URL url, char *buff, int bufsiz);
static void *arc_mime_decode(void *data, long size,
			     int comptype, long *newsize);

ArchiveEntryNode *next_mime_entry(void)
{
    ArchiveEntryNode *head, *tail;
    URL url;
    int part;
    struct StringStack boundary;
    struct MIMEHeaderStream hdr;
    int c;

    if(arc_handler.counter != 0)
	return NULL;

    head = tail = NULL;
    url = arc_handler.url; /* url_seek must be safety */

    init_string_stack(&boundary);
    url_rewind(url);
    c = url_getc(url);
    if(c != '\0')
	url_rewind(url);
    else
	url_skip(url, 128-1);	/* skip macbin header */

    part = 1;
    for(;;)
    {
	char *new_boundary, *encoding, *name, *filename;
	char *p;
	MBlockList pool;
	long data_start, data_end, savepoint;
	int last_check, comptype, arctype;
	void *part_data;
	long part_data_size;

	new_boundary = encoding = name = filename = NULL;
	init_mblock(&pool);
	init_mime_stream(&hdr, url);
	while(next_mime_header(&hdr))
	{
	    if(strncmp(hdr.field, "Content-", 8) != 0)
		continue;
	    if(strcmp(hdr.field + 8, "Type") == 0)
	    {
		if((p = strchr(hdr.value, ';')) == NULL)
		    continue;
		*p++ = '\0';
		while(*p == ' ')
		    p++;
		if(strncasecmp(hdr.value, "multipart/mixed", 15) == 0)
		{
		    /* Content-Type: multipart/mixed; boundary="XXXX" */
		    if(strncasecmp(p, "boundary=", 9) == 0)
		    {
			p += 9;
			if(*p == '"')
			{
			    p++;
			    new_boundary = p;
			    if((p = strchr(p, '"')) == NULL)
				continue;
			}
			else
			{
			    new_boundary = p;
			    while(*p > '"' && *p < 0x7f)
				p++;
			}

			*p = '\0';
			new_boundary = strdup_mblock(&pool, new_boundary);
		    }
		}
		else if(strcasecmp(hdr.value, "multipart/mixed") == 0)
		{
		    /* Content-Type: XXXX/YYYY; name="ZZZZ" */
		    if(strncasecmp(p, "name=\"", 6) == 0)
		    {
			p += 6;
			name = p;
			if((p = strchr(p, '"')) == NULL)
			    continue;
			*p = '\0';
			name = strdup_mblock(&pool, name);
		    }
		}
	    }
	    else if(strcmp(hdr.field + 8, "Disposition") == 0)
	    {
		if((p = strchr(hdr.value, ';')) == NULL)
		    continue;
		*p++ = '\0';
		while(*p == ' ')
		    p++;
		if((p = strstr(p, "filename=\"")) == NULL)
		    continue;
		p += 10;
		filename = p;
		if((p = strchr(p, '"')) == NULL)
		    continue;
		*p = '\0';
		filename = strdup_mblock(&pool, filename);
	    }
	    else if(strcmp(hdr.field + 8, "Transfer-Encoding") == 0)
	    {
		/* Content-Transfer-Encoding: X */
		/* X := X-uuencode, base64, quoted-printable, ... */
		encoding = strdup_mblock(&pool, hdr.value);
	    }
	}

	if(hdr.eof)
	{
	    reuse_mblock(&pool);
	    end_mime_stream(&hdr);
	    delete_string_stack(&boundary);
	    return head;
	}

	if(filename == NULL)
	    filename = name;

	if(new_boundary)
	    push_string_stack(&boundary, new_boundary, strlen(new_boundary));

	data_start = url_tell(url);
	last_check = seek_next_boundary(url, top_string_stack(&boundary),
					&data_end);

	savepoint = url_tell(url);

	/* find data type */
	comptype = -1;
	if(encoding != NULL)
	{
	    if(strcmp("base64", encoding) == 0)
		comptype = ARCHIVEC_B64;
	    else if(strcmp("quoted-printable", encoding) == 0)
		comptype = ARCHIVEC_QS;
	    else if(strcmp("X-uuencode", encoding) == 0)
	    {
		char buff[BUFSIZ];
		int i;

		comptype = ARCHIVEC_UU;
		url_seek(url, data_start, SEEK_SET);
		url_set_readlimit(url, data_end - data_start);

		/* find '^begin \d\d\d \S+' */
		for(i = 0; i < MAX_CHECK_LINES; i++)
		{
		    if(whole_read_line(url, buff, sizeof(buff)) == -1)
			break; /* ?? */
		    if(strncmp(buff, "begin ", 6) == 0)
		    {
			data_start = url_tell(url);
			p = strchr(buff + 6, ' ');
			if(p != NULL)
			    filename = strdup_mblock(&pool, p + 1);
			break;
		    }
		}
		url_set_readlimit(url, -1);
	    }
	}

	if(comptype == -1)
	{
	    char buff[BUFSIZ];
	    int i;

	    url_seek(url, data_start, SEEK_SET);
	    url_set_readlimit(url, data_end - data_start);

	    for(i = 0; i < MAX_CHECK_LINES; i++)
	    {
		if(whole_read_line(url, buff, sizeof(buff)) == -1)
		    break; /* ?? */
		if(strncmp(buff, "begin ", 6) == 0)
		{
		    comptype = ARCHIVEC_UU;
		    data_start = url_tell(url);
		    p = strchr(buff + 6, ' ');
		    if(p != NULL)
			filename = strdup_mblock(&pool, p + 1);
		    break;
		}
		else if((strncmp(buff, "(This file", 10) == 0) ||
			(strncmp(buff, "(Convert with", 13) == 0))
		{
		    int c;
		    while((c = url_getc(url)) != EOF)
		    {
			if(c == ':')
			{
			    comptype = ARCHIVEC_HQX;
			    data_start = url_tell(url);
			    break;
			}
			else if(c == '\n')
			{
			    if(++i >= MAX_CHECK_LINES)
				break;
			}
		    }
		    if(comptype != -1)
			break;
		}
	    }
	    url_set_readlimit(url, -1);
	}

	if(comptype == -1)
	    comptype = ARCHIVEC_STORED;

	if(filename == NULL)
	{
	    char buff[32];
	    sprintf(buff, "part%d", part);
	    filename = strdup_mblock(&pool, buff);
	    arctype = -1;
	}
	else
	{
	    arctype = get_archive_type(filename);
	    switch(arctype)
	      {
	      case ARCHIVE_TAR:
	      case ARCHIVE_TGZ:
	      case ARCHIVE_ZIP:
	      case ARCHIVE_LZH:
		break;
	      default:
		arctype = -1;
		break;
	      }
	}

	if(data_start == data_end)
	  {
	    ArchiveEntryNode *entry;
	    entry = new_entry_node(filename, strlen(filename));
	    entry->comptype = ARCHIVEC_STORED;
	    entry->compsize = 0;
	    entry->origsize = 0;
	    entry->start = 0;
	    entry->cache = safe_strdup("");
	    if(head == NULL)
		head = tail = entry;
	    else
		tail = tail->next = entry;
	    goto next_entry;
	  }

	url_seek(url, data_start, SEEK_SET);
	part_data = url_dump(url, data_end - data_start, &part_data_size);
	part_data = arc_mime_decode(part_data, part_data_size,
				    comptype, &part_data_size);
	if(part_data == NULL)
	  goto next_entry;

	if(arctype == -1)
	{
	  int gzmethod, gzhdrsiz, len, gz;
	  ArchiveEntryNode *entry;

	  len = strlen(filename);
	  if(len >= 3 && strcasecmp(filename + len - 3, ".gz") == 0)
	    {
	      gz = 1;
	      filename[len - 3] = '\0';
	    }
	  else
	    gz = 0;
	  entry = new_entry_node(filename, strlen(filename));

	  if(gz)
	    gzmethod = parse_gzip_header_bytes(part_data, part_data_size,
					       &gzhdrsiz);
	  else
	    gzmethod = -1;
	  if(gzmethod == ARCHIVEC_DEFLATED)
	    {
	      entry->comptype = ARCHIVEC_DEFLATED;
	      entry->compsize = part_data_size - gzhdrsiz;
	      entry->origsize = -1;
	      entry->start = gzhdrsiz;
	      entry->cache = part_data;
	    }
	  else
	    {
	      entry->comptype = ARCHIVEC_DEFLATED;
	      entry->origsize = part_data_size;
	      entry->start = 0;
	      entry->cache = arc_compress(part_data, part_data_size,
					 ARC_DEFLATE_LEVEL, &entry->compsize);
	      free(part_data);
	      if(entry->cache == NULL)
		{
		  free_entry_node(entry);
		  goto next_entry;
		}
	    }
	    if(head == NULL)
		head = tail = entry;
	    else
		tail = tail->next = entry;
	}
	else
	{
	    URL arcurl;
	    ArchiveEntryNode *entry;
	    ArchiveHandler orig;

	    arcurl = url_mem_open(part_data, part_data_size, 1);
	    orig = arc_handler; /* save */
	    entry = arc_parse_entry(arcurl, arctype);
	    arc_handler = orig; /* restore */
	    if(head == NULL)
		head = tail = entry;
	    else
		tail = tail->next = entry;
	    while(tail->next)
	      tail = tail->next;
	}

      next_entry:
	url_seek(url, savepoint, SEEK_SET);
	part++;
	reuse_mblock(&pool);
	end_mime_stream(&hdr);

	if(last_check)
	{
	    pop_string_stack(&boundary);
	    if(top_string_stack(&boundary) == NULL)
		break;
	}
    }
    delete_string_stack(&boundary);
    return head;
}

static void init_string_stack(struct StringStack *stk)
{
    stk->elem = NULL;
    init_mblock(&stk->pool);
}

static void push_string_stack(struct StringStack *stk, char *str, int len)
{
    struct StringStackElem *elem;

    elem = (struct StringStackElem *)
	new_segment(&stk->pool, sizeof(struct StringStackElem) + len + 1);
    memcpy(elem->str, str, len);
    elem->str[len] = '\0';
    elem->next = stk->elem;
    stk->elem = elem;
}

static char *top_string_stack(struct StringStack *stk)
{
    if(stk->elem == NULL)
	return NULL;
    return stk->elem->str;
}

static void pop_string_stack(struct StringStack *stk)
{
    if(stk->elem == NULL)
	return;
    stk->elem = stk->elem->next;
}

static void delete_string_stack(struct StringStack *stk)
{
    reuse_mblock(&stk->pool);
}

static void init_mime_stream(struct MIMEHeaderStream *hdr, URL url)
{
    hdr->url = url;
    hdr->field = hdr->value = hdr->line = NULL;
    hdr->eof = 0;
    init_mblock(&hdr->pool);
}

static int whole_read_line(URL url, char *buff, int bufsiz)
{
    int len;

    if(url_gets(url, buff, bufsiz) == NULL)
	return -1;
    len = strlen(buff);
    if(len == 0)
	return 0;
    if(buff[len - 1] == '\n')
    {
	buff[--len] = '\0';
	if(len > 0 && buff[len - 1] == '\r')
	    buff[--len] = '\0';
    }
    else
    {
	/* skip line */
	int c;
	do
	{
	    c = url_getc(url);
	} while(c != EOF && c != '\n');
    }

    return len;
}

static int next_mime_header(struct MIMEHeaderStream *hdr)
{
    int len, c, n;
    char *p;

    if(hdr->eof)
	return 0;

    if(hdr->line == NULL)
    {
	hdr->line = (char *)new_segment(&hdr->pool, MIN_MBLOCK_SIZE);
	len = whole_read_line(hdr->url, hdr->line, MIN_MBLOCK_SIZE);
	if(len <= 0)
	{
	    if(len == -1)
		hdr->eof = 1;
	    return 0;
	}
	hdr->field = (char *)new_segment(&hdr->pool, MIN_MBLOCK_SIZE);
	hdr->bufflen = 0;
    }

    if((hdr->bufflen = strlen(hdr->line)) == 0)
	return 0;

    memcpy(hdr->field, hdr->line, hdr->bufflen);
    hdr->field[hdr->bufflen] = '\0';

    for(;;)
    {
	len = whole_read_line(hdr->url, hdr->line, MIN_MBLOCK_SIZE);
	if(len <= 0)
	{
	    if(len == -1)
		hdr->eof = 1;
	    break;
	}
	c = *hdr->line;
	if(c == '>' || ('A' <= c && c <= 'Z') ||  ('a' <= c && c <= 'z'))
	    break;
	if(c != ' ' && c != '\t')
	    return 0; /* ?? */

	n = MIN_MBLOCK_SIZE - 1 - hdr->bufflen;
	if(n > 0)
	{
	    int i;

	    if(len > n)
		len = n;

	    /* s/\t/ /g; */
	    p = hdr->line;
	    for(i = 0; i < len; i++)
		if(p[i] == '\t')
		    p[i] = ' ';

	    memcpy(hdr->field + hdr->bufflen, p, len);
	    hdr->bufflen += len;
	    hdr->field[hdr->bufflen] = '\0';
	}
    }
    p = hdr->field;
    while(*p && *p != ':')
	p++;
    if(!*p)
	return 0;
    *p++ = '\0';
    while(*p && *p == ' ')
	p++;
    hdr->value = p;
    return 1;
}

static void end_mime_stream(struct MIMEHeaderStream *hdr)
{
    reuse_mblock(&hdr->pool);
}

static int seek_next_boundary(URL url, char *boundary, long *endpoint)
{
    MBlockList pool;
    char *buff;
    int blen, ret;

    if(boundary == NULL)
    {
	url_seek(url, 0, SEEK_END);
	*endpoint = url_tell(url);
	return 0;
    }

    init_mblock(&pool);
    buff = (char *)new_segment(&pool, MIN_MBLOCK_SIZE);
    blen = strlen(boundary);
    ret = 0;
    for(;;)
    {
	int len;

	*endpoint = url_tell(url);
	if((len = whole_read_line(url, buff, MIN_MBLOCK_SIZE)) < 0)
	    break;
	if(len < blen + 2)
	    continue;

	if(buff[0] == '-' && buff[1] == '-' &&
	   strncmp(buff + 2, boundary, blen) == 0)
	{
	    if(buff[blen + 2] == '-' && buff[blen + 3] == '-')
		ret = 1;
	    break;
	}
    }
    reuse_mblock(&pool);
    return ret;
}

static void *arc_mime_decode(void *data, long size,
			     int comptype, long *newsize)
{
  URL url;

  if(comptype == ARCHIVEC_STORED)
    return data;

  if(data == NULL)
    return NULL;

  if((url = url_mem_open(data, size, 1)) == NULL)
    return NULL;

  switch(comptype)
    {
    case ARCHIVEC_UU:		/* uu encoded */
      url = url_uudecode_open(url, 1);
      break;
    case ARCHIVEC_B64:		/* base64 encoded */
      url = url_b64decode_open(url, 1);
      break;
    case ARCHIVEC_QS:		/* quoted string encoded */
      url = url_hqxdecode_open(url, 1, 1);
      break;
    case ARCHIVEC_HQX:		/* HQX encoded */
      url = url_qsdecode_open(url, 1);
      break;
    default:
      url_close(url);
      return NULL;
    }
  data = url_dump(url, -1, newsize);
  url_close(url);
  return data;
}
