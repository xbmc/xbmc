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
 * $Id: compat.h,v 1.8 2004/01/23 09:41:32 rob Exp $
 */

# ifndef LIBID3TAG_COMPAT_H
# define LIBID3TAG_COMPAT_H

# include "id3tag.h"

typedef int id3_compat_func_t(struct id3_frame *, char const *,
			      id3_byte_t const *, id3_length_t);

struct id3_compat {
  char const *id;
  char const *equiv;
  id3_compat_func_t *translate;
};

struct id3_compat const *id3_compat_lookup(register char const *,
					   register unsigned int);

int id3_compat_fixup(struct id3_tag *);

# endif
