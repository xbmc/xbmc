/* flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2000,2001,2002,2003,2004,2005,2006,2007  Josh Coalson
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

#if defined _MSC_VER || defined __MINGW32__
#include <sys/types.h> /* for off_t */
#if _MSC_VER <= 1600 /* @@@ [2G limit] */
#define fseeko fseek
#define ftello ftell
#endif
#endif
#include <stdio.h> /* for FILE etc. */
#include <stdlib.h> /* for calloc() etc. */
#include <string.h> /* for memcmp() etc. */
#include "FLAC/assert.h"
#include "FLAC/metadata.h"
#include "share/alloc.h"
#include "foreign_metadata.h"

#ifdef min
#undef min
#endif
#define min(x,y) ((x)<(y)?(x):(y))


static const char *FLAC__FOREIGN_METADATA_APPLICATION_ID[2] = { "aiff" , "riff" };

static FLAC__uint32 unpack32be_(const FLAC__byte *b)
{
	return ((FLAC__uint32)b[0]<<24) + ((FLAC__uint32)b[1]<<16) + ((FLAC__uint32)b[2]<<8) + (FLAC__uint32)b[3];
}

static FLAC__uint32 unpack32le_(const FLAC__byte *b)
{
	return (FLAC__uint32)b[0] + ((FLAC__uint32)b[1]<<8) + ((FLAC__uint32)b[2]<<16) + ((FLAC__uint32)b[3]<<24);
}

static FLAC__bool copy_data_(FILE *fin, FILE *fout, size_t size, const char **error, const char * const read_error, const char * const write_error)
{
	static FLAC__byte buffer[4096];
	size_t left;
	for(left = size; left > 0; ) {
		size_t need = min(sizeof(buffer), left);
		if(fread(buffer, 1, need, fin) < need) {
			if(error) *error = read_error;
			return false;
		}
		if(fwrite(buffer, 1, need, fout) < need) {
			if(error) *error = write_error;
			return false;
		}
		left -= need;
	}
	return true;
}

static FLAC__bool append_block_(foreign_metadata_t *fm, off_t offset, FLAC__uint32 size, const char **error)
{
	foreign_block_t *fb = safe_realloc_muladd2_(fm->blocks, sizeof(foreign_block_t), /*times (*/fm->num_blocks, /*+*/1/*)*/);
	if(fb) {
		fb[fm->num_blocks].offset = offset;
		fb[fm->num_blocks].size = size;
		fm->num_blocks++;
		fm->blocks = fb;
		return true;
	}
	if(error) *error = "out of memory";
	return false;
}

static FLAC__bool read_from_aiff_(foreign_metadata_t *fm, FILE *f, const char **error)
{
	FLAC__byte buffer[12];
	off_t offset, eof_offset;
	if((offset = ftello(f)) < 0) {
		if(error) *error = "ftello() error (001)";
		return false;
	}
	if(fread(buffer, 1, 12, f) < 12 || memcmp(buffer, "FORM", 4) || (memcmp(buffer+8, "AIFF", 4) && memcmp(buffer+8, "AIFC", 4))) {
		if(error) *error = "unsupported FORM layout (002)";
		return false;
	}
	if(!append_block_(fm, offset, 12, error))
		return false;
	eof_offset = 8 + unpack32be_(buffer+4);
	while(!feof(f)) {
		FLAC__uint32 size;
		if((offset = ftello(f)) < 0) {
			if(error) *error = "ftello() error (003)";
			return false;
		}
		if((size = fread(buffer, 1, 8, f)) < 8) {
			if(size == 0 && feof(f))
				break;
			if(error) *error = "invalid AIFF file (004)";
			return false;
		}
		size = unpack32be_(buffer+4);
		/* check if pad byte needed */
		if(size & 1)
			size++;
		if(!memcmp(buffer, "COMM", 4)) {
			if(fm->format_block) {
				if(error) *error = "invalid AIFF file: multiple \"COMM\" chunks (005)";
				return false;
			}
			if(fm->audio_block) {
				if(error) *error = "invalid AIFF file: \"SSND\" chunk before \"COMM\" chunk (006)";
				return false;
			}
			fm->format_block = fm->num_blocks;
		}
		else if(!memcmp(buffer, "SSND", 4)) {
			if(fm->audio_block) {
				if(error) *error = "invalid AIFF file: multiple \"SSND\" chunks (007)";
				return false;
			}
			if(!fm->format_block) {
				if(error) *error = "invalid AIFF file: \"SSND\" chunk before \"COMM\" chunk (008)";
				return false;
			}
			fm->audio_block = fm->num_blocks;
			/* read #offset bytes */
			if(fread(buffer+8, 1, 4, f) < 4) {
				if(error) *error = "invalid AIFF file (009)";
				return false;
			}
			fm->ssnd_offset_size = unpack32be_(buffer+8);
			if(fseeko(f, -4, SEEK_CUR) < 0) {
				if(error) *error = "invalid AIFF file: seek error (010)";
				return false;
			}
		}
		if(!append_block_(fm, offset, 8 + (memcmp(buffer, "SSND", 4)? size : 8 + fm->ssnd_offset_size), error))
			return false;
		if(fseeko(f, size, SEEK_CUR) < 0) {
			if(error) *error = "invalid AIFF file: seek error (011)";
			return false;
		}
	}
	if(eof_offset != ftello(f)) {
		if(error) *error = "invalid AIFF file: unexpected EOF (012)";
		return false;
	}
	if(!fm->format_block) {
		if(error) *error = "invalid AIFF file: missing \"COMM\" chunk (013)";
		return false;
	}
	if(!fm->audio_block) {
		if(error) *error = "invalid AIFF file: missing \"SSND\" chunk (014)";
		return false;
	}
	return true;
}

