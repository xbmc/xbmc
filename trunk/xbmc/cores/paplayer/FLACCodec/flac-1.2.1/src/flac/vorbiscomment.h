/* flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef flac__vorbiscomment_h
#define flac__vorbiscomment_h

#include "FLAC/metadata.h"

FLAC__bool flac__vorbiscomment_add(FLAC__StreamMetadata *block, const char *comment, FLAC__bool value_from_file, FLAC__bool raw, const char **violation);

#endif
