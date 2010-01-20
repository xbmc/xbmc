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
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <ctype.h>
#include "timidity.h"
#include "arc.h"

#define boolean int

#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE 1
#endif

#define DELIM ('/')
#define DELIM2 (0xff)
#define LZHEADER_STRAGE 4096
#define FILENAME_LENGTH 1024

#define I_HEADER_SIZE			0
#define I_HEADER_CHECKSUM		1
#define I_METHOD			2
#define I_PACKED_SIZE			7
#define I_ORIGINAL_SIZE			11
#define I_LAST_MODIFIED_STAMP		15
#define I_ATTRIBUTE			19
#define I_HEADER_LEVEL			20
#define I_NAME_LENGTH			21
#define I_NAME				22

#define EXTEND_GENERIC		0
#define EXTEND_UNIX		'U'
#define EXTEND_MSDOS		'M'
#define EXTEND_MACOS		'm'
#define EXTEND_OS9		'9'
#define EXTEND_OS2		'2'
#define EXTEND_OS68K		'K'
#define EXTEND_OS386		'3'		/* OS-9000??? */
#define EXTEND_HUMAN		'H'
#define EXTEND_CPM		'C'
#define EXTEND_FLEX		'F'
#define EXTEND_RUNSER		'R'
/*	this OS type is not official */
#define EXTEND_TOWNSOS		'T'
#define EXTEND_XOSK		'X'

static char *get_ptr;
#define setup_get(PTR)		(get_ptr = (PTR))
#define get_byte()		(*get_ptr++ & 0xff)
#define skip_byte()		get_ptr++

static unsigned short get_word(void)
{
    int b0, b1;

    b0 = get_byte();
    b1 = get_byte();
    return (b1 << 8) + b0;
}

static long get_longword(void)
{
    long b0, b1, b2, b3;

    b0 = get_byte();
    b1 = get_byte();
    b2 = get_byte();
    b3 = get_byte();
    return (b3 << 24) + (b2 << 16) + (b1 << 8) + b0;
}

static void msdos_to_unix_filename(char *name, int len)
{
    int i;

#ifdef MULTIBYTE_CHAR
    for(i = 0; i < len; i ++)
    {
	int c1, c2;
	c1 = (int)(unsigned char)name[i];
	c2 = (int)(unsigned char)name[i+1];
	if(MULTIBYTE_FIRST_P(c1) && MULTIBYTE_SECOND_P(c2))
	    i++;
	else if(c1 == '\\')
	    name[i] = '/';
	else if(isupper(c1))
	    name[i] = tolower(c1);
    }
#else
    for(i = 0; i < len; i ++)
    {
	int c;
	c = (int)(unsigned char)name[i];
	if(c == '\\')
	    name[i] = '/';
	else if(isupper(c))
	    name[i] = tolower(c);
    }
#endif
}

static void generic_to_unix_filename(char *name, int len)
{
    register int i;
    boolean lower_case_used = FALSE;

#ifdef MULTIBYTE_CHAR
    for(i = 0; i < len; i ++)
    {
	int c1, c2;
	c1 = (int)(unsigned char)name[i];
	c2 = (int)(unsigned char)name[i+1];
	if(MULTIBYTE_FIRST_P(c1) && MULTIBYTE_SECOND_P(c2))
	    i ++;
	else if(islower(c1))
	{
	    lower_case_used = TRUE;
	    break;
	}
    }
    for(i = 0; i < len; i ++)
    {
	int c1, c2;
	c1 = (int)(unsigned char)name[i];
	c2 = (int)(unsigned char)name[i+1];
	if(MULTIBYTE_FIRST_P(c1) && MULTIBYTE_SECOND_P(c2))
	    i++;
	else if(c1 == '\\')
	    name[i] = '/';
	else if(!lower_case_used && isupper(c1))
	    name[i] = tolower(c1);
    }
#else
    for(i = 0; i < len; i ++)
    {
	int c;
	c = (int)(unsigned char)name[i];
	if(islower(c))
	{
	    lower_case_used = TRUE;
	    break;
	}
    }
    for(i = 0; i < len; i ++)
    {
	int c;
	c = (int)(unsigned char)name[i];
	if(c == '\\')
	    name[i] = '/';
	else if(!lower_case_used && isupper(c))
	    name[i] = tolower(c);
    }
#endif
}

static void macos_to_unix_filename(char *name, int len)
{
    register int i;

    for(i = 0; i < len; i ++)
    {
	if(name[i] == ':')
	    name[i] = '/';
	else if(name[i] == '/')
	    name[i] = ':';
    }
}

