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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "options.h"
#include "usage.h"
#include "utils.h"
#include "FLAC/assert.h"
#include "share/alloc.h"
#include "share/grabbag/replaygain.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
   share__getopt format struct; note we don't use short options so we just
   set the 'val' field to 0 everywhere to indicate a valid option.
*/
struct share__option long_options_[] = {
	/* global options */
	{ "preserve-modtime", 0, 0, 0 },
	{ "with-filename", 0, 0, 0 },
	{ "no-filename", 0, 0, 0 },
	{ "no-utf8-convert", 0, 0, 0 },
	{ "dont-use-padding", 0, 0, 0 },
	{ "no-cued-seekpoints", 0, 0, 0 },
	/* shorthand operations */
	{ "show-md5sum", 0, 0, 0 },
	{ "show-min-blocksize", 0, 0, 0 },
	{ "show-max-blocksize", 0, 0, 0 },
	{ "show-min-framesize", 0, 0, 0 },
	{ "show-max-framesize", 0, 0, 0 },
	{ "show-sample-rate", 0, 0, 0 },
	{ "show-channels", 0, 0, 0 },
	{ "show-bps", 0, 0, 0 },
	{ "show-total-samples", 0, 0, 0 },
	{ "set-md5sum", 1, 0, 0 }, /* undocumented */
	{ "set-min-blocksize", 1, 0, 0 }, /* undocumented */
	{ "set-max-blocksize", 1, 0, 0 }, /* undocumented */
	{ "set-min-framesize", 1, 0, 0 }, /* undocumented */
	{ "set-max-framesize", 1, 0, 0 }, /* undocumented */
	{ "set-sample-rate", 1, 0, 0 }, /* undocumented */
	{ "set-channels", 1, 0, 0 }, /* undocumented */
	{ "set-bps", 1, 0, 0 }, /* undocumented */
	{ "set-total-samples", 1, 0, 0 }, /* undocumented */ /* WATCHOUT: used by test/test_flac.sh on windows */
	{ "show-vendor-tag", 0, 0, 0 }, 
	{ "show-tag", 1, 0, 0 }, 
	{ "remove-all-tags", 0, 0, 0 }, 
	{ "remove-tag", 1, 0, 0 }, 
	{ "remove-first-tag", 1, 0, 0 }, 
	{ "set-tag", 1, 0, 0 }, 
	{ "set-tag-from-file", 1, 0, 0 }, 
	{ "import-tags-from", 1, 0, 0 }, 
	{ "export-tags-to", 1, 0, 0 }, 
	{ "import-cuesheet-from", 1, 0, 0 },
	{ "export-cuesheet-to", 1, 0, 0 },
	{ "import-picture-from", 1, 0, 0 },
	{ "export-picture-to", 1, 0, 0 },
	{ "add-seekpoint", 1, 0, 0 },
	{ "add-replay-gain", 0, 0, 0 },
	{ "remove-replay-gain", 0, 0, 0 },
	{ "add-padding", 1, 0, 0 },
	/* major operations */
	{ "help", 0, 0, 0 },
	{ "version", 0, 0, 0 },
	{ "list", 0, 0, 0 },
	{ "append", 0, 0, 0 },
	{ "remove", 0, 0, 0 },
	{ "remove-all", 0, 0, 0 },
	{ "merge-padding", 0, 0, 0 },
	{ "sort-padding", 0, 0, 0 },
	/* major operation arguments */
	{ "block-number", 1, 0, 0 },
	{ "block-type", 1, 0, 0 },
	{ "except-block-type", 1, 0, 0 },
	{ "data-format", 1, 0, 0 },
	{ "application-data-format", 1, 0, 0 },
	{ "from-file", 1, 0, 0 },
	{0, 0, 0, 0}
};

static FLAC__bool parse_option(int option_index, const char *option_argument, CommandLineOptions *options);
static void append_new_operation(CommandLineOptions *options, Operation operation);
static void append_new_argument(CommandLineOptions *options, Argument argument);
static Operation *append_major_operation(CommandLineOptions *options, OperationType type);
static Operation *append_shorthand_operation(CommandLineOptions *options, OperationType type);
static Argument *find_argument(CommandLineOptions *options, ArgumentType type);
static Operation *find_shorthand_operation(CommandLineOptions *options, OperationType type);
static Argument *append_argument(CommandLineOptions *options, ArgumentType type);
static FLAC__bool parse_md5(const char *src, FLAC__byte dest[16]);
static FLAC__bool parse_uint32(const char *src, FLAC__uint32 *dest);
static FLAC__bool parse_uint64(const char *src, FLAC__uint64 *dest);
static FLAC__bool parse_string(const char *src, char **dest);
static FLAC__bool parse_vorbis_comment_field_name(const char *field_ref, char **name, const char **violation);
static FLAC__bool parse_add_seekpoint(const char *in, char **out, const char **violation);
static FLAC__bool parse_add_padding(const char *in, unsigned *out);
static FLAC__bool parse_block_number(const char *in, Argument_BlockNumber *out);
static FLAC__bool parse_block_type(const char *in, Argument_BlockType *out);
static FLAC__bool parse_data_format(const char *in, Argument_DataFormat *out);
static FLAC__bool parse_application_data_format(const char *in, FLAC__bool *out);
static void undocumented_warning(const char *opt);