static FLAC__bool read_from_wave_(foreign_metadata_t *fm, FILE *f, const char **error)
{
	FLAC__byte buffer[12];
	off_t offset, eof_offset;
	if((offset = ftello(f)) < 0) {
		if(error) *error = "ftello() error (001)";
		return false;
	}
	if(fread(buffer, 1, 12, f) < 12 || memcmp(buffer, "RIFF", 4) || memcmp(buffer+8, "WAVE", 4)) {
		if(error) *error = "unsupported RIFF layout (002)";
		return false;
	}
	if(!append_block_(fm, offset, 12, error))
		return false;
	eof_offset = 8 + unpack32le_(buffer+4);
	while(!feof(f)) {
		FLAC__uint32 size;
		if((offset = ftello(f)) < 0) {
			if(error) *error = "ftello() error (003)";
			return false;
		}
		if((size = fread(buffer, 1, 8, f)) < 8) {
			if(size == 0 && feof(f))
				break;
			if(error) *error = "invalid WAVE file (004)";
			return false;
		}
		size = unpack32le_(buffer+4);
		/* check if pad byte needed */
		if(size & 1)
			size++;
		if(!memcmp(buffer, "fmt ", 4)) {
			if(fm->format_block) {
				if(error) *error = "invalid WAVE file: multiple \"fmt \" chunks (005)";
				return false;
			}
			if(fm->audio_block) {
				if(error) *error = "invalid WAVE file: \"data\" chunk before \"fmt \" chunk (006)";
				return false;
			}
			fm->format_block = fm->num_blocks;
		}
		else if(!memcmp(buffer, "data", 4)) {
			if(fm->audio_block) {
				if(error) *error = "invalid WAVE file: multiple \"data\" chunks (007)";
				return false;
			}
			if(!fm->format_block) {
				if(error) *error = "invalid WAVE file: \"data\" chunk before \"fmt \" chunk (008)";
				return false;
			}
			fm->audio_block = fm->num_blocks;
		}
		if(!append_block_(fm, offset, 8 + (memcmp(buffer, "data", 4)? size : 0), error))
			return false;
		if(fseeko(f, size, SEEK_CUR) < 0) {
			if(error) *error = "invalid WAVE file: seek error (009)";
			return false;
		}
	}
	if(eof_offset != ftello(f)) {
		if(error) *error = "invalid WAVE file: unexpected EOF (010)";
		return false;
	}
	if(!fm->format_block) {
		if(error) *error = "invalid WAVE file: missing \"fmt \" chunk (011)";
		return false;
	}
	if(!fm->audio_block) {
		if(error) *error = "invalid WAVE file: missing \"data\" chunk (012)";
		return false;
	}
	return true;
}

