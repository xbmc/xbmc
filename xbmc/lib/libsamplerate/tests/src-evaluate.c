/*
** Copyright (C) 2002-2008 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "config.h"

#if (HAVE_FFTW3 && HAVE_SNDFILE && HAVE_SYS_TIMES_H)

#include <time.h>
#include <sys/times.h>

#include <sndfile.h>
#include <math.h>
#include <sys/utsname.h>

#include "util.h"

#define	MAX_FREQS		4
#define	BUFFER_LEN		80000

#define SAFE_STRNCAT(dest,src,len)								\
		{	int safe_strncat_count ;							\
			safe_strncat_count = (len) - strlen (dest) - 1 ;	\
			strncat ((dest), (src), safe_strncat_count) ;		\
			(dest) [(len) - 1] = 0 ;							\
			} ;

typedef struct
{	int		freq_count ;
	double	freqs [MAX_FREQS] ;

	int		output_samplerate ;
	int		pass_band_peaks ;

	double	peak_value ;
} SNR_TEST ;

typedef struct
{	const char	*progname ;
	const char	*version_cmd ;
	const char	*version_start ;
	const char	*convert_cmd ;
	int			format ;
} RESAMPLE_PROG ;

static char *get_progname (char *) ;
static void usage_exit (const char *, const RESAMPLE_PROG *prog, int count) ;
static void measure_program (const RESAMPLE_PROG *prog, int verbose) ;
static void generate_source_wav (const char *filename, const double *freqs, int freq_count, int format) ;
static const char* get_machine_details (void) ;

static char	version_string [512] ;

int
main (int argc, char *argv [])
{	static RESAMPLE_PROG resample_progs [] =
	{	{	"sndfile-resample",
			"examples/sndfile-resample --version",
			"libsamplerate",
			"examples/sndfile-resample --max-speed -c 0 -to %d source.wav destination.wav",
			SF_FORMAT_WAV | SF_FORMAT_PCM_32
			},
		{	"sox",
			"sox -h 2>&1",
			"sox",
			"sox source.wav -r %d destination.wav resample 0.835",
			SF_FORMAT_WAV | SF_FORMAT_PCM_32
			},
		{	"ResampAudio",
			"ResampAudio --version",
			"ResampAudio",
			"ResampAudio -f cutoff=0.41,atten=100,ratio=128 -s %d source.wav destination.wav",
			SF_FORMAT_WAV | SF_FORMAT_PCM_32
			},

		/*-
		{	/+*
			** The Shibatch converter doesn't work for all combinations of
			** source and destination sample rates. Therefore it can't be
			** included in this test.
			*+/
			"shibatch",
			"ssrc",
			"Shibatch",
			"ssrc --rate %d source.wav destination.wav",
			SF_FORMAT_WAV | SF_FORMAT_PCM_32
			},-*/

		/*-
		{	/+*
			** The resample program is not able to match the bandwidth and SNR
			** specs or sndfile-resample and hence will not be tested.
			*+/
			"resample",
			"resample -version",
			"resample",
			"resample -to %d source.wav destination.wav",
			SF_FORMAT_WAV | SF_FORMAT_FLOAT
			},-*/

		/*-
		{	"mplayer",
			"mplayer -v 2>&1",
			"MPlayer ",
			"mplayer -ao pcm -srate %d source.wav >/dev/null 2>&1 && mv audiodump.wav destination.wav",
			SF_FORMAT_WAV | SF_FORMAT_PCM_32
			},-*/

		} ; /* resample_progs */

	char	*progname ;
	int 	prog = 0, verbose = 0 ;

	progname = get_progname (argv [0]) ;

	printf ("\n  %s : evaluate a sample rate converter.\n", progname) ;

	if (argc == 3 && strcmp ("--verbose", argv [1]) == 0)
	{	verbose = 1 ;
		prog = atoi (argv [2]) ;
		}
	else if (argc == 2)
	{	verbose = 0 ;
		prog = atoi (argv [1]) ;
		}
	else
		usage_exit (progname, resample_progs, ARRAY_LEN (resample_progs)) ;

	if (prog < 0 || prog >= ARRAY_LEN (resample_progs))
		usage_exit (progname, resample_progs, ARRAY_LEN (resample_progs)) ;

	measure_program (& (resample_progs [prog]), verbose) ;

	puts ("") ;

	return 0 ;
} /* main */

