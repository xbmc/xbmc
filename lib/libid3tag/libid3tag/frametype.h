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
 * $Id: frametype.h,v 1.7 2004/01/23 09:41:32 rob Exp $
 */

# ifndef LIBID3TAG_FRAMETYPE_H
# define LIBID3TAG_FRAMETYPE_H

struct id3_frametype {
  char const *id;
  unsigned int nfields;
  enum id3_field_type const *fields;
  int defaultflags;
  char const *description;
};

extern struct id3_frametype const id3_frametype_text;
extern struct id3_frametype const id3_frametype_url;
extern struct id3_frametype const id3_frametype_experimental;
extern struct id3_frametype const id3_frametype_unknown;
extern struct id3_frametype const id3_frametype_obsolete;

struct id3_frametype const *id3_frametype_lookup(register char const *,
						 register unsigned int);

# endif
