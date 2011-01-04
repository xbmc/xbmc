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
 * $Id: version.c,v 1.7 2004/01/23 09:41:32 rob Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include "global.h"

# include "id3tag.h"
# include "version.h"

char const id3_version[]   = "ID3 Tag Library " ID3_VERSION;
char const id3_copyright[] = "Copyright (C) " ID3_PUBLISHYEAR " " ID3_AUTHOR;
char const id3_author[]    = ID3_AUTHOR " <" ID3_EMAIL ">";

char const id3_build[] = ""
# if defined(DEBUG)
  "DEBUG "
# elif defined(NDEBUG)
  "NDEBUG "
# endif

# if defined(EXPERIMENTAL)
  "EXPERIMENTAL "
# endif
;