void init_options(CommandLineOptions *options)
{
	options->preserve_modtime = false;

	/* '2' is a hack to mean "use default if not forced on command line" */
	FLAC__ASSERT(true != 2);
	options->prefix_with_filename = 2;

	options->utf8_convert = true;
	options->use_padding = true;
	options->cued_seekpoints = true;
	options->show_long_help = false;
	options->show_version = false;
	options->application_data_format_is_hexdump = false;

	options->ops.operations = 0;
	options->ops.num_operations = 0;
	options->ops.capacity = 0;

	options->args.arguments = 0;
	options->args.num_arguments = 0;
	options->args.capacity = 0;

	options->args.checks.num_shorthand_ops = 0;
	options->args.checks.num_major_ops = 0;
	options->args.checks.has_block_type = false;
	options->args.checks.has_except_block_type = false;

	options->num_files = 0;
	options->filenames = 0;
}

FLAC__bool parse_options(int argc, char *argv[], CommandLineOptions *options)
{
	int ret;
	int option_index = 1;
	FLAC__bool had_error = false;

	while ((ret = share__getopt_long(argc, argv, "", long_options_, &option_index)) != -1) {
		switch (ret) {
			case 0:
				had_error |= !parse_option(option_index, share__optarg, options);
				break;
			case '?':
			case ':':
				had_error = true;
				break;
			default:
				FLAC__ASSERT(0);
				break;
		}
	}

	if(options->prefix_with_filename == 2)
		options->prefix_with_filename = (argc - share__optind > 1);

	if(share__optind >= argc && !options->show_long_help && !options->show_version) {
		fprintf(stderr,"ERROR: you must specify at least one FLAC file;\n");
		fprintf(stderr,"       metaflac cannot be used as a pipe\n");
		had_error = true;
	}

	options->num_files = argc - share__optind;

	if(options->num_files > 0) {
		unsigned i = 0;
		if(0 == (options->filenames = (char**)safe_malloc_mul_2op_(sizeof(char*), /*times*/options->num_files)))
			die("out of memory allocating space for file names list");
		while(share__optind < argc)
			options->filenames[i++] = local_strdup(argv[share__optind++]);
	}

	if(options->args.checks.num_major_ops > 0) {
		if(options->args.checks.num_major_ops > 1) {
			fprintf(stderr, "ERROR: you may only specify one major operation at a time\n");
			had_error = true;
		}
		else if(options->args.checks.num_shorthand_ops > 0) {
			fprintf(stderr, "ERROR: you may not mix shorthand and major operations\n");
			had_error = true;
		}
	}

	/* check for only one FLAC file used with certain options */
	if(options->num_files > 1) {
		if(0 != find_shorthand_operation(options, OP__IMPORT_CUESHEET_FROM)) {
			fprintf(stderr, "ERROR: you may only specify one FLAC file when using '--import-cuesheet-from'\n");
			had_error = true;
		}
		if(0 != find_shorthand_operation(options, OP__EXPORT_CUESHEET_TO)) {
			fprintf(stderr, "ERROR: you may only specify one FLAC file when using '--export-cuesheet-to'\n");
			had_error = true;
		}
		if(0 != find_shorthand_operation(options, OP__EXPORT_PICTURE_TO)) {
			fprintf(stderr, "ERROR: you may only specify one FLAC file when using '--export-picture-to'\n");
			had_error = true;
		}
		if(
			0 != find_shorthand_operation(options, OP__IMPORT_VC_FROM) &&
			0 == strcmp(find_shorthand_operation(options, OP__IMPORT_VC_FROM)->argument.filename.value, "-")
		) {
			fprintf(stderr, "ERROR: you may only specify one FLAC file when using '--import-tags-from=-'\n");
			had_error = true;
		}
	}

	if(options->args.checks.has_block_type && options->args.checks.has_except_block_type) {
		fprintf(stderr, "ERROR: you may not specify both '--block-type' and '--except-block-type'\n");
		had_error = true;
	}

	if(had_error)
		short_usage(0);

	/*
	 * We need to create an OP__ADD_SEEKPOINT operation if there is
	 * not one already, and --import-cuesheet-from was specified but
	 * --no-cued-seekpoints was not:
	 */
	if(options->cued_seekpoints) {
		Operation *op = find_shorthand_operation(options, OP__IMPORT_CUESHEET_FROM);
		if(0 != op) {
			Operation *op2 = find_shorthand_operation(options, OP__ADD_SEEKPOINT);
			if(0 == op2)
				op2 = append_shorthand_operation(options, OP__ADD_SEEKPOINT);
			op->argument.import_cuesheet_from.add_seekpoint_link = &(op2->argument.add_seekpoint);
		}
	}

	return !had_error;
}

