/*
 * libid3tag - ID3 tag manipulation library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: parse.h,v 1.6 2004/01/23 09:41:32 rob Exp $
 */

# ifndef LIBID3TAG_PARSE_H
# define LIBID3TAG_PARSE_H

signed long id3_parse_int(id3_byte_t const **, unsigned int);
unsigned long id3_parse_uint(id3_byte_t const **, unsigned int);
unsigned long id3_parse_syncsafe(id3_byte_t const **, unsigned int);
void id3_parse_immediate(id3_byte_t const **, unsigned int, char *);
id3_latin1_t *id3_parse_latin1(id3_byte_t const **, id3_length_t, int);
id3_ucs4_t *id3_parse_string(id3_byte_t const **, id3_length_t,
			     enum id3_field_textencoding, int);
id3_byte_t *id3_parse_binary(id3_byte_t const **, id3_length_t);

# endif
