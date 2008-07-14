/* metaflac - Command-line FLAC metadata editor
 * Copyright (C) 2001,2002,2003,2004,2005,2006,2007  Josh Coalson
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

#ifndef metaflac__utils_h
#define metaflac__utils_h

#include "FLAC/metadata.h"
#include <stdio.h> /* for FILE */

void die(const char *message);
#ifdef FLAC__VALGRIND_TESTING
size_t local_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
#else
#define local_fwrite fwrite
#endif
char *local_strdup(const char *source);
void local_strcat(char **dest, const char *source);
void hexdump(const char *filename, const FLAC__byte *buf, unsigned bytes, const char *indent);
void print_error_with_chain_status(FLAC__Metadata_Chain *chain, const char *format, ...);

FLAC__bool parse_vorbis_comment_field(const char *field_ref, char **field, char **name, char **value, unsigned *length, const char **violation);

void write_vc_field(const char *filename, const FLAC__StreamMetadata_VorbisComment_Entry *entry, FLAC__bool raw, FILE *f);
void write_vc_fields(const char *filename, const char *field_name, const FLAC__StreamMetadata_VorbisComment_Entry entry[], unsigned num_entries, FLAC__bool raw, FILE *f);

#endif