void free_options(CommandLineOptions *options)
{
	unsigned i;
	Operation *op;
	Argument *arg;

	FLAC__ASSERT(0 == options->ops.operations || options->ops.num_operations > 0);
	FLAC__ASSERT(0 == options->args.arguments || options->args.num_arguments > 0);

	for(i = 0, op = options->ops.operations; i < options->ops.num_operations; i++, op++) {
		switch(op->type) {
			case OP__SHOW_VC_FIELD:
			case OP__REMOVE_VC_FIELD:
			case OP__REMOVE_VC_FIRSTFIELD:
				if(0 != op->argument.vc_field_name.value)
					free(op->argument.vc_field_name.value);
				break;
			case OP__SET_VC_FIELD:
				if(0 != op->argument.vc_field.field)
					free(op->argument.vc_field.field);
				if(0 != op->argument.vc_field.field_name)
					free(op->argument.vc_field.field_name);
				if(0 != op->argument.vc_field.field_value)
					free(op->argument.vc_field.field_value);
				break;
			case OP__IMPORT_VC_FROM:
			case OP__EXPORT_VC_TO:
			case OP__EXPORT_CUESHEET_TO:
				if(0 != op->argument.filename.value)
					free(op->argument.filename.value);
				break;
			case OP__IMPORT_CUESHEET_FROM:
				if(0 != op->argument.import_cuesheet_from.filename)
					free(op->argument.import_cuesheet_from.filename);
				break;
			case OP__IMPORT_PICTURE_FROM:
				if(0 != op->argument.specification.value)
					free(op->argument.specification.value);
				break;
			case OP__EXPORT_PICTURE_TO:
				if(0 != op->argument.export_picture_to.filename)
					free(op->argument.export_picture_to.filename);
				break;
			case OP__ADD_SEEKPOINT:
				if(0 != op->argument.add_seekpoint.specification)
					free(op->argument.add_seekpoint.specification);
				break;
			default:
				break;
		}
	}

	for(i = 0, arg = options->args.arguments; i < options->args.num_arguments; i++, arg++) {
		switch(arg->type) {
			case ARG__BLOCK_NUMBER:
				if(0 != arg->value.block_number.entries)
					free(arg->value.block_number.entries);
				break;
			case ARG__BLOCK_TYPE:
			case ARG__EXCEPT_BLOCK_TYPE:
				if(0 != arg->value.block_type.entries)
					free(arg->value.block_type.entries);
				break;
			case ARG__FROM_FILE:
				if(0 != arg->value.from_file.file_name)
					free(arg->value.from_file.file_name);
				break;
			default:
				break;
		}
	}

	if(0 != options->ops.operations)
		free(options->ops.operations);

	if(0 != options->args.arguments)
		free(options->args.arguments);

	if(0 != options->filenames) {
		for(i = 0; i < options->num_files; i++) {
			if(0 != options->filenames[i])
				free(options->filenames[i]);
		}
		free(options->filenames);
	}
}

/*
 * local routines
 */