/*==============================================================================
*/

static char *
get_progname (char *progname)
{	char *cptr ;

	if ((cptr = strrchr (progname, '/')) != NULL)
		progname = cptr + 1 ;

	if ((cptr = strrchr (progname, '\\')) != NULL)
		progname = cptr + 1 ;

	return progname ;
} /* get_progname */

static void
usage_exit (const char *progname, const RESAMPLE_PROG *prog, int count)
{	int k ;

	printf ("\n  Usage : %s <number>\n\n", progname) ;

	puts ("  where <number> specifies the program to test:\n") ;

	for (k = 0 ; k < count ; k++)
		printf ("    %d : %s\n", k, prog [k].progname) ;

	puts ("\n"
		" Obviously to test a given program you have to have it available on\n"
		" your system. See http://www.mega-nerd.com/SRC/quality.html for\n"
		" the download location of these programs.\n") ;

	exit (1) ;
} /* usage_exit */

static const char*
get_machine_details (void)
{	static char namestr [256] ;

	struct utsname name ;

	if (uname (&name) != 0)
	{	snprintf (namestr, sizeof (namestr), "Unknown") ;
		return namestr ;
		} ;

	snprintf (namestr, sizeof (namestr), "%s (%s %s %s)", name.nodename,
			name.machine, name.sysname, name.release) ;

	return namestr ;
} /* get_machine_details */


/*==============================================================================
*/

static void
get_version_string (const RESAMPLE_PROG *prog)
{	FILE *file ;
	char *cptr ;

	/* Default. */
	snprintf (version_string, sizeof (version_string), "no version") ;

	if (prog->version_cmd == NULL)
		return ;

	if ((file = popen (prog->version_cmd, "r")) == NULL)
		return ;

	while ((cptr = fgets (version_string, sizeof (version_string), file)) != NULL)
	{
		if (strstr (cptr, prog->version_start) != NULL)
			break ;

		version_string [0] = 0 ;
		} ;

	pclose (file) ;

	/* Remove trailing newline. */
	if ((cptr = strchr (version_string, '\n')) != NULL)
		cptr [0] = 0 ;

	/* Remove leading whitespace from version string. */
	cptr = version_string ;
	while (cptr [0] != 0 && isspace (cptr [0]))
		cptr ++ ;

	if (cptr != version_string)
		strncpy (version_string, cptr, sizeof (version_string)) ;

	return ;
} /* get_version_string */

static void
generate_source_wav (const char *filename, const double *freqs, int freq_count, int format)
{	static float buffer [BUFFER_LEN] ;

	SNDFILE *sndfile ;
	SF_INFO sfinfo ;

	sfinfo.channels = 1 ;
	sfinfo.samplerate = 44100 ;
	sfinfo.format = format ;

	if ((sndfile = sf_open (filename, SFM_WRITE, &sfinfo)) == NULL)
	{	printf ("Line %d : cound not open '%s' : %s\n", __LINE__, filename, sf_strerror (NULL)) ;
		exit (1) ;
		} ;

	sf_command (sndfile, SFC_SET_ADD_PEAK_CHUNK, NULL, SF_FALSE) ;

	gen_windowed_sines (freq_count, freqs, 0.9, buffer, ARRAY_LEN (buffer)) ;

	if (sf_write_float (sndfile, buffer, ARRAY_LEN (buffer)) != ARRAY_LEN (buffer))
	{	printf ("Line %d : sf_write_float short write.\n", __LINE__) ;
		exit (1) ;
		} ;

	sf_close (sndfile) ;
} /* generate_source_wav */

