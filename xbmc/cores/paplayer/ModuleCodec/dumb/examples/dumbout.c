/*  _______         ____    __         ___    ___
 * \    _  \       \    /  \  /       \   \  /   /       '   '  '
 *  |  | \  \       |  |    ||         |   \/   |         .      .
 *  |  |  |  |      |  |    ||         ||\  /|  |
 *  |  |  |  |      |  |    ||         || \/ |  |         '  '  '
 *  |  |  |  |      |  |    ||         ||    |  |         .      .
 *  |  |_/  /        \  \__//          ||    |  |
 * /_______/ynamic    \____/niversal  /__\  /____\usic   /|  .  . ibliotheque
 *                                                      /  \
 *                                                     / .  \
 * dumbout.c - Utility to stream music to a file.     / / \  \
 *                                                   | <  /   \_
 * By entheh.                                        |  \/ /\   /
 *                                                    \_  /  > /
 * This example demonstrates how to use DUMB without    | \ / /
 * using Allegro.                                       |  ' /
 *                                                       \__/
 */

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <dumb.h>


union {
	short s16[8192];
	char s8[16384];
} buffer;


int main(int argc, const char *const *argv) /* I'm const-crazy! */
{
	DUH *duh;
	DUH_SIGRENDERER *sr;

	const char *fn = NULL;
	const char *fn_out = NULL;
	FILE *outf;

	int depth = 16;
	int bigendian = 0;
	int unsign = 0;
	int freq = 44100;
	int n_channels = 2;
	float volume = 1.0f;
	float delay = 0.0f;
	float delta;
	int bufsize;
	clock_t start, end;

	int i = 1;

	LONG_LONG length;
	LONG_LONG done;
	int dots;

	while (i < argc) {
		const char *arg = argv[i++];
		if (*arg != '-') {
			if (fn) {
				fprintf(stderr,
					"Cannot specify multiple filenames!\n"
					"Second filename found: \"%s\"\n", arg);
				return EXIT_FAILURE;
			}
			fn = arg;
			continue;
		}
		arg++;
		while (*arg) {
			char *endptr;
			switch (*arg++) {
				case 'o':
				case 'O':
					if (i >= argc) {
						fprintf(stderr, "Out of arguments; output filename expected!\n");
						return EXIT_FAILURE;
					}
					fn_out = argv[i++];
					break;
				case 'd':
				case 'D':
					if (i >= argc) {
						fprintf(stderr, "Out of arguments; delay expected!\n");
						return EXIT_FAILURE;
					}
					delay = (float)strtod(argv[i++], &endptr);
					if (*endptr != 0 || delay < 0.0f || delay > 64.0f) {
						fprintf(stderr, "Invalid delay!\n");
						return EXIT_FAILURE;
					}
					break;
				case 'v':
				case 'V':
					if (i >= argc) {
						fprintf(stderr, "Out of arguments; volume expected!\n");
						return EXIT_FAILURE;
					}
					volume = (float)strtod(argv[i++], &endptr);
					if (*endptr != 0 || volume < -8.0f || volume > 8.0f) {
						fprintf(stderr, "Invalid volume!\n");
						return EXIT_FAILURE;
					}
					break;
				case 's':
				case 'S':
					if (i >= argc) {
						fprintf(stderr, "Out of arguments; sampling rate expected!\n");
						return EXIT_FAILURE;
					}
					freq = strtol(argv[i++], &endptr, 10);
					if (*endptr != 0 || freq < 1 || freq > 960000) {
						fprintf(stderr, "Invalid sampling rate!\n");
						return EXIT_FAILURE;
					}
					break;
				case '8':
					depth = 8;
					break;
				case 'b':
				case 'B':
					bigendian = 1;
					break;
				case 'm':
				case 'M':
					n_channels = 1;
					break;
				case 'u':
				case 'U':
					unsign = 1;
					break;
				case 'r':
				case 'R':
					if (i >= argc) {
						fprintf(stderr, "Out of arguments; resampling quality expected!\n");
						return EXIT_FAILURE;
					}
					dumb_resampling_quality = strtol(argv[i++], &endptr, 10);
					if (*endptr != 0 || dumb_resampling_quality < 0 || dumb_resampling_quality > 2) {
						fprintf(stderr, "Invalid resampling quality!\n");
						return EXIT_FAILURE;
					}
					break;
				default:
					fprintf(stderr, "Invalid switch - '%c'!\n", isprint(arg[-1]) ? arg[-1] : '?');
					return EXIT_FAILURE;
			}
		}
	}

	if (!fn) {
		fprintf(stderr,
			"Usage: dumbout [options] module [more-options]\n"
			"\n"
			"The module can be any IT, XM, S3M or MOD file. It will be rendered to a .pcm\n"
			"file of the same name, unless you specify otherwise with the -o option.\n"
			"\n"
			"The valid options are:\n"
			"-o <file>   specify the output filename (defaults to the input filename with\n"
			"              the extension replaced with .pcm); use - to write to standard\n"
			"              output or . to write nowhere (useful for measuring DUMB's\n"
			"              performance, and DOS and Windows don't have /dev/null!)\n"
			"-d <delay>  set the initial delay, in seconds (default 0.0)\n"
			"-v <volume> adjust the volume (default 1.0)\n"
			"-s <freq>   set the sampling rate in Hz (default 44100)\n"
			"-8          generate 8-bit instead of 16-bit\n"
			"-b          generate big-endian data instead of little-endian (meaningless when\n"
			"              using -8)\n"
			"-m          generate mono output instead of stereo left/right pairs\n"
			"-u          generated unsigned output instead of signed\n"
			"-r <value>  specify the resampling quality to use\n");
		return EXIT_FAILURE;
	}

	/* Initialisation, as in dumbplay.c, except this time we have to
	 * register stdio files since we're not using Allegro.
	 */
	atexit(&dumb_exit);
	dumb_register_stdfiles();

	/* Mix as many voices as possible. DUMB only maintains state for 256
	 * of them.
	 */
	dumb_it_max_to_mix = 256;

	/* We may as well try and load a .duh file too, even though that file
	 * format will probably never materialise. :)
	 */
	duh = load_duh(fn);
	if (!duh) {
		duh = dumb_load_it(fn);
		if (!duh) {
			duh = dumb_load_xm(fn);
			if (!duh) {
				duh = dumb_load_s3m(fn);
				if (!duh) {
					duh = dumb_load_mod(fn);
					if (!duh) {
						fprintf(stderr, "Unable to open %s!\n", fn);
						return EXIT_FAILURE;
					}
				}
			}
		}
	}

	/* This is equivalent to al_start_duh(), except the object returned
	 * contains playback state alone and no Allegro audio stream. We can
	 * get samples from it on demand.
	 */
	sr = duh_start_sigrenderer(duh, 0, n_channels, 0);
	if (!sr) {
		unload_duh(duh);
		fprintf(stderr, "Unable to play file!\n");
		return EXIT_FAILURE;
	}

	if (fn_out) {
		if (fn_out[0] == '-' && fn_out[1] == 0)
			outf = stdout;
		else if (fn_out[0] == '.' && fn_out[1] == 0)
			outf = NULL;
		else {
			outf = fopen(fn_out, "wb");
			if (!outf) {
				fprintf(stderr, "Unable to open %s for writing!\n", fn_out);
				duh_end_sigrenderer(sr);
				unload_duh(duh);
				return EXIT_FAILURE;
			}
		}
	} else {
		char *extptr = NULL, *p;
		char *fn_out = malloc(strlen(fn)+5);
		if (!fn_out) {
			fprintf(stderr, "Out of memory!\n");
			duh_end_sigrenderer(sr);
			unload_duh(duh);
			return EXIT_FAILURE;
		}
		strcpy(fn_out, fn);
		for (p = fn_out; *p; p++)
			if (*p == '.') extptr = p;
		if (!extptr) extptr = p;
		strcpy(extptr, ".pcm");
		outf = fopen(fn_out, "wb");
		if (!outf) {
			fprintf(stderr, "Unable to open %s for writing!\n", fn_out);
			free(fn_out);
			duh_end_sigrenderer(sr);
			unload_duh(duh);
			return EXIT_FAILURE;
		}
		free(fn_out);
	}

	/* Install dumb_it_callback_terminate() as the loop and XM speed zero
	 * callbacks. That means DUMB will stop generating samples
	 * immediately upon either of these events occurring.
	 * The callback function itself is provided by DUMB.
	 */
	{
		DUMB_IT_SIGRENDERER *itsr = duh_get_it_sigrenderer(sr);
		dumb_it_set_loop_callback(itsr, &dumb_it_callback_terminate, NULL);
		dumb_it_set_xm_speed_zero_callback(itsr, &dumb_it_callback_terminate, NULL);
	}

	/* This length value is not accurate. It is only used for the
	 * progress bar.
	 */
	length = (LONG_LONG)duh_get_length(duh) * freq >> 16;
	done = 0;
	dots = 0;
	delta = 65536.0f / freq;
	bufsize = depth == 16 ? 8192 : 16384;
	bufsize /= n_channels;

	/* Write the initial delay to the file if one was requested. */
	{
		long l = (long)floor(delay * freq + 0.5f);
		l *= n_channels * (depth >> 3);
		if (l) {
			if (unsign) {
				if (depth == 16) {
					if (bigendian) {
						for (i = 0; i < 8192; i++) {
							buffer.s8[i*2] = (char)0x80;
							buffer.s8[i*2+1] = (char)0x00;
						}
					} else {
						for (i = 0; i < 8192; i++) {
							buffer.s8[i*2] = (char)0x00;
							buffer.s8[i*2+1] = (char)0x80;
						}
					}
				} else
					memset(buffer.s8, 0x80, 16384);
			} else
				memset(buffer.s8, 0, 16384);
			while (l >= 16384) {
				if (outf) fwrite(buffer.s8, 1, 16384, outf);
				l -= 16384;
			}
			if (l && outf) fwrite(buffer.s8, 1, l, outf);
		}
	}

	/* On Linux, clock() is a measure of how much processing time was
	 * used by the program.
	 */
	start = clock();

	fprintf(stderr, "................................................................\n");
	for (;;) {
		/* This is the function that generates samples. It is all
		 * explained in docs/dumb.txt. The return value is the number
		 * of samples generated. If it's less than the buffer size,
		 * we known that it's finished.
		 */
		int l = duh_render(sr, depth, unsign, volume, delta, bufsize, &buffer);
		if (depth == 16) {
			/* If you are only targeting platforms of a specific
			 * endianness or you can find out what endianness the
			 * target platform is, you should be able to
			 * eliminate one case here.
			 */
			if (bigendian) {
				for (i = 0; i < l * n_channels; i++) {
					short val = buffer.s16[i];
					buffer.s8[i*2] = (char)(val >> 8);
					buffer.s8[i*2+1] = (char)val;
				}
			} else {
				for (i = 0; i < l * n_channels; i++) {
					short val = buffer.s16[i];
					buffer.s8[i*2] = (char)val;
					buffer.s8[i*2+1] = (char)(val >> 8);
				}
			}
		}
		if (outf) fwrite(buffer.s8, 1, l * n_channels * (depth >> 3), outf);
		if (l < bufsize) break;
		done += l;
		l = (int)(done * 64 / length);
		while (dots < 64 && l > dots) {
			fprintf(stderr, "|");
			dots++;
		}
	}

	while (64 > dots) {
		fprintf(stderr, "|");
		dots++;
	}
	fprintf(stderr, "\n");

	end = clock();

	/* Deallocate stuff and close the output file. */
	duh_end_sigrenderer(sr);
	unload_duh(duh);
	if (outf && outf != stdout) fclose(outf);

	fprintf(stderr, "Elapsed time: %f seconds\n", (end - start) / (float)CLOCKS_PER_SEC);

	return EXIT_SUCCESS;
}
