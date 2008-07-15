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
 * dumb2wav.c - Utility to convert DUH to WAV.        / / \  \
 *                                                   | <  /   \_
 * By Chad Austin, based on dumbout.c by entheh.     |  \/ /\   /
 *                                                    \_  /  > /
 * Not such a good example. Just a useful utility!      | \ / /
 *                                                      |  ' /
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

static int loop_count = 1;


static int write32_le(FILE* outf, unsigned int value) {
	int total = 0;
	total += fputc(value & 0xFF,         outf);
	total += fputc((value >> 8)  & 0xFF, outf);
	total += fputc((value >> 16) & 0xFF, outf);
	total += fputc((value >> 24) & 0xFF, outf);
	return total;
}

static int write16_le(FILE* outf, unsigned int value) {
	int total = 0;
	total += fputc(value & 0xFF,        outf);
	total += fputc((value >> 8) & 0xFF, outf);
	return total;
}


static int loop_callback(void *data) {
	(void)data;
	return --loop_count <= 0;
}


int main(int argc, const char *argv[])
{
	DUH *duh;
	DUH_SIGRENDERER *sr;

	const char *fn = NULL;
	const char *fn_out = NULL;
	FILE *outf;

	int depth = 16;
	int unsign = 0;
	int freq = 44100;
	int n_channels = 2;
	float volume = 1.0f;
	float delay = 0.0f;
	float delta;
	int bufsize;
	clock_t start, end;
	int data_written = 0;  /* total bytes written to data chunk */

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
				case 'l':
				case 'L':
					if (i >= argc) {
						fprintf(stderr, "Out of arguments: loop count expected!\n");
						return EXIT_FAILURE;
					}
					loop_count = strtol(argv[i++], &endptr, 10);
					if (*endptr != 0 || loop_count < 1) {
						fprintf(stderr, "Invalid loop count!\n");
						return EXIT_FAILURE;
					}
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
			"Usage: dumb2wav [options] module [more-options]\n"
			"\n"
			"The module can be any IT, XM, S3M or MOD file. It will be rendered to a .wav\n"
			"file of the same name, unless you specify otherwise with the -o option.\n"
			"\n"
			"The valid options are:\n"
			"-o <file>   specify the output filename (defaults to the input filename with\n"
			"              the extension replaced with .wav); use - to write to standard\n"
			"              output or . to write nowhere (useful for measuring DUMB's\n"
			"              performance, and DOS and Windows don't have /dev/null!)\n"
			"-d <delay>  set the initial delay, in seconds (default 0.0)\n"
			"-v <volume> adjust the volume (default 1.0)\n"
			"-s <freq>   set the sampling rate in Hz (default 44100)\n"
			"-8          generate 8-bit instead of 16-bit\n"
			"-m          generate mono output instead of stereo left/right pairs\n"
			"-u          generated unsigned output instead of signed\n"
			"-r <value>  specify the resampling quality to use\n"
			"-l <value>  specify the number of times to loop (default 1)\n");
		return EXIT_FAILURE;
	}

	atexit(&dumb_exit);
	dumb_register_stdfiles();

	dumb_it_max_to_mix = 256;

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
		strcpy(extptr, ".wav");
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

	{
		DUMB_IT_SIGRENDERER *itsr = duh_get_it_sigrenderer(sr);
		dumb_it_set_loop_callback(itsr, loop_callback, NULL);
		dumb_it_set_xm_speed_zero_callback(itsr, &dumb_it_callback_terminate, NULL);
	}


	length = (LONG_LONG)duh_get_length(duh) * freq >> 16;
	done = 0;
	dots = 0;
	delta = 65536.0f / freq;
	bufsize = depth == 16 ? 8192 : 16384;
	bufsize /= n_channels;

	/* If writing to stdout, we can't seek, so do an initial run-through
	 * to determine the length.
	 */
	if (outf == stdout) {
		int lc = loop_count;
		/* Account for initial delay. */
		data_written = (long)floor(delay * freq + 0.5f) * n_channels * (depth >> 3);
		/* Simulate the rendering loop. */
		for (;;) {
			int l = duh_sigrenderer_generate_samples(sr, 0, delta, bufsize, NULL);
			data_written += l * n_channels * (depth >> 3);
			if (l < bufsize) break;
		}
		/* Reinitialise the player. */
		duh_end_sigrenderer(sr);
		loop_count = lc;
		sr = duh_start_sigrenderer(duh, 0, n_channels, 0);
		if (!sr) {
			unload_duh(duh);
			fprintf(stderr, "Unable to play file!\n");
			return EXIT_FAILURE;
		}
		{
			DUMB_IT_SIGRENDERER *itsr = duh_get_it_sigrenderer(sr);
			dumb_it_set_loop_callback(itsr, loop_callback, NULL);
			dumb_it_set_xm_speed_zero_callback(itsr, &dumb_it_callback_terminate, NULL);
		}
	}

	if (outf) {
		/* file size, not including RIFF header */
		const int fmt_size = 8 + 16;
		const int data_size = 8 + data_written;
		const int file_size = fmt_size + data_size;

		/* write RIFF header: correct file length later if not stdout */
		fwrite("RIFF", 1, 4, outf);
		write32_le(outf, file_size);
		fwrite("WAVE", 1, 4, outf);

		/* write format chunk */
		fwrite("fmt ", 1, 4, outf);
		write32_le(outf, 16);         /* header length */
		write16_le(outf, 1);          /* WAVE_FORMAT_PCM */
		write16_le(outf, n_channels); /* channel count */
		write32_le(outf, freq);       /* frequency */
		write32_le(outf, freq * n_channels * depth / 8); /*bytes/sec*/
		write16_le(outf, n_channels * depth / 8); /* block alignment */
		write16_le(outf, depth);      /* bits per sample */

		/* start data chunk */
		fwrite("data", 1, 4, outf);
		write32_le(outf, data_written); /* correct later if not stdout */
	}

	{
		long l = (long)floor(delay * freq + 0.5f);
		l *= n_channels * (depth >> 3);
		if (l) {
			if (unsign) {
				if (depth == 16) {
					for (i = 0; i < 8192; i++) {
						buffer.s8[i*2] = (char)0x00;
						buffer.s8[i*2+1] = (char)0x80;
					}
				} else
					memset(buffer.s8, 0x80, 16384);
			} else
				memset(buffer.s8, 0, 16384);
			while (l >= 16384) {
				if (outf) fwrite(buffer.s8, 1, 16384, outf);
				l -= 16384;
				data_written += 16384;
			}
			if (l) {
				if (outf) fwrite(buffer.s8, 1, l, outf);
				data_written += l;
			}
		}
	}

	start = clock();

	fprintf(stderr, "................................................................\n");
	for (;;) {
		int write_size;
		int l = duh_render(sr, depth, unsign, volume, delta, bufsize, &buffer);
		if (depth == 16) {
			for (i = 0; i < l * n_channels; i++) {
				short val = buffer.s16[i];
				buffer.s8[i*2] = (char)val;
				buffer.s8[i*2+1] = (char)(val >> 8);
			}
		}
		write_size = l * n_channels * (depth >> 3);
		if (outf) fwrite(buffer.s8, 1, write_size, outf);
		data_written += write_size;
		if (l < bufsize) break;
		done += l;
		l = (int)(done * 64 / length);
		while (dots < 64 && l > dots) {
			fprintf(stderr, "|");
			dots++;
		}
		if (dots >= 64) {
			fprintf(stderr, "\n");
			dots = 0;
			done = 0;
		}
	}

	while (64 > dots) {
		fprintf(stderr, "|");
		dots++;
	}
	fprintf(stderr, "\n");

	end = clock();

	/* fill in blanks we left in WAVE file */
	if (outf && outf != stdout) {
		/* file size, not including RIFF header */
		const int fmt_size = 8 + 16;
		const int data_size = 8 + data_written;
		const int file_size = fmt_size + data_size;

		fseek(outf, 4, SEEK_SET);
		write32_le(outf, file_size);

		fseek(outf, 12 + fmt_size + 4, SEEK_SET);
		write32_le(outf, data_written);
	}


	duh_end_sigrenderer(sr);
	unload_duh(duh);
	if (outf && outf != stdout) fclose(outf);

	fprintf(stderr, "Elapsed time: %f seconds\n", (end - start) / (float)CLOCKS_PER_SEC);

	return EXIT_SUCCESS;
}