static double
measure_destination_wav (char *filename, int *output_samples, int expected_peaks)
{	static float buffer [250000] ;

	SNDFILE *sndfile ;
	SF_INFO sfinfo ;
	double snr ;

	if ((sndfile = sf_open (filename, SFM_READ, &sfinfo)) == NULL)
	{	printf ("Line %d : Cound not open '%s' : %s\n", __LINE__, filename, sf_strerror (NULL)) ;
		exit (1) ;
		} ;

	if (sfinfo.channels != 1)
	{	printf ("Line %d : Bad channel count (%d). Should be 1.\n", __LINE__, sfinfo.channels) ;
		exit (1) ;
		} ;

	if (sfinfo.frames > ARRAY_LEN (buffer))
	{	printf ("Line %d : Too many frames (%ld) of data in file.\n", __LINE__, (long) sfinfo.frames) ;
		exit (1) ;
		} ;

	*output_samples = (int) sfinfo.frames ;

	if (sf_read_float (sndfile, buffer, sfinfo.frames) != sfinfo.frames)
	{	printf ("Line %d : Bad read.\n", __LINE__) ;
		exit (1) ;
		} ;

	sf_close (sndfile) ;

	snr = calculate_snr (buffer, sfinfo.frames, expected_peaks) ;

	return snr ;
} /* measure_desination_wav */

static double
measure_snr (const RESAMPLE_PROG *prog, int *output_samples, int verbose)
{	static SNR_TEST snr_test [] =
	{
		{	1,	{ 0.211111111111 },		48000,		1,	1.0 },
		{	1,	{ 0.011111111111 },		132301,		1,	1.0 },
		{	1,	{ 0.111111111111 },		92301,		1,	1.0 },
		{	1,	{ 0.011111111111 },		26461,		1,	1.0 },
		{	1,	{ 0.011111111111 },		13231,		1,	1.0 },
		{	1,	{ 0.011111111111 },		44101,		1,	1.0 },
		{	2,	{ 0.311111, 0.49 },		78199,		2,	1.0 },
		{	2,	{ 0.011111, 0.49 },		12345,		1,	0.5 },
		{	2,	{ 0.0123456, 0.4 },		20143,		1,	0.5 },
		{	2,	{ 0.0111111, 0.4 },		26461,		1,	0.5 },
		{	1,	{ 0.381111111111 },		58661,		1,	1.0 }
		} ; /* snr_test */
	static char command [256] ;

	double snr, worst_snr = 500.0 ;
	int k , retval, sample_count ;

	*output_samples = 0 ;

	for (k = 0 ; k < ARRAY_LEN (snr_test) ; k++)
	{	remove ("source.wav") ;
		remove ("destination.wav") ;

		if (verbose)
			printf ("       SNR test #%d : ", k) ;
		fflush (stdout) ;
		generate_source_wav ("source.wav", snr_test [k].freqs, snr_test [k].freq_count, prog->format) ;

		snprintf (command, sizeof (command), prog->convert_cmd, snr_test [k].output_samplerate) ;
		SAFE_STRNCAT (command, " >/dev/null 2>&1", sizeof (command)) ;
		if ((retval = system (command)) != 0)
			printf ("system returned %d\n", retval) ;

		snr = measure_destination_wav ("destination.wav", &sample_count, snr_test->pass_band_peaks) ;

		*output_samples += sample_count ;

		if (fabs (snr) < fabs (worst_snr))
			worst_snr = fabs (snr) ;

		if (verbose)
			printf ("%6.2f dB\n", snr) ;
		} ;

	return worst_snr ;
} /* measure_snr */

/*------------------------------------------------------------------------------
*/

static double
measure_destination_peak (const char *filename)
{	static float data [2 * BUFFER_LEN] ;
	SNDFILE		*sndfile ;
	SF_INFO		sfinfo ;
	double		peak = 0.0 ;
	int			k = 0 ;

	if ((sndfile = sf_open (filename, SFM_READ, &sfinfo)) == NULL)
	{	printf ("Line %d : failed to open file %s\n", __LINE__, filename) ;
		exit (1) ;
		} ;

	if (sfinfo.channels != 1)
	{	printf ("Line %d : bad channel count.\n", __LINE__) ;
		exit (1) ;
		} ;

	if (sfinfo.frames > ARRAY_LEN (data) + 4 || sfinfo.frames < ARRAY_LEN (data) - 100)
	{	printf ("Line %d : bad frame count (got %d, expected %d).\n", __LINE__, (int) sfinfo.frames, ARRAY_LEN (data)) ;
		exit (1) ;
		} ;

	if (sf_read_float (sndfile, data, sfinfo.frames) != sfinfo.frames)
	{	printf ("Line %d : bad read.\n", __LINE__) ;
		exit (1) ;
		} ;

	sf_close (sndfile) ;

	for (k = 0 ; k < (int) sfinfo.frames ; k++)
		if (fabs (data [k]) > peak)
			peak = fabs (data [k]) ;

	return peak ;
} /* measure_destination_peak */