FLAC__bool parse_option(int option_index, const char *option_argument, CommandLineOptions *options)
{
	const char *opt = long_options_[option_index].name;
	Operation *op;
	Argument *arg;
	FLAC__bool ok = true;

	if(0 == strcmp(opt, "preserve-modtime")) {
		options->preserve_modtime = true;
	}
	else if(0 == strcmp(opt, "with-filename")) {
		options->prefix_with_filename = true;
	}
	else if(0 == strcmp(opt, "no-filename")) {
		options->prefix_with_filename = false;
	}
	else if(0 == strcmp(opt, "no-utf8-convert")) {
		options->utf8_convert = false;
	}
	else if(0 == strcmp(opt, "dont-use-padding")) {
		options->use_padding = false;
	}
	else if(0 == strcmp(opt, "no-cued-seekpoints")) {
		options->cued_seekpoints = false;
	}
	else if(0 == strcmp(opt, "show-md5sum")) {
		(void) append_shorthand_operation(options, OP__SHOW_MD5SUM);
	}
	else if(0 == strcmp(opt, "show-min-blocksize")) {
		(void) append_shorthand_operation(options, OP__SHOW_MIN_BLOCKSIZE);
	}
	else if(0 == strcmp(opt, "show-max-blocksize")) {
		(void) append_shorthand_operation(options, OP__SHOW_MAX_BLOCKSIZE);
	}
	else if(0 == strcmp(opt, "show-min-framesize")) {
		(void) append_shorthand_operation(options, OP__SHOW_MIN_FRAMESIZE);
	}
	else if(0 == strcmp(opt, "show-max-framesize")) {
		(void) append_shorthand_operation(options, OP__SHOW_MAX_FRAMESIZE);
	}
	else if(0 == strcmp(opt, "show-sample-rate")) {
		(void) append_shorthand_operation(options, OP__SHOW_SAMPLE_RATE);
	}
	else if(0 == strcmp(opt, "show-channels")) {
		(void) append_shorthand_operation(options, OP__SHOW_CHANNELS);
	}
	else if(0 == strcmp(opt, "show-bps")) {
		(void) append_shorthand_operation(options, OP__SHOW_BPS);
	}
	else if(0 == strcmp(opt, "show-total-samples")) {
		(void) append_shorthand_operation(options, OP__SHOW_TOTAL_SAMPLES);
	}
	else if(0 == strcmp(opt, "set-md5sum")) {
		op = append_shorthand_operation(options, OP__SET_MD5SUM);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_md5(option_argument, op->argument.streaminfo_md5.value)) {
			fprintf(stderr, "ERROR (--%s): bad MD5 sum\n", opt);
			ok = false;
		}
		else
			undocumented_warning(opt);
	}
	else if(0 == strcmp(opt, "set-min-blocksize")) {
		op = append_shorthand_operation(options, OP__SET_MIN_BLOCKSIZE);
		if(!parse_uint32(option_argument, &(op->argument.streaminfo_uint32.value)) || op->argument.streaminfo_uint32.value < FLAC__MIN_BLOCK_SIZE || op->argument.streaminfo_uint32.value > FLAC__MAX_BLOCK_SIZE) {
			fprintf(stderr, "ERROR (--%s): value must be >= %u and <= %u\n", opt, FLAC__MIN_BLOCK_SIZE, FLAC__MAX_BLOCK_SIZE);
			ok = false;
		}
		else
			undocumented_warning(opt);
	}
	else if(0 == strcmp(opt, "set-max-blocksize")) {
		op = append_shorthand_operation(options, OP__SET_MAX_BLOCKSIZE);
		if(!parse_uint32(option_argument, &(op->argument.streaminfo_uint32.value)) || op->argument.streaminfo_uint32.value < FLAC__MIN_BLOCK_SIZE || op->argument.streaminfo_uint32.value > FLAC__MAX_BLOCK_SIZE) {
			fprintf(stderr, "ERROR (--%s): value must be >= %u and <= %u\n", opt, FLAC__MIN_BLOCK_SIZE, FLAC__MAX_BLOCK_SIZE);
			ok = false;
		}
		else
			undocumented_warning(opt);
	}
	else if(0 == strcmp(opt, "set-min-framesize")) {
		op = append_shorthand_operation(options, OP__SET_MIN_FRAMESIZE);
		if(!parse_uint32(option_argument, &(op->argument.streaminfo_uint32.value)) || op->argument.streaminfo_uint32.value >= (1u<<FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN)) {
			fprintf(stderr, "ERROR (--%s): value must be a %u-bit unsigned integer\n", opt, FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN);
			ok = false;
		}
		else
			undocumented_warning(opt);
	}
	else if(0 == strcmp(opt, "set-max-framesize")) {
		op = append_shorthand_operation(options, OP__SET_MAX_FRAMESIZE);
		if(!parse_uint32(option_argument, &(op->argument.streaminfo_uint32.value)) || op->argument.streaminfo_uint32.value >= (1u<<FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN)) {
			fprintf(stderr, "ERROR (--%s): value must be a %u-bit unsigned integer\n", opt, FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN);
			ok = false;
		}
		else
			undocumented_warning(opt);
	}
	else if(0 == strcmp(opt, "set-sample-rate")) {
		op = append_shorthand_operation(options, OP__SET_SAMPLE_RATE);
		if(!parse_uint32(option_argument, &(op->argument.streaminfo_uint32.value)) || !FLAC__format_sample_rate_is_valid(op->argument.streaminfo_uint32.value)) {
			fprintf(stderr, "ERROR (--%s): invalid sample rate\n", opt);
			ok = false;
		}
		else
			undocumented_warning(opt);
	}
	else if(0 == strcmp(opt, "set-channels")) {
		op = append_shorthand_operation(options, OP__SET_CHANNELS);
		if(!parse_uint32(option_argument, &(op->argument.streaminfo_uint32.value)) || op->argument.streaminfo_uint32.value > FLAC__MAX_CHANNELS) {
			fprintf(stderr, "ERROR (--%s): value must be > 0 and <= %u\n", opt, FLAC__MAX_CHANNELS);
			ok = false;
		}
		else
			undocumented_warning(opt);
	}
	else if(0 == strcmp(opt, "set-bps")) {
		op = append_shorthand_operation(options, OP__SET_BPS);
		if(!parse_uint32(option_argument, &(op->argument.streaminfo_uint32.value)) || op->argument.streaminfo_uint32.value < FLAC__MIN_BITS_PER_SAMPLE || op->argument.streaminfo_uint32.value > FLAC__MAX_BITS_PER_SAMPLE) {
			fprintf(stderr, "ERROR (--%s): value must be >= %u and <= %u\n", opt, FLAC__MIN_BITS_PER_SAMPLE, FLAC__MAX_BITS_PER_SAMPLE);
			ok = false;
		}
		else
			undocumented_warning(opt);
	}
	else if(0 == strcmp(opt, "set-total-samples")) {
		op = append_shorthand_operation(options, OP__SET_TOTAL_SAMPLES);
		if(!parse_uint64(option_argument, &(op->argument.streaminfo_uint64.value)) || op->argument.streaminfo_uint64.value >= (((FLAC__uint64)1)<<FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN)) {
			fprintf(stderr, "ERROR (--%s): value must be a %u-bit unsigned integer\n", opt, FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN);
			ok = false;
		}
		else
			undocumented_warning(opt);
	}
	else if(0 == strcmp(opt, "show-vendor-tag")) {
		(void) append_shorthand_operation(options, OP__SHOW_VC_VENDOR);
	}
	else if(0 == strcmp(opt, "show-tag")) {
		const char *violation;
		op = append_shorthand_operation(options, OP__SHOW_VC_FIELD);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_vorbis_comment_field_name(option_argument, &(op->argument.vc_field_name.value), &violation)) {
			FLAC__ASSERT(0 != violation);
			fprintf(stderr, "ERROR (--%s): malformed vorbis comment field name \"%s\",\n       %s\n", opt, option_argument, violation);
			ok = false;
		}
	}
	else if(0 == strcmp(opt, "remove-all-tags")) {
		(void) append_shorthand_operation(options, OP__REMOVE_VC_ALL);
	}
	else if(0 == strcmp(opt, "remove-tag")) {
		const char *violation;
		op = append_shorthand_operation(options, OP__REMOVE_VC_FIELD);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_vorbis_comment_field_name(option_argument, &(op->argument.vc_field_name.value), &violation)) {
			FLAC__ASSERT(0 != violation);
			fprintf(stderr, "ERROR (--%s): malformed vorbis comment field name \"%s\",\n       %s\n", opt, option_argument, violation);
			ok = false;
		}
	}
	else if(0 == strcmp(opt, "remove-first-tag")) {
		const char *violation;
		op = append_shorthand_operation(options, OP__REMOVE_VC_FIRSTFIELD);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_vorbis_comment_field_name(option_argument, &(op->argument.vc_field_name.value), &violation)) {
			FLAC__ASSERT(0 != violation);
			fprintf(stderr, "ERROR (--%s): malformed vorbis comment field name \"%s\",\n       %s\n", opt, option_argument, violation);
			ok = false;
		}
	}
	else if(0 == strcmp(opt, "set-tag")) {
		const char *violation;
		op = append_shorthand_operation(options, OP__SET_VC_FIELD);
		FLAC__ASSERT(0 != option_argument);
		op->argument.vc_field.field_value_from_file = false;
		if(!parse_vorbis_comment_field(option_argument, &(op->argument.vc_field.field), &(op->argument.vc_field.field_name), &(op->argument.vc_field.field_value), &(op->argument.vc_field.field_value_length), &violation)) {
			FLAC__ASSERT(0 != violation);
			fprintf(stderr, "ERROR (--%s): malformed vorbis comment field \"%s\",\n       %s\n", opt, option_argument, violation);
			ok = false;
		}
	}
	else if(0 == strcmp(opt, "set-tag-from-file")) {
		const char *violation;
		op = append_shorthand_operation(options, OP__SET_VC_FIELD);
		FLAC__ASSERT(0 != option_argument);
		op->argument.vc_field.field_value_from_file = true;
		if(!parse_vorbis_comment_field(option_argument, &(op->argument.vc_field.field), &(op->argument.vc_field.field_name), &(op->argument.vc_field.field_value), &(op->argument.vc_field.field_value_length), &violation)) {
			FLAC__ASSERT(0 != violation);
			fprintf(stderr, "ERROR (--%s): malformed vorbis comment field \"%s\",\n       %s\n", opt, option_argument, violation);
			ok = false;
		}
	}
	else if(0 == strcmp(opt, "import-tags-from")) {
		op = append_shorthand_operation(options, OP__IMPORT_VC_FROM);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_string(option_argument, &(op->argument.filename.value))) {
			fprintf(stderr, "ERROR (--%s): missing filename\n", opt);
			ok = false;
		}
	}
	else if(0 == strcmp(opt, "export-tags-to")) {
		op = append_shorthand_operation(options, OP__EXPORT_VC_TO);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_string(option_argument, &(op->argument.filename.value))) {
			fprintf(stderr, "ERROR (--%s): missing filename\n", opt);
			ok = false;
		}
	}
	else if(0 == strcmp(opt, "import-cuesheet-from")) {
		if(0 != find_shorthand_operation(options, OP__IMPORT_CUESHEET_FROM)) {
			fprintf(stderr, "ERROR (--%s): may be specified only once\n", opt);
			ok = false;
		}
		op = append_shorthand_operation(options, OP__IMPORT_CUESHEET_FROM);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_string(option_argument, &(op->argument.import_cuesheet_from.filename))) {
			fprintf(stderr, "ERROR (--%s): missing filename\n", opt);
			ok = false;
		}
	}
	else if(0 == strcmp(opt, "export-cuesheet-to")) {
		op = append_shorthand_operation(options, OP__EXPORT_CUESHEET_TO);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_string(option_argument, &(op->argument.filename.value))) {
			fprintf(stderr, "ERROR (--%s): missing filename\n", opt);
			ok = false;
		}
	}
	else if(0 == strcmp(opt, "import-picture-from")) {
		op = append_shorthand_operation(options, OP__IMPORT_PICTURE_FROM);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_string(option_argument, &(op->argument.specification.value))) {
			fprintf(stderr, "ERROR (--%s): missing specification\n", opt);
			ok = false;
		}
	}
	else if(0 == strcmp(opt, "export-picture-to")) {
		const Argument *arg = find_argument(options, ARG__BLOCK_NUMBER);
		op = append_shorthand_operation(options, OP__EXPORT_PICTURE_TO);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_string(option_argument, &(op->argument.export_picture_to.filename))) {
			fprintf(stderr, "ERROR (--%s): missing filename\n", opt);
			ok = false;
		}
		op->argument.export_picture_to.block_number_link = arg? &(arg->value.block_number) : 0;
	}
	else if(0 == strcmp(opt, "add-seekpoint")) {
		const char *violation;
		char *spec;
		FLAC__ASSERT(0 != option_argument);
		if(!parse_add_seekpoint(option_argument, &spec, &violation)) {
			FLAC__ASSERT(0 != violation);
			fprintf(stderr, "ERROR (--%s): malformed seekpoint specification \"%s\",\n       %s\n", opt, option_argument, violation);
			ok = false;
		}
		else {
			op = find_shorthand_operation(options, OP__ADD_SEEKPOINT);
			if(0 == op)
				op = append_shorthand_operation(options, OP__ADD_SEEKPOINT);
			local_strcat(&(op->argument.add_seekpoint.specification), spec);
			local_strcat(&(op->argument.add_seekpoint.specification), ";");
			free(spec);
		}
	}
	else if(0 == strcmp(opt, "add-replay-gain")) {
		(void) append_shorthand_operation(options, OP__ADD_REPLAY_GAIN);
	}
	else if(0 == strcmp(opt, "remove-replay-gain")) {
		const FLAC__byte * const tags[5] = {
			GRABBAG__REPLAYGAIN_TAG_REFERENCE_LOUDNESS,
			GRABBAG__REPLAYGAIN_TAG_TITLE_GAIN,
			GRABBAG__REPLAYGAIN_TAG_TITLE_PEAK,
			GRABBAG__REPLAYGAIN_TAG_ALBUM_GAIN,
			GRABBAG__REPLAYGAIN_TAG_ALBUM_PEAK
		};
		size_t i;
		for(i = 0; i < sizeof(tags)/sizeof(tags[0]); i++) {
			op = append_shorthand_operation(options, OP__REMOVE_VC_FIELD);
			op->argument.vc_field_name.value = local_strdup((const char *)tags[i]);
		}
	}
	else if(0 == strcmp(opt, "add-padding")) {
		op = append_shorthand_operation(options, OP__ADD_PADDING);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_add_padding(option_argument, &(op->argument.add_padding.length))) {
			fprintf(stderr, "ERROR (--%s): illegal length \"%s\", length must be >= 0 and < 2^%u\n", opt, option_argument, FLAC__STREAM_METADATA_LENGTH_LEN);
			ok = false;
		}
	}
	else if(0 == strcmp(opt, "help")) {
		options->show_long_help = true;
	}
	else if(0 == strcmp(opt, "version")) {
		options->show_version = true;
	}
	else if(0 == strcmp(opt, "list")) {
		(void) append_major_operation(options, OP__LIST);
	}
	else if(0 == strcmp(opt, "append")) {
		(void) append_major_operation(options, OP__APPEND);
	}
	else if(0 == strcmp(opt, "remove")) {
		(void) append_major_operation(options, OP__REMOVE);
	}
	else if(0 == strcmp(opt, "remove-all")) {
		(void) append_major_operation(options, OP__REMOVE_ALL);
	}
	else if(0 == strcmp(opt, "merge-padding")) {
		(void) append_major_operation(options, OP__MERGE_PADDING);
	}
	else if(0 == strcmp(opt, "sort-padding")) {
		(void) append_major_operation(options, OP__SORT_PADDING);
	}
	else if(0 == strcmp(opt, "block-number")) {
		arg = append_argument(options, ARG__BLOCK_NUMBER);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_block_number(option_argument, &(arg->value.block_number))) {
			fprintf(stderr, "ERROR: malformed block number specification \"%s\"\n", option_argument);
			ok = false;
		}
	}
	else if(0 == strcmp(opt, "block-type")) {
		arg = append_argument(options, ARG__BLOCK_TYPE);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_block_type(option_argument, &(arg->value.block_type))) {
			fprintf(stderr, "ERROR (--%s): malformed block type specification \"%s\"\n", opt, option_argument);
			ok = false;
		}
		options->args.checks.has_block_type = true;
	}
	else if(0 == strcmp(opt, "except-block-type")) {
		arg = append_argument(options, ARG__EXCEPT_BLOCK_TYPE);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_block_type(option_argument, &(arg->value.block_type))) {
			fprintf(stderr, "ERROR (--%s): malformed block type specification \"%s\"\n", opt, option_argument);
			ok = false;
		}
		options->args.checks.has_except_block_type = true;
	}
	else if(0 == strcmp(opt, "data-format")) {
		arg = append_argument(options, ARG__DATA_FORMAT);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_data_format(option_argument, &(arg->value.data_format))) {
			fprintf(stderr, "ERROR (--%s): illegal data format \"%s\"\n", opt, option_argument);
			ok = false;
		}
	}
	else if(0 == strcmp(opt, "application-data-format")) {
		FLAC__ASSERT(0 != option_argument);
		if(!parse_application_data_format(option_argument, &(options->application_data_format_is_hexdump))) {
			fprintf(stderr, "ERROR (--%s): illegal application data format \"%s\"\n", opt, option_argument);
			ok = false;
		}
	}
	else if(0 == strcmp(opt, "from-file")) {
		arg = append_argument(options, ARG__FROM_FILE);
		FLAC__ASSERT(0 != option_argument);
		arg->value.from_file.file_name = local_strdup(option_argument);
	}
	else {
		FLAC__ASSERT(0);
	}

	return ok;
}

