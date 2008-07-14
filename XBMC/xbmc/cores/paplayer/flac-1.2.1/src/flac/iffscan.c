/* iffscan - Simple AIFF/RIFF chunk scanner
 * Copyright (C) 2007  Josh Coalson
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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined _MSC_VER || defined __MINGW32__
#include <sys/types.h> /* for off_t */
#if _MSC_VER <= 1600 /* @@@ [2G limit] */
#define fseeko fseek
#define ftello ftell
#endif
#endif
#include "foreign_metadata.h"

static FLAC__uint32 unpack32be_(const FLAC__byte *b)
{
	return ((FLAC__uint32)b[0]<<24) + ((FLAC__uint32)b[1]<<16) + ((FLAC__uint32)b[2]<<8) + (FLAC__uint32)b[3];
}

static FLAC__uint32 unpack32le_(const FLAC__byte *b)
{
	return (FLAC__uint32)b[0] + ((FLAC__uint32)b[1]<<8) + ((FLAC__uint32)b[2]<<16) + ((FLAC__uint32)b[3]<<24);
}

static FLAC__uint32 unpack32_(const FLAC__byte *b, foreign_block_type_t type)
{
	if(type == FOREIGN_BLOCK_TYPE__AIFF)
		return unpack32be_(b);
	else
		return unpack32le_(b);
}

int main(int argc, char *argv[])
{
	FILE *f;
	char buf[12];
	foreign_metadata_t *fm;
	const char *fn, *error;
	size_t i;
	FLAC__uint32 size;

	if(argc != 2) {
		fprintf(stderr, "usage: %s { file.wav | file.aif }\n", argv[0]);
		return 1;
	}
	fn = argv[1];
	if(0 == (f = fopen(fn, "rb")) || fread(buf, 1, 4, f) != 4) {
		fprintf(stderr, "ERROR opening %s for reading\n", fn);
		return 1;
	}
	fclose(f);
	if(0 == (fm = flac__foreign_metadata_new(memcmp(buf, "RIFF", 4)? FOREIGN_BLOCK_TYPE__AIFF : FOREIGN_BLOCK_TYPE__RIFF))) {
		fprintf(stderr, "ERROR: out of memory\n");
		return 1;
	}
	if(fm->type == FOREIGN_BLOCK_TYPE__AIFF) {
		if(!flac__foreign_metadata_read_from_aiff(fm, fn, &error)) {
			fprintf(stderr, "ERROR reading chunks from %s: %s\n", fn, error);
			return 1;
		}
	}
	else {
		if(!flac__foreign_metadata_read_from_wave(fm, fn, &error)) {
			fprintf(stderr, "ERROR reading chunks from %s: %s\n", fn, error);
			return 1;
		}
	}
	if(0 == (f = fopen(fn, "rb"))) {
		fprintf(stderr, "ERROR opening %s for reading\n", fn);
		return 1;
	}
	for(i = 0; i < fm->num_blocks; i++) {
		if(fseeko(f, fm->blocks[i].offset, SEEK_SET) < 0) {
			fprintf(stderr, "ERROR seeking in %s\n", fn);
			return 1;
		}
		if(fread(buf, 1, 12, f) != 12) {
			fprintf(stderr, "ERROR reading %s\n", fn);
			return 1;
		}
		size = unpack32_((const FLAC__byte*)buf+4, fm->type);
		printf("block:[%c%c%c%c] size=%08x=(%10u)", buf[0], buf[1], buf[2], buf[3], size, size);
		if(i == 0)
			printf(" type:[%c%c%c%c]", buf[8], buf[9], buf[10], buf[11]);
		else if(fm->type == FOREIGN_BLOCK_TYPE__AIFF && i == fm->audio_block)
			printf(" offset size=%08x=(%10u)", fm->ssnd_offset_size, fm->ssnd_offset_size);
		printf("\n");
	}
	fclose(f);
	flac__foreign_metadata_delete(fm);
	return 0;
}
