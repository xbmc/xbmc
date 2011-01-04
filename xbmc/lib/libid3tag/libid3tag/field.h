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
 * $Id: field.h,v 1.9 2004/01/23 09:41:32 rob Exp $
 */

# ifndef LIBID3TAG_FIELD_H
# define LIBID3TAG_FIELD_H

# include "id3tag.h"

void id3_field_init(union id3_field *, enum id3_field_type);
void id3_field_finish(union id3_field *);

int id3_field_parse(union id3_field *, id3_byte_t const **,
		    id3_length_t, enum id3_field_textencoding *);

id3_length_t id3_field_render(union id3_field const *, id3_byte_t **,
			      enum id3_field_textencoding *, int);

# endif