static double
find_attenuation (double freq, const RESAMPLE_PROG *prog, int verbose)
{	static char	command [256] ;
	double	output_peak ;
	int		retval ;
	char	*filename ;

	filename = "destination.wav" ;

	generate_source_wav ("source.wav", &freq, 1, prog->format) ;

	remove (filename) ;

	snprintf (command, sizeof (command), prog->convert_cmd, 88189) ;
	SAFE_STRNCAT (command, " >/dev/null 2>&1", sizeof (command)) ;
	if ((retval = system (command)) != 0)
		printf ("system returned %d\n", retval) ;

	output_peak = measure_destination_peak (filename) ;

	if (verbose)
		printf ("        freq : %f     peak : %f\n", freq, output_peak) ;

	return fabs (20.0 * log10 (output_peak)) ;
} /* find_attenuation */

static double
bandwidth_test (const RESAMPLE_PROG *prog, int verbose)
{	double	f1, f2, a1, a2 ;
	double	freq, atten ;

	f1 = 0.35 ;
	a1 = find_attenuation (f1, prog, verbose) ;

	f2 = 0.49999 ;
	a2 = find_attenuation (f2, prog, verbose) ;


	if (fabs (a1) < 1e-2 && a2 < 3.0)
		return -1.0 ;

	if (a1 > 3.0 || a2 < 3.0)
	{	printf ("\n\nLine %d : cannot bracket 3dB point.\n\n", __LINE__) ;
		exit (1) ;
		} ;

	while (a2 - a1 > 1.0)
	{	freq = f1 + 0.5 * (f2 - f1) ;
		atten = find_attenuation (freq, prog, verbose) ;

		if (atten < 3.0)
		{	f1 = freq ;
			a1 = atten ;
			}
		else
		{	f2 = freq ;
			a2 = atten ;
			} ;
		} ;

	freq = f1 + (3.0 - a1) * (f2 - f1) / (a2 - a1) ;

	return 200.0 * freq ;
} /* bandwidth_test */

static void
measure_program (const RESAMPLE_PROG *prog, int verbose)
{	double	snr, bandwidth, conversion_rate ;
	int		output_samples ;
	struct	tms	time_data ;
	time_t	time_now ;

	printf ("\n  Machine : %s\n", get_machine_details ()) ;
	time_now = time (NULL) ;
	printf ("  Date    : %s", ctime (&time_now)) ;

	get_version_string (prog) ;
	printf ("  Program : %s\n", version_string) ;
	printf ("  Command : %s\n\n", prog->convert_cmd) ;

	snr = measure_snr (prog, &output_samples, verbose) ;

	printf ("  Worst case SNR     : %6.2f dB\n", snr) ;

	times (&time_data) ;

	conversion_rate = (1.0 * output_samples * sysconf (_SC_CLK_TCK)) / time_data.tms_cutime ;

	printf ("  Conversion rate    : %5.0f samples/sec\n", conversion_rate) ;

	bandwidth = bandwidth_test (prog, verbose) ;

	if (bandwidth > 0.0)
		printf ("  Measured bandwidth : %5.2f %%\n", bandwidth) ;
	else
		printf ("  Could not measure bandwidth (no -3dB point found).\n") ;

	return ;
} /* measure_program */

/*##############################################################################
*/

#else

int
main (void)
{	puts ("\n"
		"****************************************************************\n"
		" This program has been compiled without :\n"
		"	1) FFTW (http://www.fftw.org/).\n"
		"	2) libsndfile (http://www.zip.com.au/~erikd/libsndfile/).\n"
		" Without these two libraries there is not much it can do.\n"
		"****************************************************************\n") ;

	return 0 ;
} /* main */

#endif /* (HAVE_FFTW3 && HAVE_SNDFILE) */