static FLAC__bool write_to_flac_(foreign_metadata_t *fm, FILE *fin, FILE *fout, FLAC__Metadata_SimpleIterator *it, const char **error)
{
	FLAC__byte buffer[4];
	const unsigned ID_LEN = FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8;
	size_t block_num = 0;
	FLAC__ASSERT(sizeof(buffer) >= ID_LEN);
	while(block_num < fm->num_blocks) {
		/* find next matching padding block */
		do {
			/* even on the first chunk's loop there will be a skippable STREAMINFO block, on subsequent loops we are first moving past the PADDING we just used */
			if(!FLAC__metadata_simple_iterator_next(it)) {
				if(error) *error = "no matching PADDING block found (004)";
				return false;
			}
		} while(FLAC__metadata_simple_iterator_get_block_type(it) != FLAC__METADATA_TYPE_PADDING);
		if(FLAC__metadata_simple_iterator_get_block_length(it) != ID_LEN+fm->blocks[block_num].size) {
			if(error) *error = "PADDING block with wrong size found (005)";
			return false;
		}
		/* transfer chunk into APPLICATION block */
		/* first set up the file pointers */
		if(fseeko(fin, fm->blocks[block_num].offset, SEEK_SET) < 0) {
			if(error) *error = "seek failed in WAVE/AIFF file (006)";
			return false;
		}
		if(fseeko(fout, FLAC__metadata_simple_iterator_get_block_offset(it), SEEK_SET) < 0) {
			if(error) *error = "seek failed in FLAC file (007)";
			return false;
		}
		/* update the type */
		buffer[0] = FLAC__METADATA_TYPE_APPLICATION;
		if(FLAC__metadata_simple_iterator_is_last(it))
			buffer[0] |= 0x80; /*MAGIC number*/
		if(fwrite(buffer, 1, 1, fout) < 1) {
			if(error) *error = "write failed in FLAC file (008)";
			return false;
		}
		/* length stays the same so skip over it */
		if(fseeko(fout, FLAC__STREAM_METADATA_LENGTH_LEN/8, SEEK_CUR) < 0) {
			if(error) *error = "seek failed in FLAC file (009)";
			return false;
		}
		/* write the APPLICATION ID */
		memcpy(buffer, FLAC__FOREIGN_METADATA_APPLICATION_ID[fm->type], ID_LEN);
		if(fwrite(buffer, 1, ID_LEN, fout) < ID_LEN) {
			if(error) *error = "write failed in FLAC file (010)";
			return false;
		}
		/* transfer the foreign metadata */
		if(!copy_data_(fin, fout, fm->blocks[block_num].size, error, "read failed in WAVE/AIFF file (011)", "write failed in FLAC file (012)"))
			return false;
		block_num++;
	}
	return true;
}

