/*
 * asap2wav.c - converter of ASAP-supported formats to WAV files
 *
 * Copyright (C) 2005-2009  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "asap.h"

static const char *output_file = NULL;
static abool output_header = TRUE;
static int song = -1;
static ASAP_SampleFormat format = ASAP_FORMAT_S16_LE;
static int duration = -1;
static int mute_mask = 0;

static void print_help(void)
{
	printf(
		"Usage: asap2wav [OPTIONS] INPUTFILE...\n"
		"Each INPUTFILE must be in a supported format:\n"
		"SAP, CMC, CM3, CMR, CMS, DMC, DLT, MPT, MPD, RMT, TMC, TM8 or TM2.\n"
		"Options:\n"
		"-o FILE     --output=FILE      Set output file name\n"
		"-o -        --output=-         Write to standard output\n"
		"-s SONG     --song=SONG        Select subsong number (zero-based)\n"
		"-t TIME     --time=TIME        Set output length (MM:SS format)\n"
		"-b          --byte-samples     Output 8-bit samples\n"
		"-w          --word-samples     Output 16-bit samples (default)\n"
		"            --raw              Output raw audio (no WAV header)\n"
		"-m CHANNELS --mute=CHANNELS    Mute POKEY channels (1-8)\n"
		"-h          --help             Display this information\n"
		"-v          --version          Display version information\n"
	);
}

static void fatal_error(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, "asap2wav: ");
	vfprintf(stderr, format, args);
	fputc('\n', stderr);
	va_end(args);
	exit(1);
}

static void set_song(const char *s)
{
	song = 0;
	do {
		if (*s < '0' || *s > '9')
			fatal_error("subsong number must be an integer");
		song = 10 * song + *s++ - '0';
		if (song >= ASAP_SONGS_MAX)
			fatal_error("maximum subsong number is %d", ASAP_SONGS_MAX - 1);
	} while (*s != '\0');
}

static void set_time(const char *s)
{
	duration = ASAP_ParseDuration(s);
	if (duration <= 0)
		fatal_error("invalid time format");
}

static void set_mute_mask(const char *s)
{
	int mask = 0;
	while (*s != '\0') {
		if (*s >= '1' && *s <= '8')
			mask |= 1 << (*s - '1');
		s++;
	}
	mute_mask = mask;
}

static void process_file(const char *input_file)
{
	FILE *fp;
	static byte module[ASAP_MODULE_MAX];
	int module_len;
	static ASAP_State asap;
	int n_bytes;
	static byte buffer[8192];
	if (strlen(input_file) >= FILENAME_MAX)
		fatal_error("filename too long");
	fp = fopen(input_file, "rb");
	if (fp == NULL)
		fatal_error("cannot open %s", input_file);
	module_len = fread(module, 1, sizeof(module), fp);
	fclose(fp);
	if (!ASAP_Load(&asap, input_file, module, module_len))
		fatal_error("%s: format not supported", input_file);
	if (song < 0)
		song = asap.module_info.default_song;
	if (song >= asap.module_info.songs) {
		fatal_error("you have requested subsong %d ...\n"
			"... but %s contains only %d subsongs",
			song, input_file, asap.module_info.songs);
	}
	if (duration < 0) {
		duration = asap.module_info.durations[song];
		if (duration < 0)
			duration = 180 * 1000;
	}
	ASAP_PlaySong(&asap, song, duration);
	ASAP_MutePokeyChannels(&asap, mute_mask);
	if (output_file == NULL) {
		static char output_default[FILENAME_MAX];
		strcpy(output_default, input_file);
		ASAP_ChangeExt(output_default, output_header ? "wav" : "raw");
		output_file = output_default;
	}
	if (output_file[0] == '-' && output_file[1] == '\0')
		fp = stdout;
	else {
		fp = fopen(output_file, "wb");
		if (fp == NULL)
			fatal_error("cannot write %s", output_file);
	}
	if (output_header) {
		ASAP_GetWavHeader(&asap, buffer, format);
		fwrite(buffer, 1, ASAP_WAV_HEADER_BYTES, fp);
	}
	do {
		n_bytes = ASAP_Generate(&asap, buffer, sizeof(buffer), format);
		if (fwrite(buffer, 1, n_bytes, fp) != n_bytes) {
			fclose(fp);
			fatal_error("error writing to %s", output_file);
		}
	} while (n_bytes == sizeof(buffer));
	if (!(output_file[0] == '-' && output_file[1] == '\0'))
		fclose(fp);
	output_file = NULL;
	song = -1;
	duration = -1;
}

int main(int argc, char *argv[])
{
	const char *options_error = "no input files";
	int i;
	for (i = 1; i < argc; i++) {
		const char *arg = argv[i];
		if (arg[0] != '-') {
			process_file(arg);
			options_error = NULL;
			continue;
		}
		options_error = "options must be specified before the input file";
		if (arg[1] == 'o' && arg[2] == '\0')
			output_file = argv[++i];
		else if (strncmp(arg, "--output=", 9) == 0)
			output_file = arg + 9;
		else if (arg[1] == 's' && arg[2] == '\0')
			set_song(argv[++i]);
		else if (strncmp(arg, "--song=", 7) == 0)
			set_song(arg + 7);
		else if (arg[1] == 't' && arg[2] == '\0')
			set_time(argv[++i]);
		else if (strncmp(arg, "--time=", 7) == 0)
			set_time(arg + 7);
		else if ((arg[1] == 'b' && arg[2] == '\0')
			|| strcmp(arg, "--byte-samples") == 0)
			format = ASAP_FORMAT_U8;
		else if ((arg[1] == 'w' && arg[2] == '\0')
			|| strcmp(arg, "--word-samples") == 0)
			format = ASAP_FORMAT_S16_LE;
		else if (strcmp(arg, "--raw") == 0)
			output_header = FALSE;
		else if (arg[1] == 'm' && arg[2] == '\0')
			set_mute_mask(argv[++i]);
		else if (strncmp(arg, "--mute=", 7) == 0)
			set_mute_mask(arg + 7);
		else if ((arg[1] == 'h' && arg[2] == '\0')
			|| strcmp(arg, "--help") == 0) {
			print_help();
			options_error = NULL;
		}
		else if ((arg[1] == 'v' && arg[2] == '\0')
			|| strcmp(arg, "--version") == 0) {
			printf("ASAP2WAV " ASAP_VERSION "\n");
			options_error = NULL;
		}
		else
			fatal_error("unknown option: %s", arg);
	}
	if (options_error != NULL) {
		fprintf(stderr, "asap2wav: %s\n", options_error);
		print_help();
		return 1;
	}
	return 0;
}