void append_new_operation(CommandLineOptions *options, Operation operation)
{
	if(options->ops.capacity == 0) {
		options->ops.capacity = 50;
		if(0 == (options->ops.operations = (Operation*)malloc(sizeof(Operation) * options->ops.capacity)))
			die("out of memory allocating space for option list");
		memset(options->ops.operations, 0, sizeof(Operation) * options->ops.capacity);
	}
	if(options->ops.capacity <= options->ops.num_operations) {
		unsigned original_capacity = options->ops.capacity;
		if(options->ops.capacity > SIZE_MAX / 2) /* overflow check */
			die("out of memory allocating space for option list");
		options->ops.capacity *= 2;
		if(0 == (options->ops.operations = (Operation*)safe_realloc_mul_2op_(options->ops.operations, sizeof(Operation), /*times*/options->ops.capacity)))
			die("out of memory allocating space for option list");
		memset(options->ops.operations + original_capacity, 0, sizeof(Operation) * (options->ops.capacity - original_capacity));
	}

	options->ops.operations[options->ops.num_operations++] = operation;
}

void append_new_argument(CommandLineOptions *options, Argument argument)
{
	if(options->args.capacity == 0) {
		options->args.capacity = 50;
		if(0 == (options->args.arguments = (Argument*)malloc(sizeof(Argument) * options->args.capacity)))
			die("out of memory allocating space for option list");
		memset(options->args.arguments, 0, sizeof(Argument) * options->args.capacity);
	}
	if(options->args.capacity <= options->args.num_arguments) {
		unsigned original_capacity = options->args.capacity;
		if(options->args.capacity > SIZE_MAX / 2) /* overflow check */
			die("out of memory allocating space for option list");
		options->args.capacity *= 2;
		if(0 == (options->args.arguments = (Argument*)safe_realloc_mul_2op_(options->args.arguments, sizeof(Argument), /*times*/options->args.capacity)))
			die("out of memory allocating space for option list");
		memset(options->args.arguments + original_capacity, 0, sizeof(Argument) * (options->args.capacity - original_capacity));
	}

	options->args.arguments[options->args.num_arguments++] = argument;
}