static FLAC__bool read_from_flac_(foreign_metadata_t *fm, FILE *f, FLAC__Metadata_SimpleIterator *it, const char **error)
{
	FLAC__byte id[4], buffer[12];
	off_t offset;
	FLAC__bool type_found = false;

	FLAC__ASSERT(FLAC__STREAM_METADATA_APPLICATION_ID_LEN == sizeof(id)*8);

	while(FLAC__metadata_simple_iterator_next(it)) {
		if(FLAC__metadata_simple_iterator_get_block_type(it) != FLAC__METADATA_TYPE_APPLICATION)
			continue;
		if(!FLAC__metadata_simple_iterator_get_application_id(it, id)) {
			if(error) *error = "FLAC__metadata_simple_iterator_get_application_id() error (003)";
			return false;
		}
		if(memcmp(id, FLAC__FOREIGN_METADATA_APPLICATION_ID[fm->type], sizeof(id)))
			continue;
		offset = FLAC__metadata_simple_iterator_get_block_offset(it);
		/* skip over header and app ID */
		offset += (FLAC__STREAM_METADATA_IS_LAST_LEN + FLAC__STREAM_METADATA_TYPE_LEN + FLAC__STREAM_METADATA_LENGTH_LEN) / 8;
		offset += sizeof(id);
		/* look for format or audio blocks */
		if(fseek(f, offset, SEEK_SET) < 0) {
			if(error) *error = "seek error (004)";
			return false;
		}
		if(fread(buffer, 1, 4, f) != 4) {
			if(error) *error = "read error (005)";
			return false;
		}
		if(fm->num_blocks == 0) {
			if(fm->type == FOREIGN_BLOCK_TYPE__RIFF && 0 == memcmp(buffer, "RIFF", 4))
				type_found = true;
			else if(fm->type == FOREIGN_BLOCK_TYPE__AIFF && 0 == memcmp(buffer, "FORM", 4))
				type_found = true;
			else {
				if(error) *error = "unsupported foreign metadata found, may need newer FLAC decoder (005)";
				return false;
			}
		}
		else if(!type_found) {
			FLAC__ASSERT(0);
			/* double protection: */
			if(error) *error = "unsupported foreign metadata found, may need newer FLAC decoder (006)";
			return false;
		}
		else if(fm->type == FOREIGN_BLOCK_TYPE__RIFF) {
			if(!memcmp(buffer, "fmt ", 4)) {
				if(fm->format_block) {
					if(error) *error = "invalid WAVE metadata: multiple \"fmt \" chunks (007)";
					return false;
				}
				if(fm->audio_block) {
					if(error) *error = "invalid WAVE metadata: \"data\" chunk before \"fmt \" chunk (008)";
					return false;
				}
				fm->format_block = fm->num_blocks;
			}
			else if(!memcmp(buffer, "data", 4)) {
				if(fm->audio_block) {
					if(error) *error = "invalid WAVE metadata: multiple \"data\" chunks (009)";
					return false;
				}
				if(!fm->format_block) {
					if(error) *error = "invalid WAVE metadata: \"data\" chunk before \"fmt \" chunk (010)";
					return false;
				}
				fm->audio_block = fm->num_blocks;
			}
		}
		else if(fm->type == FOREIGN_BLOCK_TYPE__AIFF) {
			if(!memcmp(buffer, "COMM", 4)) {
				if(fm->format_block) {
					if(error) *error = "invalid AIFF metadata: multiple \"COMM\" chunks (011)";
					return false;
				}
				if(fm->audio_block) {
					if(error) *error = "invalid AIFF metadata: \"SSND\" chunk before \"COMM\" chunk (012)";
					return false;
				}
				fm->format_block = fm->num_blocks;
			}
			else if(!memcmp(buffer, "SSND", 4)) {
				if(fm->audio_block) {
					if(error) *error = "invalid AIFF metadata: multiple \"SSND\" chunks (013)";
					return false;
				}
				if(!fm->format_block) {
					if(error) *error = "invalid AIFF metadata: \"SSND\" chunk before \"COMM\" chunk (014)";
					return false;
				}
				fm->audio_block = fm->num_blocks;
				/* read SSND offset size */
				if(fread(buffer+4, 1, 8, f) != 8) {
					if(error) *error = "read error (015)";
					return false;
				}
				fm->ssnd_offset_size = unpack32be_(buffer+8);
			}
		}
		else {
			FLAC__ASSERT(0);
			/* double protection: */
			if(error) *error = "unsupported foreign metadata found, may need newer FLAC decoder (016)";
			return false;
		}
		if(!append_block_(fm, offset, FLAC__metadata_simple_iterator_get_block_length(it)-sizeof(id), error))
			return false;
	}
	if(!type_found) {
		if(error) *error = "no foreign metadata found (017)";
		return false;
	}
	if(!fm->format_block) {
		if(error) *error = fm->type==FOREIGN_BLOCK_TYPE__RIFF? "invalid WAVE file: missing \"fmt \" chunk (018)" : "invalid AIFF file: missing \"COMM\" chunk (018)";
		return false;
	}
	if(!fm->audio_block) {
		if(error) *error = fm->type==FOREIGN_BLOCK_TYPE__RIFF? "invalid WAVE file: missing \"data\" chunk (019)" : "invalid AIFF file: missing \"SSND\" chunk (019)";
		return false;
	}
	return true;
}

static FLAC__bool write_to_iff_(foreign_metadata_t *fm, FILE *fin, FILE *fout, off_t offset1, off_t offset2, off_t offset3, const char **error)
{
	size_t i;
	if(fseeko(fout, offset1, SEEK_SET) < 0) {
		if(error) *error = "seek failed in WAVE/AIFF file (002)";
		return false;
	}
	for(i = 1; i < fm->format_block; i++) {
		if(fseeko(fin, fm->blocks[i].offset, SEEK_SET) < 0) {
			if(error) *error = "seek failed in FLAC file (003)";
			return false;
		}
		if(!copy_data_(fin, fout, fm->blocks[i].size, error, "read failed in WAVE/AIFF file (004)", "write failed in FLAC file (005)"))
			return false;
	}
	if(fseeko(fout, offset2, SEEK_SET) < 0) {
		if(error) *error = "seek failed in WAVE/AIFF file (006)";
		return false;
	}
	for(i = fm->format_block+1; i < fm->audio_block; i++) {
		if(fseeko(fin, fm->blocks[i].offset, SEEK_SET) < 0) {
			if(error) *error = "seek failed in FLAC file (007)";
			return false;
		}
		if(!copy_data_(fin, fout, fm->blocks[i].size, error, "read failed in WAVE/AIFF file (008)", "write failed in FLAC file (009)"))
			return false;
	}
	if(fseeko(fout, offset3, SEEK_SET) < 0) {
		if(error) *error = "seek failed in WAVE/AIFF file (010)";
		return false;
	}
	for(i = fm->audio_block+1; i < fm->num_blocks; i++) {
		if(fseeko(fin, fm->blocks[i].offset, SEEK_SET) < 0) {
			if(error) *error = "seek failed in FLAC file (011)";
			return false;
		}
		if(!copy_data_(fin, fout, fm->blocks[i].size, error, "read failed in WAVE/AIFF file (012)", "write failed in FLAC file (013)"))
			return false;
	}
	return true;
}

