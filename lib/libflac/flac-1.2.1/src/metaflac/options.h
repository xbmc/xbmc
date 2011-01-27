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

#ifndef metaflac__options_h
#define metaflac__options_h

#include "FLAC/format.h"

#if 0
/*[JEC] was:#if HAVE_GETOPT_LONG*/
/*[JEC] see flac/include/share/getopt.h as to why the change */
#  include <getopt.h>
#else
#  include "share/getopt.h"
#endif

extern struct share__option long_options_[];

typedef enum {
	OP__SHOW_MD5SUM,
	OP__SHOW_MIN_BLOCKSIZE,
	OP__SHOW_MAX_BLOCKSIZE,
	OP__SHOW_MIN_FRAMESIZE,
	OP__SHOW_MAX_FRAMESIZE,
	OP__SHOW_SAMPLE_RATE,
	OP__SHOW_CHANNELS,
	OP__SHOW_BPS,
	OP__SHOW_TOTAL_SAMPLES,
	OP__SET_MD5SUM,
	OP__SET_MIN_BLOCKSIZE,
	OP__SET_MAX_BLOCKSIZE,
	OP__SET_MIN_FRAMESIZE,
	OP__SET_MAX_FRAMESIZE,
	OP__SET_SAMPLE_RATE,
	OP__SET_CHANNELS,
	OP__SET_BPS,
	OP__SET_TOTAL_SAMPLES,
	OP__SHOW_VC_VENDOR,
	OP__SHOW_VC_FIELD,
	OP__REMOVE_VC_ALL,
	OP__REMOVE_VC_FIELD,
	OP__REMOVE_VC_FIRSTFIELD,
	OP__SET_VC_FIELD,
	OP__IMPORT_VC_FROM,
	OP__EXPORT_VC_TO,
	OP__IMPORT_CUESHEET_FROM,
	OP__EXPORT_CUESHEET_TO,
	OP__IMPORT_PICTURE_FROM,
	OP__EXPORT_PICTURE_TO,
	OP__ADD_SEEKPOINT,
	OP__ADD_REPLAY_GAIN,
	OP__ADD_PADDING,
	OP__LIST,
	OP__APPEND,
	OP__REMOVE,
	OP__REMOVE_ALL,
	OP__MERGE_PADDING,
	OP__SORT_PADDING
} OperationType;

typedef enum {
	ARG__BLOCK_NUMBER,
	ARG__BLOCK_TYPE,
	ARG__EXCEPT_BLOCK_TYPE,
	ARG__DATA_FORMAT,
	ARG__FROM_FILE
} ArgumentType;

typedef struct {
	FLAC__byte value[16];
} Argument_StreaminfoMD5;

typedef struct {
	FLAC__uint32 value;
} Argument_StreaminfoUInt32;

typedef struct {
	FLAC__uint64 value;
} Argument_StreaminfoUInt64;

typedef struct {
	char *value;
} Argument_VcFieldName;

typedef struct {
	char *field; /* the whole field as passed on the command line, i.e. "NAME=VALUE" */
	char *field_name;
	/* according to the vorbis spec, field values can contain \0 so simple C strings are not enough here */
	unsigned field_value_length;
	char *field_value;
	FLAC__bool field_value_from_file; /* true if field_value holds a filename for the value, false for plain value */
} Argument_VcField;

typedef struct {
	char *value;
} Argument_String;

typedef struct {
	unsigned num_entries;
	unsigned *entries;
} Argument_BlockNumber;

typedef struct {
	FLAC__MetadataType type;
	char application_id[4]; /* only relevant if type == FLAC__STREAM_METADATA_TYPE_APPLICATION */
	FLAC__bool filter_application_by_id;
} Argument_BlockTypeEntry;

typedef struct {
	unsigned num_entries;
	Argument_BlockTypeEntry *entries;
} Argument_BlockType;

typedef struct {
	FLAC__bool is_binary;
} Argument_DataFormat;

typedef struct {
	char *file_name;
} Argument_FromFile;

typedef struct {
	char *specification;
} Argument_AddSeekpoint;

typedef struct {
	char *filename;
	Argument_AddSeekpoint *add_seekpoint_link;
} Argument_ImportCuesheetFrom;

typedef struct {
	char *filename;
	const Argument_BlockNumber *block_number_link; /* may be NULL to mean 'first PICTURE block' */
} Argument_ExportPictureTo;

typedef struct {
	unsigned length;
} Argument_AddPadding;

typedef struct {
	OperationType type;
	union {
		Argument_StreaminfoMD5 streaminfo_md5;
		Argument_StreaminfoUInt32 streaminfo_uint32;
		Argument_StreaminfoUInt64 streaminfo_uint64;
		Argument_VcFieldName vc_field_name;
		Argument_VcField vc_field;
		Argument_String filename;
		Argument_String specification;
		Argument_ImportCuesheetFrom import_cuesheet_from;
		Argument_ExportPictureTo export_picture_to;
		Argument_AddSeekpoint add_seekpoint;
		Argument_AddPadding add_padding;
	} argument;
} Operation;

typedef struct {
	ArgumentType type;
	union {
		Argument_BlockNumber block_number;
		Argument_BlockType block_type;
		Argument_DataFormat data_format;
		Argument_FromFile from_file;
	} value;
} Argument;

typedef struct {
	FLAC__bool preserve_modtime;
	FLAC__bool prefix_with_filename;
	FLAC__bool utf8_convert;
	FLAC__bool use_padding;
	FLAC__bool cued_seekpoints;
	FLAC__bool show_long_help;
	FLAC__bool show_version;
	FLAC__bool application_data_format_is_hexdump;
	struct {
		Operation *operations;
		unsigned num_operations;
		unsigned capacity;
	} ops;
	struct {
		struct {
			unsigned num_shorthand_ops;
			unsigned num_major_ops;
			FLAC__bool has_block_type;
			FLAC__bool has_except_block_type;
		} checks;
		Argument *arguments;
		unsigned num_arguments;
		unsigned capacity;
	} args;
	unsigned num_files;
	char **filenames;
} CommandLineOptions;

void init_options(CommandLineOptions *options);
FLAC__bool parse_options(int argc, char *argv[], CommandLineOptions *options);
void free_options(CommandLineOptions *options);

#endif