Operation *append_major_operation(CommandLineOptions *options, OperationType type)
{
	Operation op;
	memset(&op, 0, sizeof(op));
	op.type = type;
	append_new_operation(options, op);
	options->args.checks.num_major_ops++;
	return options->ops.operations + (options->ops.num_operations - 1);
}

Operation *append_shorthand_operation(CommandLineOptions *options, OperationType type)
{
	Operation op;
	memset(&op, 0, sizeof(op));
	op.type = type;
	append_new_operation(options, op);
	options->args.checks.num_shorthand_ops++;
	return options->ops.operations + (options->ops.num_operations - 1);
}

Argument *find_argument(CommandLineOptions *options, ArgumentType type)
{
	unsigned i;
	for(i = 0; i < options->args.num_arguments; i++)
		if(options->args.arguments[i].type == type)
			return &options->args.arguments[i];
	return 0;
}

Operation *find_shorthand_operation(CommandLineOptions *options, OperationType type)
{
	unsigned i;
	for(i = 0; i < options->ops.num_operations; i++)
		if(options->ops.operations[i].type == type)
			return &options->ops.operations[i];
	return 0;
}

Argument *append_argument(CommandLineOptions *options, ArgumentType type)
{
	Argument arg;
	memset(&arg, 0, sizeof(arg));
	arg.type = type;
	append_new_argument(options, arg);
	return options->args.arguments + (options->args.num_arguments - 1);
}