foreign_metadata_t *flac__foreign_metadata_new(foreign_block_type_t type)
{
	foreign_metadata_t *x = (foreign_metadata_t*)calloc(sizeof(foreign_metadata_t), 1);
	if(x)
		x->type = type;
	return x;
}

void flac__foreign_metadata_delete(foreign_metadata_t *fm)
{
	if(fm) {
		if(fm->blocks)
			free(fm->blocks);
		free(fm);
	}
}

FLAC__bool flac__foreign_metadata_read_from_aiff(foreign_metadata_t *fm, const char *filename, const char **error)
{
	FLAC__bool ok;
	FILE *f = fopen(filename, "rb");
	if(!f) {
		if(error) *error = "can't open AIFF file for reading (000)";
		return false;
	}
	ok = read_from_aiff_(fm, f, error);
	fclose(f);
	return ok;
}

FLAC__bool flac__foreign_metadata_read_from_wave(foreign_metadata_t *fm, const char *filename, const char **error)
{
	FLAC__bool ok;
	FILE *f = fopen(filename, "rb");
	if(!f) {
		if(error) *error = "can't open WAVE file for reading (000)";
		return false;
	}
	ok = read_from_wave_(fm, f, error);
	fclose(f);
	return ok;
}

FLAC__bool flac__foreign_metadata_write_to_flac(foreign_metadata_t *fm, const char *infilename, const char *outfilename, const char **error)
{
	FLAC__bool ok;
	FILE *fin, *fout;
	FLAC__Metadata_SimpleIterator *it = FLAC__metadata_simple_iterator_new();
	if(!it) {
		if(error) *error = "out of memory (000)";
		return false;
	}
	if(!FLAC__metadata_simple_iterator_init(it, outfilename, /*read_only=*/true, /*preserve_file_stats=*/false)) {
		if(error) *error = "can't initialize iterator (001)";
		FLAC__metadata_simple_iterator_delete(it);
		return false;
	}
	if(0 == (fin = fopen(infilename, "rb"))) {
		if(error) *error = "can't open WAVE/AIFF file for reading (002)";
		FLAC__metadata_simple_iterator_delete(it);
		return false;
	}
	if(0 == (fout = fopen(outfilename, "r+b"))) {
		if(error) *error = "can't open FLAC file for updating (003)";
		FLAC__metadata_simple_iterator_delete(it);
		fclose(fin);
		return false;
	}
	ok = write_to_flac_(fm, fin, fout, it, error);
	FLAC__metadata_simple_iterator_delete(it);
	fclose(fin);
	fclose(fout);
	return ok;
}

FLAC__bool flac__foreign_metadata_read_from_flac(foreign_metadata_t *fm, const char *filename, const char **error)
{
	FLAC__bool ok;
	FILE *f;
	FLAC__Metadata_SimpleIterator *it = FLAC__metadata_simple_iterator_new();
	if(!it) {
		if(error) *error = "out of memory (000)";
		return false;
	}
	if(!FLAC__metadata_simple_iterator_init(it, filename, /*read_only=*/true, /*preserve_file_stats=*/false)) {
		if(error) *error = "can't initialize iterator (001)";
		FLAC__metadata_simple_iterator_delete(it);
		return false;
	}
	if(0 == (f = fopen(filename, "rb"))) {
		if(error) *error = "can't open FLAC file for reading (002)";
		FLAC__metadata_simple_iterator_delete(it);
		return false;
	}
	ok = read_from_flac_(fm, f, it, error);
	FLAC__metadata_simple_iterator_delete(it);
	fclose(f);
	return ok;
}

FLAC__bool flac__foreign_metadata_write_to_iff(foreign_metadata_t *fm, const char *infilename, const char *outfilename, off_t offset1, off_t offset2, off_t offset3, const char **error)
{
	FLAC__bool ok;
	FILE *fin, *fout;
	if(0 == (fin = fopen(infilename, "rb"))) {
		if(error) *error = "can't open FLAC file for reading (000)";
		return false;
	}
	if(0 == (fout = fopen(outfilename, "r+b"))) {
		if(error) *error = "can't open WAVE/AIFF file for updating (001)";
		fclose(fin);
		return false;
	}
	ok = write_to_iff_(fm, fin, fout, offset1, offset2, offset3, error);
	fclose(fin);
	fclose(fout);
	return ok;
}
