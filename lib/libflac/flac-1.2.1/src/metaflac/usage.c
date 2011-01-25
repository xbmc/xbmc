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

#include "usage.h"
#include "FLAC/format.h"
#include <stdarg.h>
#include <stdio.h>

static void usage_header(FILE *out)
{
	fprintf(out, "==============================================================================\n");
	fprintf(out, "metaflac - Command-line FLAC metadata editor version %s\n", FLAC__VERSION_STRING);
	fprintf(out, "Copyright (C) 2001,2002,2003,2004,2005,2006,2007  Josh Coalson\n");
	fprintf(out, "\n");
	fprintf(out, "This program is free software; you can redistribute it and/or\n");
	fprintf(out, "modify it under the terms of the GNU General Public License\n");
	fprintf(out, "as published by the Free Software Foundation; either version 2\n");
	fprintf(out, "of the License, or (at your option) any later version.\n");
	fprintf(out, "\n");
	fprintf(out, "This program is distributed in the hope that it will be useful,\n");
	fprintf(out, "but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	fprintf(out, "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
	fprintf(out, "GNU General Public License for more details.\n");
	fprintf(out, "\n");
	fprintf(out, "You should have received a copy of the GNU General Public License\n");
	fprintf(out, "along with this program; if not, write to the Free Software\n");
	fprintf(out, "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.\n");
	fprintf(out, "==============================================================================\n");
}

static void usage_summary(FILE *out)
{
	fprintf(out, "Usage:\n");
	fprintf(out, "  metaflac [options] [operations] FLACfile [FLACfile ...]\n");
	fprintf(out, "\n");
	fprintf(out, "Use metaflac to list, add, remove, or edit metadata in one or more FLAC files.\n");
	fprintf(out, "You may perform one major operation, or many shorthand operations at a time.\n");
	fprintf(out, "\n");
	fprintf(out, "Options:\n");
	fprintf(out, "--preserve-modtime    Preserve the original modification time in spite of edits\n");
	fprintf(out, "--with-filename       Prefix each output line with the FLAC file name\n");
	fprintf(out, "                      (the default if more than one FLAC file is specified)\n");
	fprintf(out, "--no-filename         Do not prefix each output line with the FLAC file name\n");
	fprintf(out, "                      (the default if only one FLAC file is specified)\n");
	fprintf(out, "--no-utf8-convert     Do not convert tags from UTF-8 to local charset,\n");
	fprintf(out, "                      or vice versa.  This is useful for scripts, and setting\n");
	fprintf(out, "                      tags in situations where the locale is wrong.\n");
	fprintf(out, "--dont-use-padding    By default metaflac tries to use padding where possible\n");
	fprintf(out, "                      to avoid rewriting the entire file if the metadata size\n");
	fprintf(out, "                      changes.  Use this option to tell metaflac to not take\n");
	fprintf(out, "                      advantage of padding this way.\n");
}

int short_usage(const char *message, ...)
{
	va_list args;

	if(message) {
		va_start(args, message);

		(void) vfprintf(stderr, message, args);

		va_end(args);

	}
	usage_header(stderr);
	fprintf(stderr, "\n");
	fprintf(stderr, "This is the short help; for full help use 'metaflac --help'\n");
	fprintf(stderr, "\n");
	usage_summary(stderr);

	return message? 1 : 0;
}

int long_usage(const char *message, ...)
{
	FILE *out = (message? stderr : stdout);
	va_list args;

	if(message) {
		va_start(args, message);

		(void) vfprintf(stderr, message, args);

		va_end(args);

	}
	usage_header(out);
	fprintf(out, "\n");
	usage_summary(out);
	fprintf(out, "\n");
	fprintf(out, "Shorthand operations:\n");
	fprintf(out, "--show-md5sum         Show the MD5 signature from the STREAMINFO block.\n");
	fprintf(out, "--show-min-blocksize  Show the minimum block size from the STREAMINFO block.\n");
	fprintf(out, "--show-max-blocksize  Show the maximum block size from the STREAMINFO block.\n");
	fprintf(out, "--show-min-framesize  Show the minimum frame size from the STREAMINFO block.\n");
	fprintf(out, "--show-max-framesize  Show the maximum frame size from the STREAMINFO block.\n");
	fprintf(out, "--show-sample-rate    Show the sample rate from the STREAMINFO block.\n");
	fprintf(out, "--show-channels       Show the number of channels from the STREAMINFO block.\n");
	fprintf(out, "--show-bps            Show the # of bits per sample from the STREAMINFO block.\n");
	fprintf(out, "--show-total-samples  Show the total # of samples from the STREAMINFO block.\n");
	fprintf(out, "\n");
	fprintf(out, "--show-vendor-tag     Show the vendor string from the VORBIS_COMMENT block.\n");
	fprintf(out, "--show-tag=NAME       Show all tags where the the field name matches 'NAME'.\n");
	fprintf(out, "--remove-tag=NAME     Remove all tags whose field name is 'NAME'.\n");
	fprintf(out, "--remove-first-tag=NAME  Remove first tag whose field name is 'NAME'.\n");
	fprintf(out, "--remove-all-tags     Remove all tags, leaving only the vendor string.\n");
	fprintf(out, "--set-tag=FIELD       Add a tag.  The FIELD must comply with the Vorbis comment\n");
	fprintf(out, "                      spec, of the form \"NAME=VALUE\".  If there is currently\n");
	fprintf(out, "                      no tag block, one will be created.\n");
	fprintf(out, "--set-tag-from-file=FIELD   Like --set-tag, except the VALUE is a filename\n");
	fprintf(out, "                      whose contents will be read verbatim to set the tag value.\n");
	fprintf(out, "                      Unless --no-utf8-convert is specified, the contents will\n");
	fprintf(out, "                      be converted to UTF-8 from the local charset.  This can\n");
	fprintf(out, "                      be used to store a cuesheet in a tag (e.g.\n");
	fprintf(out, "                      --set-tag-from-file=\"CUESHEET=image.cue\").  Do not try\n");
	fprintf(out, "                      to store binary data in tag fields!  Use APPLICATION\n");
	fprintf(out, "                      blocks for that.\n");
	fprintf(out, "--import-tags-from=FILE Import tags from a file.  Use '-' for stdin.  Each line\n");
	fprintf(out, "                      should be of the form NAME=VALUE.  Multi-line comments\n");
	fprintf(out, "                      are currently not supported.  Specify --remove-all-tags\n");
	fprintf(out, "                      and/or --no-utf8-convert before --import-tags-from if\n");
	fprintf(out, "                      necessary.  If FILE is '-' (stdin), only one FLAC file\n");
	fprintf(out, "                      may be specified.\n");
	fprintf(out, "--export-tags-to=FILE Export tags to a file.  Use '-' for stdout.  Each line\n");
	fprintf(out, "                      will be of the form NAME=VALUE.  Specify\n");
	fprintf(out, "                      --no-utf8-convert if necessary.\n");
	fprintf(out, "--import-cuesheet-from=FILE  Import a cuesheet from a file.  Use '-' for stdin.\n");
	fprintf(out, "                      Only one FLAC file may be specified.  A seekpoint will be\n");
	fprintf(out, "                      added for each index point in the cuesheet to the\n");
	fprintf(out, "                      SEEKTABLE unless --no-cued-seekpoints is specified.\n");
	fprintf(out, "--export-cuesheet-to=FILE  Export CUESHEET block to a cuesheet file, suitable\n");
	fprintf(out, "                      for use by CD authoring software.  Use '-' for stdout.\n");
	fprintf(out, "                      Only one FLAC file may be specified on the command line.\n");
	fprintf(out, "--import-picture-from=FILENAME|SPECIFICATION  Import a picture and store it in a\n");
	fprintf(out, "                      PICTURE block.  Either a filename for the picture file or\n");
	fprintf(out, "                      a more complete specification form can be used.  The\n");
	fprintf(out, "                      SPECIFICATION is a string whose parts are separated by |\n");
	fprintf(out, "                      characters.  Some parts may be left empty to invoke\n");
	fprintf(out, "                      default values.  FILENAME is just shorthand for\n");
	fprintf(out, "                      \"||||FILENAME\".  The format of SPECIFICATION is:\n");
	fprintf(out, "         [TYPE]|[MIME-TYPE]|[DESCRIPTION]|[WIDTHxHEIGHTxDEPTH[/COLORS]]|FILE\n");
	fprintf(out, "           TYPE is optional; it is a number from one of:\n");
	fprintf(out, "              0: Other\n");
	fprintf(out, "              1: 32x32 pixels 'file icon' (PNG only)\n");
	fprintf(out, "              2: Other file icon\n");
	fprintf(out, "              3: Cover (front)\n");
	fprintf(out, "              4: Cover (back)\n");
	fprintf(out, "              5: Leaflet page\n");
	fprintf(out, "              6: Media (e.g. label side of CD)\n");
	fprintf(out, "              7: Lead artist/lead performer/soloist\n");
	fprintf(out, "              8: Artist/performer\n");
	fprintf(out, "              9: Conductor\n");
	fprintf(out, "             10: Band/Orchestra\n");
	fprintf(out, "             11: Composer\n");
	fprintf(out, "             12: Lyricist/text writer\n");
	fprintf(out, "             13: Recording Location\n");
	fprintf(out, "             14: During recording\n");
	fprintf(out, "             15: During performance\n");
	fprintf(out, "             16: Movie/video screen capture\n");
	fprintf(out, "             17: A bright coloured fish\n");
	fprintf(out, "             18: Illustration\n");
	fprintf(out, "             19: Band/artist logotype\n");
	fprintf(out, "             20: Publisher/Studio logotype\n");
	fprintf(out, "             The default is 3 (front cover).  There may only be one picture each\n");
	fprintf(out, "             of type 1 and 2 in a file.\n");
	fprintf(out, "           MIME-TYPE is optional; if left blank, it will be detected from the\n");
	fprintf(out, "             file.  For best compatibility with players, use pictures with MIME\n");
	fprintf(out, "             type image/jpeg or image/png.  The MIME type can also be --> to\n");
	fprintf(out, "             mean that FILE is actually a URL to an image, though this use is\n");
	fprintf(out, "             discouraged.\n");
	fprintf(out, "           DESCRIPTION is optional; the default is an empty string\n");
	fprintf(out, "           The next part specfies the resolution and color information.  If\n");
	fprintf(out, "             the MIME-TYPE is image/jpeg, image/png, or image/gif, you can\n");
	fprintf(out, "             usually leave this empty and they can be detected from the file.\n");
	fprintf(out, "             Otherwise, you must specify the width in pixels, height in pixels,\n");
	fprintf(out, "             and color depth in bits-per-pixel.  If the image has indexed colors\n");
	fprintf(out, "             you should also specify the number of colors used.\n");
	fprintf(out, "           FILE is the path to the picture file to be imported, or the URL if\n");
	fprintf(out, "             MIME type is -->\n");
	fprintf(out, "--export-picture-to=FILE  Export PICTURE block to a file.  Use '-' for stdout.\n");
	fprintf(out, "                      Only one FLAC file may be specified.  The first PICTURE\n");
	fprintf(out, "                      block will be exported unless --export-picture-to is\n");
	fprintf(out, "                      preceded by a --block-number=# option to specify the exact\n");
	fprintf(out, "                      metadata block to extract.  Note that the block number is\n");
	fprintf(out, "                      the one shown by --list.\n");
	fprintf(out, "--add-replay-gain     Calculates the title and album gains/peaks of the given\n");
	fprintf(out, "                      FLAC files as if all the files were part of one album,\n");
	fprintf(out, "                      then stores them in the VORBIS_COMMENT block.  The tags\n");
	fprintf(out, "                      are the same as those used by vorbisgain.  Existing\n");
	fprintf(out, "                      ReplayGain tags will be replaced.  If only one FLAC file\n");
	fprintf(out, "                      is given, the album and title gains will be the same.\n");
	fprintf(out, "                      Since this operation requires two passes, it is always\n");
	fprintf(out, "                      executed last, after all other operations have been\n");
	fprintf(out, "                      completed and written to disk.  All FLAC files specified\n");
	fprintf(out, "                      must have the same resolution, sample rate, and number\n");
	fprintf(out, "                      of channels.  The sample rate must be one of 8, 11.025,\n");
	fprintf(out, "                      12, 16, 22.05, 24, 32, 44.1, or 48 kHz.\n");
	fprintf(out, "--remove-replay-gain  Removes the ReplayGain tags.\n");
	fprintf(out, "--add-seekpoint={#|X|#x|#s}  Add seek points to a SEEKTABLE block\n");
	fprintf(out, "       #  : a specific sample number for a seek point\n");
	fprintf(out, "       X  : a placeholder point (always goes at the end of the SEEKTABLE)\n");
	fprintf(out, "       #x : # evenly spaced seekpoints, the first being at sample 0\n");
	fprintf(out, "       #s : a seekpoint every # seconds; # does not have to be a whole number\n");
	fprintf(out, "                      If no SEEKTABLE block exists, one will be created.  If\n");
	fprintf(out, "                      one already exists, points will be added to the existing\n");
	fprintf(out, "                      table, and any duplicates will be turned into placeholder\n");
	fprintf(out, "                      points.  You may use many --add-seekpoint options; the\n");
	fprintf(out, "                      resulting SEEKTABLE will be the unique-ified union of\n");
	fprintf(out, "                      all such values.  Example: --add-seekpoint=100x\n");
	fprintf(out, "                      --add-seekpoint=3.5s will add 100 evenly spaced\n");
	fprintf(out, "                      seekpoints and a seekpoint every 3.5 seconds.\n");
	fprintf(out, "--add-padding=length  Add a padding block of the given length (in bytes).\n");
	fprintf(out, "                      The overall length of the new block will be 4 + length;\n");
	fprintf(out, "                      the extra 4 bytes is for the metadata block header.\n");
	fprintf(out, "\n");
	fprintf(out, "Major operations:\n");
	fprintf(out, "--version\n");
	fprintf(out, "    Show the metaflac version number.\n");
	fprintf(out, "--list\n");
	fprintf(out, "    List the contents of one or more metadata blocks to stdout.  By default,\n");
	fprintf(out, "    all metadata blocks are listed in text format.  Use the following options\n");
	fprintf(out, "    to change this behavior:\n");
	fprintf(out, "\n");
	fprintf(out, "    --block-number=#[,#[...]]\n");
	fprintf(out, "    An optional comma-separated list of block numbers to display.  The first\n");
	fprintf(out, "    block, the STREAMINFO block, is block 0.\n");
	fprintf(out, "\n");
	fprintf(out, "    --block-type=type[,type[...]]\n");
	fprintf(out, "    --except-block-type=type[,type[...]]\n");
	fprintf(out, "    An optional comma-separated list of block types to be included or ignored\n");
	fprintf(out, "    with this option.  Use only one of --block-type or --except-block-type.\n");
	fprintf(out, "    The valid block types are: STREAMINFO, PADDING, APPLICATION, SEEKTABLE,\n");
	fprintf(out, "    VORBIS_COMMENT.  You may narrow down the types of APPLICATION blocks\n");
	fprintf(out, "    displayed as follows:\n");
	fprintf(out, "        APPLICATION:abcd        The APPLICATION block(s) whose textual repre-\n");
	fprintf(out, "                                sentation of the 4-byte ID is \"abcd\"\n");
	fprintf(out, "        APPLICATION:0xXXXXXXXX  The APPLICATION block(s) whose hexadecimal big-\n");
	fprintf(out, "                                endian representation of the 4-byte ID is\n");
	fprintf(out, "                                \"0xXXXXXXXX\".  For the example \"abcd\" above the\n");
	fprintf(out, "                                hexadecimal equivalalent is 0x61626364\n");
	fprintf(out, "\n");
	fprintf(out, "    NOTE: if both --block-number and --[except-]block-type are specified,\n");
	fprintf(out, "          the result is the logical AND of both arguments.\n");
	fprintf(out, "\n");
#if 0
	/*@@@ not implemented yet */
	fprintf(out, "    --data-format=binary|text\n");
	fprintf(out, "    By default a human-readable text representation of the data is displayed.\n");
	fprintf(out, "    You may specify --data-format=binary to dump the raw binary form of each\n");
	fprintf(out, "    metadata block.  The output can be read in using a subsequent call to\n");
	fprintf(out, "    "metaflac --append --from-file=..."\n");
	fprintf(out, "\n");
#endif
	fprintf(out, "    --application-data-format=hexdump|text\n");
	fprintf(out, "    If the application block you are displaying contains binary data but your\n");
	fprintf(out, "    --data-format=text, you can display a hex dump of the application data\n");
	fprintf(out, "    contents instead using --application-data-format=hexdump\n");
	fprintf(out, "\n");
#if 0
	/*@@@ not implemented yet */
	fprintf(out, "--append\n");
	fprintf(out, "    Insert a metadata block from a file.  The input file must be in the same\n");
	fprintf(out, "    format as generated with --list.\n");
	fprintf(out, "\n");
	fprintf(out, "    --block-number=#\n");
	fprintf(out, "    Specify the insertion point (defaults to last block).  The new block will\n");
	fprintf(out, "    be added after the given block number.  This prevents the illegal insertion\n");
	fprintf(out, "    of a block before the first STREAMINFO block.  You may not --append another\n");
	fprintf(out, "    STREAMINFO block.\n");
	fprintf(out, "\n");
	fprintf(out, "    --from-file=filename\n");
	fprintf(out, "    Mandatory 'option' to specify the input file containing the block contents.\n");
	fprintf(out, "\n");
	fprintf(out, "    --data-format=binary|text\n");
	fprintf(out, "    By default the block contents are assumed to be in binary format.  You can\n");
	fprintf(out, "    override this by specifying --data-format=text\n");
	fprintf(out, "\n");
#endif
	fprintf(out, "--remove\n");
	fprintf(out, "    Remove one or more metadata blocks from the metadata.  Unless\n");
	fprintf(out, "    --dont-use-padding is specified, the blocks will be replaced with padding.\n");
	fprintf(out, "    You may not remove the STREAMINFO block.\n");
	fprintf(out, "\n");
	fprintf(out, "    --block-number=#[,#[...]]\n");
	fprintf(out, "    --block-type=type[,type[...]]\n");
	fprintf(out, "    --except-block-type=type[,type[...]]\n");
	fprintf(out, "    See --list above for usage.\n");
	fprintf(out, "\n");
	fprintf(out, "    NOTE: if both --block-number and --[except-]block-type are specified,\n");
	fprintf(out, "          the result is the logical AND of both arguments.\n");
	fprintf(out, "\n");
	fprintf(out, "--remove-all\n");
	fprintf(out, "    Remove all metadata blocks (except the STREAMINFO block) from the\n");
	fprintf(out, "    metadata.  Unless --dont-use-padding is specified, the blocks will be\n");
	fprintf(out, "    replaced with padding.\n");
	fprintf(out, "\n");
	fprintf(out, "--merge-padding\n");
	fprintf(out, "    Merge adjacent PADDING blocks into single blocks.\n");
	fprintf(out, "\n");
	fprintf(out, "--sort-padding\n");
	fprintf(out, "    Move all PADDING blocks to the end of the metadata and merge them into a\n");
	fprintf(out, "    single block.\n");

	return message? 1 : 0;
}