FLAC__bool parse_md5(const char *src, FLAC__byte dest[16])
{
	unsigned i, d;
	int c;
	FLAC__ASSERT(0 != src);
	if(strlen(src) != 32)
		return false;
	/* strtoul() accepts negative numbers which we do not want, so we do it the hard way */
	for(i = 0; i < 16; i++) {
		c = (int)(*src++);
		if(isdigit(c))
			d = (unsigned)(c - '0');
		else if(c >= 'a' && c <= 'f')
			d = (unsigned)(c - 'a') + 10u;
		else if(c >= 'A' && c <= 'F')
			d = (unsigned)(c - 'A') + 10u;
		else
			return false;
		d <<= 4;
		c = (int)(*src++);
		if(isdigit(c))
			d |= (unsigned)(c - '0');
		else if(c >= 'a' && c <= 'f')
			d |= (unsigned)(c - 'a') + 10u;
		else if(c >= 'A' && c <= 'F')
			d |= (unsigned)(c - 'A') + 10u;
		else
			return false;
		dest[i] = (FLAC__byte)d;
	}
	return true;
}

FLAC__bool parse_uint32(const char *src, FLAC__uint32 *dest)
{
	FLAC__ASSERT(0 != src);
	if(strlen(src) == 0 || strspn(src, "0123456789") != strlen(src))
		return false;
	*dest = strtoul(src, 0, 10);
	return true;
}

#ifdef _MSC_VER
/* There's no strtoull() in MSVC6 so we just write a specialized one */
static FLAC__uint64 local__strtoull(const char *src)
{
	FLAC__uint64 ret = 0;
	int c;
	FLAC__ASSERT(0 != src);
	while(0 != (c = *src++)) {
		c -= '0';
		if(c >= 0 && c <= 9)
			ret = (ret * 10) + c;
		else
			break;
	}
	return ret;
}
#endif

FLAC__bool parse_uint64(const char *src, FLAC__uint64 *dest)
{
	FLAC__ASSERT(0 != src);
	if(strlen(src) == 0 || strspn(src, "0123456789") != strlen(src))
		return false;
#ifdef _MSC_VER
	*dest = local__strtoull(src);
#else
	*dest = strtoull(src, 0, 10);
#endif
	return true;
}

FLAC__bool parse_string(const char *src, char **dest)
{
	if(0 == src || strlen(src) == 0)
		return false;
	*dest = strdup(src);
	return true;
}

FLAC__bool parse_vorbis_comment_field_name(const char *field_ref, char **name, const char **violation)
{
	static const char * const violations[] = {
		"field name contains invalid character"
	};

	char *q, *s;

	s = local_strdup(field_ref);

	for(q = s; *q; q++) {
		if(*q < 0x20 || *q > 0x7d || *q == 0x3d) {
			free(s);
			*violation = violations[0];
			return false;
		}
	}

	*name = s;

	return true;
}

FLAC__bool parse_add_seekpoint(const char *in, char **out, const char **violation)
{
	static const char *garbled_ = "garbled specification";
	const unsigned n = strlen(in);

	FLAC__ASSERT(0 != in);
	FLAC__ASSERT(0 != out);

	if(n == 0) {
		*violation = "specification is empty";
		return false;
	}

	if(n > strspn(in, "0123456789.Xsx")) {
		*violation = "specification contains invalid character";
		return false;
	}

	if(in[n-1] == 'X') {
		if(n > 1) {
			*violation = garbled_;
			return false;
		}
	}
	else if(in[n-1] == 's') {
		if(n-1 > strspn(in, "0123456789.")) {
			*violation = garbled_;
			return false;
		}
	}
	else if(in[n-1] == 'x') {
		if(n-1 > strspn(in, "0123456789")) {
			*violation = garbled_;
			return false;
		}
	}
	else {
		if(n > strspn(in, "0123456789")) {
			*violation = garbled_;
			return false;
		}
	}

	*out = local_strdup(in);
	return true;
}