#ifdef MULTIBYTE_CHAR
#define iskanji(c) (c & 0x80)
#endif /* MULTIBYTE_CHAR */
static unsigned char *convdelim(unsigned char *path, unsigned char delim)
{
    unsigned char c;
    unsigned char *p;
#ifdef MULTIBYTE_CHAR
    int kflg;

    kflg = 0;
#endif
    for(p = path; (c = *p) != 0; p++)
    {
#ifdef MULTIBYTE_CHAR
	if(kflg)
	{
	    kflg = 0;
	}
	else if(iskanji(c))
	{
	    kflg = 1;
	}
	else
#endif
	    if(c == '\\' || c == DELIM || c == DELIM2)
	    {
		*p = delim;
		path = p + 1;
	    }
    }
    return path;
}

ArchiveEntryNode *next_lzh_entry(void)
{
    ArchiveEntryNode *entry;
    URL url;
    int header_size;
    char data[LZHEADER_STRAGE];
    char dirname[FILENAME_LENGTH];
    char filename[FILENAME_LENGTH];
    int dir_length, name_length;
    int i;
    char *ptr;
    int header_level;
    char method_id[5];
    long compsize, origsize;
    int extend_type;
    int hdrsiz;
    int macbin_check;
    extern char *lzh_methods[];

    url = arc_handler.url;
    macbin_check = (arc_handler.counter == 0);

  retry_read:
    dir_length = 0;
    name_length = 0;
#if 0
    if((header_size = url_getc(url)) == EOF)
	return NULL;
    if(header_size == 0)
    {
	if(macbin_check)
	{
	    macbin_check = 0;
	    url_skip(url, 128-1);
	    if(arc_handler.isfile)
		arc_handler.pos += 128;
	    goto retry_read;
	}
	return NULL;
    }

    macbin_check = 0;
    if(url_read(url, data + I_HEADER_CHECKSUM,
		header_size - 1) < header_size - 1)
	return NULL;
#else	/* a little cleverer lzh check */
	if(macbin_check){
/*	for(i=0;i<LZHEADER_STRAGE;i++){ */
	for(i=0;i<1024;i++){
		if((header_size = url_getc(url)) == EOF)
			return NULL;
		*(data + i) = header_size;
		if(i >= 6){
			if(*(data + i - 4) == '-'
				&& *(data + i - 3) == 'l'
				&& *(data + i - 2) == 'h'
				&& *(data + i - 0) == '-')
			{
				int j;
				if(arc_handler.isfile)
					arc_handler.pos += i - 6;
				for(j = 0; j<= 6; j++)
					*(data + j) = *(data + i - 6 + j);
				header_size = (int)(unsigned char)(*(data + i - 6));
				if(header_size == 0)
					return NULL;
				if(url_read(url, data + 7, header_size - 7) < header_size - 7)
					return NULL;
				break;
			}
		}
	}
	if(i >= LZHEADER_STRAGE)
		return NULL;
	} else {
	    if((header_size = url_getc(url)) == EOF)
			return NULL;
		if(url_read(url, data + I_HEADER_CHECKSUM,
			header_size - 1) < header_size - 1)
		return NULL;
	}
    macbin_check = 0;
#endif
    hdrsiz = header_size;
    setup_get(data + I_HEADER_LEVEL);
    header_level = get_byte();

    if(header_level != 2)
    {
	if(url_read(url, data + header_size, 2) < 2)
	    return NULL;
	hdrsiz += 2;
    }

    setup_get(data + I_HEADER_CHECKSUM);
    skip_byte(); /* checksum */

    memcpy(method_id, data + I_METHOD, sizeof(method_id));

    setup_get(data + I_PACKED_SIZE);
    compsize = get_longword();
    origsize = get_longword();
    get_longword(); /* last_modified_stamp */
    skip_byte(); /* attribute */

    if((header_level = get_byte()) != 2)
    {
	name_length = get_byte();
	for(i = 0; i < name_length; i ++)
	    filename[i] =(char)get_byte();
	filename[name_length] = '\0';
    }

    if(header_size - name_length >= 24)
    {				/* EXTEND FORMAT */
	get_word(); /* crc */
	extend_type = get_byte();
    }
    else if(header_size - name_length == 22)
    {				/* Generic with CRC */
	get_word(); /* crc */
	extend_type = EXTEND_GENERIC;
    }
    else if(header_size - name_length == 20)
    {				/* Generic no CRC */
	extend_type = EXTEND_GENERIC;
    }
    else
	return NULL;

    if(extend_type == EXTEND_UNIX && header_level == 0)
    {
	skip_byte();		/* minor_version */
	get_longword();		/* unix_last_modified_stamp */
	get_word();		/* unix_mode */
	get_word();		/* unix_uid */
	get_word();		/* unix_gid */
	goto parse_ok;
    }

    if(header_level > 0)
    {
	/* Extend Header */
	if(header_level != 2)
	    setup_get(data + header_size);
	ptr = get_ptr;
	while((header_size = get_word()) != 0)
	{
	    if(header_level != 2)
	    {
		if(data + LZHEADER_STRAGE - get_ptr < header_size)
		    return NULL;
		if(url_read(url, get_ptr, header_size) < header_size)
		    return NULL;
		hdrsiz += header_size;
	    }

	    switch(get_byte())
	    {
	      case 0:
		/*
		 * header crc
		 */
		setup_get(get_ptr + header_size - 3);
		break;
	      case 1:
		/*
		 * filename
		 */
		name_length = header_size - 3;
		if(name_length >= sizeof(filename) - 1)
		    return NULL;
		for(i = 0; i < name_length; i++)
		    filename[i] =(char)get_byte ();
		filename[name_length] = '\0';
		break;
	      case 2:
		/*
		 * directory
		 */
		dir_length = header_size - 3;
		if(dir_length >=  sizeof(dirname) - 1)
		    return NULL;
		for(i = 0; i < dir_length; i++)
		    dirname[i] = (char)get_byte ();
		dirname[dir_length] = '\0';
		convdelim((unsigned char *)dirname, DELIM);
		break;
	      case 0x40:
		/*
		 * MS-DOS attribute
		 */
		if(extend_type == EXTEND_MSDOS ||
		    extend_type == EXTEND_HUMAN ||
		    extend_type == EXTEND_GENERIC)
		    get_word(); /* attribute */
		break;
	      case 0x50:
		/*
		 * UNIX permission
		 */
		if(extend_type == EXTEND_UNIX)
		    get_word(); /* unix_mode */
		break;
	      case 0x51:
		/*
		 * UNIX gid and uid
		 */
		if(extend_type == EXTEND_UNIX)
		{
		    get_word(); /* unix_gid */
		    get_word(); /* unix_uid */
		}
		break;
	      case 0x52:
		/*
		 * UNIX group name
		 */
		setup_get(get_ptr + header_size - 3);
		break;
	      case 0x53:
		/*
		 * UNIX user name
		 */
		setup_get(get_ptr + header_size - 3);
		break;
	      case 0x54:
		/*
		 * UNIX last modified time
		 */
		if(extend_type == EXTEND_UNIX)
		    get_longword(); /* unix_last_modified_stamp */
		break;
	      default:
		/*
		 * other headers
		 */
		setup_get(get_ptr + header_size - 3);
		break;
	    }
	}
	if(header_level != 2 && get_ptr - ptr != 2)
	{
	    compsize -= get_ptr - ptr - 2;
	    header_size += get_ptr - ptr - 2;
	}
    }

    if(dir_length)
    {
	name_length += dir_length;
	if(name_length >= sizeof(filename) - 1)
	    return NULL;
	strcat(dirname, filename);
	strcpy(filename, dirname);
    }

    switch(extend_type)
    {
      case EXTEND_MSDOS:
	msdos_to_unix_filename(filename, name_length);
      case EXTEND_HUMAN:
	; /* ignored */
	break;

#ifdef OSK
      case EXTEND_OS68K:
      case EXTEND_XOSK:
#endif
      case EXTEND_UNIX:
	break;

      case EXTEND_MACOS:
	macos_to_unix_filename(filename, name_length);
	break;

      default:
	generic_to_unix_filename(filename, name_length);
    }

  parse_ok:
    if(strncmp("-lhd-", method_id, 5) == 0)
    {
	if(arc_handler.isfile)
	    arc_handler.pos += hdrsiz;
	goto retry_read; /* Skip directory entry */
    }

    for(i = 0; lzh_methods[i]; i++)
	if(strncmp(method_id, lzh_methods[i], sizeof(method_id)) == 0)
	    break;
    if(!lzh_methods[i])
	return NULL;
    entry = new_entry_node(filename, name_length);
    if(entry == NULL)
	return NULL;
    entry->comptype = i + ARCHIVEC_LZHED + 1;
    entry->compsize = compsize;
    entry->origsize = origsize;

    if(arc_handler.isfile)
    {
	arc_handler.pos += hdrsiz;
	entry->start = arc_handler.pos;
	entry->cache = NULL;
	url_skip(url, compsize);
	arc_handler.pos += compsize;
    }
    else
    {
      long n;
      entry->start = 0;
      entry->cache = url_dump(url, compsize, &n);
      if(n != compsize)
	{
	  free_entry_node(entry);
	  return NULL;
	}
    }
    return entry;
}
