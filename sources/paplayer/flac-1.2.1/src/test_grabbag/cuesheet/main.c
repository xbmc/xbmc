/* test_cuesheet - Simple tester for cuesheet routines in grabbag
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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FLAC/assert.h"
#include "FLAC/metadata.h"
#include "share/grabbag.h"

static int do_cuesheet(const char *infilename, FLAC__bool is_cdda, FLAC__uint64 lead_out_offset)
{
	FILE *fin, *fout;
	const char *error_message;
	char tmpfilename[4096];
	unsigned last_line_read;
	FLAC__StreamMetadata *cuesheet;

	FLAC__ASSERT(strlen(infilename) + 2 < sizeof(tmpfilename));

	/*
	 * pass 1
	 */
	if(0 == strcmp(infilename, "-")) {
		fin = stdin;
	}
	else if(0 == (fin = fopen(infilename, "r"))) {
		fprintf(stderr, "can't open file %s for reading: %s\n", infilename, strerror(errno));
		return 255;
	}
	if(0 != (cuesheet = grabbag__cuesheet_parse(fin, &error_message, &last_line_read, is_cdda, lead_out_offset))) {
		if(fin != stdin)
			fclose(fin);
	}
	else {
		printf("pass1: parse error, line %u: \"%s\"\n", last_line_read, error_message);
		if(fin != stdin)
			fclose(fin);
		return 1;
	}
	if(!FLAC__metadata_object_cuesheet_is_legal(cuesheet, is_cdda, &error_message)) {
		printf("pass1: illegal cuesheet: \"%s\"\n", error_message);
		FLAC__metadata_object_delete(cuesheet);
		return 1;
	}
	sprintf(tmpfilename, "%s.1", infilename);
	if(0 == (fout = fopen(tmpfilename, "w"))) {
		fprintf(stderr, "can't open file %s for writing: %s\n", tmpfilename, strerror(errno));
		FLAC__metadata_object_delete(cuesheet);
		return 255;
	}
	grabbag__cuesheet_emit(fout, cuesheet, "\"dummy.wav\" WAVE");
	FLAC__metadata_object_delete(cuesheet);
	fclose(fout);

	/*
	 * pass 2
	 */
	if(0 == (fin = fopen(tmpfilename, "r"))) {
		fprintf(stderr, "can't open file %s for reading: %s\n", tmpfilename, strerror(errno));
		return 255;
	}
	if(0 != (cuesheet = grabbag__cuesheet_parse(fin, &error_message, &last_line_read, is_cdda, lead_out_offset))) {
		if(fin != stdin)
			fclose(fin);
	}
	else {
		printf("pass2: parse error, line %u: \"%s\"\n", last_line_read, error_message);
		if(fin != stdin)
			fclose(fin);
		return 1;
	}
	if(!FLAC__metadata_object_cuesheet_is_legal(cuesheet, is_cdda, &error_message)) {
		printf("pass2: illegal cuesheet: \"%s\"\n", error_message);
		FLAC__metadata_object_delete(cuesheet);
		return 1;
	}
	sprintf(tmpfilename, "%s.2", infilename);
	if(0 == (fout = fopen(tmpfilename, "w"))) {
		fprintf(stderr, "can't open file %s for writing: %s\n", tmpfilename, strerror(errno));
		FLAC__metadata_object_delete(cuesheet);
		return 255;
	}
	grabbag__cuesheet_emit(fout, cuesheet, "\"dummy.wav\" WAVE");
	FLAC__metadata_object_delete(cuesheet);
	fclose(fout);

	return 0;
}

int main(int argc, char *argv[])
{
	FLAC__uint64 lead_out_offset;
	FLAC__bool is_cdda = false;
	const char *usage = "usage: test_cuesheet cuesheet_file lead_out_offset [ cdda ]\n";

	if(argc > 1 && 0 == strcmp(argv[1], "-h")) {
		printf(usage);
		return 0;
	}

	if(argc < 3 || argc > 4) {
		fprintf(stderr, usage);
		return 255;
	}

	lead_out_offset = (FLAC__uint64)strtoul(argv[2], 0, 10);
	if(argc == 4) {
		if(0 == strcmp(argv[3], "cdda"))
			is_cdda = true;
		else {
			fprintf(stderr, usage);
			return 255;
		}
	}

	return do_cuesheet(argv[1], is_cdda, lead_out_offset);
}