FLAC__bool parse_add_padding(const char *in, unsigned *out)
{
	FLAC__ASSERT(0 != in);
	FLAC__ASSERT(0 != out);
	*out = (unsigned)strtoul(in, 0, 10);
	return *out < (1u << FLAC__STREAM_METADATA_LENGTH_LEN);
}

FLAC__bool parse_block_number(const char *in, Argument_BlockNumber *out)
{
	char *p, *q, *s, *end;
	long i;
	unsigned entry;

	if(*in == '\0')
		return false;

	s = local_strdup(in);

	/* first count the entries */
	for(out->num_entries = 1, p = strchr(s, ','); p; out->num_entries++, p = strchr(++p, ','))
		;

	/* make space */
	FLAC__ASSERT(out->num_entries > 0);
	if(0 == (out->entries = (unsigned*)safe_malloc_mul_2op_(sizeof(unsigned), /*times*/out->num_entries)))
		die("out of memory allocating space for option list");

	/* load 'em up */
	entry = 0;
	q = s;
	while(q) {
		FLAC__ASSERT(entry < out->num_entries);
		if(0 != (p = strchr(q, ',')))
			*p++ = '\0';
		if(!isdigit((int)(*q)) || (i = strtol(q, &end, 10)) < 0 || *end) {
			free(s);
			return false;
		}
		out->entries[entry++] = (unsigned)i;
		q = p;
	}
	FLAC__ASSERT(entry == out->num_entries);

	free(s);
	return true;
}

FLAC__bool parse_block_type(const char *in, Argument_BlockType *out)
{
	char *p, *q, *r, *s;
	unsigned entry;

	if(*in == '\0')
		return false;

	s = local_strdup(in);

	/* first count the entries */
	for(out->num_entries = 1, p = strchr(s, ','); p; out->num_entries++, p = strchr(++p, ','))
		;

	/* make space */
	FLAC__ASSERT(out->num_entries > 0);
	if(0 == (out->entries = (Argument_BlockTypeEntry*)safe_malloc_mul_2op_(sizeof(Argument_BlockTypeEntry), /*times*/out->num_entries)))
		die("out of memory allocating space for option list");

	/* load 'em up */
	entry = 0;
	q = s;
	while(q) {
		FLAC__ASSERT(entry < out->num_entries);
		if(0 != (p = strchr(q, ',')))
			*p++ = 0;
		r = strchr(q, ':');
		if(r)
			*r++ = '\0';
		if(0 != r && 0 != strcmp(q, "APPLICATION")) {
			free(s);
			return false;
		}
		if(0 == strcmp(q, "STREAMINFO")) {
			out->entries[entry++].type = FLAC__METADATA_TYPE_STREAMINFO;
		}
		else if(0 == strcmp(q, "PADDING")) {
			out->entries[entry++].type = FLAC__METADATA_TYPE_PADDING;
		}
		else if(0 == strcmp(q, "APPLICATION")) {
			out->entries[entry].type = FLAC__METADATA_TYPE_APPLICATION;
			out->entries[entry].filter_application_by_id = (0 != r);
			if(0 != r) {
				if(strlen(r) == 4) {
					strcpy(out->entries[entry].application_id, r);
				}
				else if(strlen(r) == 10 && strncmp(r, "0x", 2) == 0 && strspn(r+2, "0123456789ABCDEFabcdef") == 8) {
					FLAC__uint32 x = strtoul(r+2, 0, 16);
					out->entries[entry].application_id[3] = (FLAC__byte)(x & 0xff);
					out->entries[entry].application_id[2] = (FLAC__byte)((x>>=8) & 0xff);
					out->entries[entry].application_id[1] = (FLAC__byte)((x>>=8) & 0xff);
					out->entries[entry].application_id[0] = (FLAC__byte)((x>>=8) & 0xff);
				}
				else {
					free(s);
					return false;
				}
			}
			entry++;
		}
		else if(0 == strcmp(q, "SEEKTABLE")) {
			out->entries[entry++].type = FLAC__METADATA_TYPE_SEEKTABLE;
		}
		else if(0 == strcmp(q, "VORBIS_COMMENT")) {
			out->entries[entry++].type = FLAC__METADATA_TYPE_VORBIS_COMMENT;
		}
		else if(0 == strcmp(q, "CUESHEET")) {
			out->entries[entry++].type = FLAC__METADATA_TYPE_CUESHEET;
		}
		else if(0 == strcmp(q, "PICTURE")) {
			out->entries[entry++].type = FLAC__METADATA_TYPE_PICTURE;
		}
		else {
			free(s);
			return false;
		}
		q = p;
	}
	FLAC__ASSERT(entry == out->num_entries);

	free(s);
	return true;
}

FLAC__bool parse_data_format(const char *in, Argument_DataFormat *out)
{
	if(0 == strcmp(in, "binary"))
		out->is_binary = true;
	else if(0 == strcmp(in, "text"))
		out->is_binary = false;
	else
		return false;
	return true;
}

FLAC__bool parse_application_data_format(const char *in, FLAC__bool *out)
{
	if(0 == strcmp(in, "hexdump"))
		*out = true;
	else if(0 == strcmp(in, "text"))
		*out = false;
	else
		return false;
	return true;
}

void undocumented_warning(const char *opt)
{
	fprintf(stderr, "WARNING: undocmented option --%s should be used with caution,\n         only for repairing a damaged STREAMINFO block\n", opt);
}
