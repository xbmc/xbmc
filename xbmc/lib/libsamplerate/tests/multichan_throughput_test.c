/*
** Copyright (C) 2008 Erik de Castro Lopo <erikd@mega-nerd.com>
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
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <samplerate.h>

#include "config.h"

#include "util.h"
#include "float_cast.h"

#define BUFFER_LEN	(1<<17)

static float input [BUFFER_LEN] ;
static float output [BUFFER_LEN] ;

static long
throughput_test (int converter, int channels, long best_throughput)
{	SRC_DATA src_data ;
	clock_t start_time, clock_time ;
	double duration ;
	long total_frames = 0, throughput ;
	int error ;

	printf ("    %-30s     %2d         ", src_get_name (converter), channels) ;
	fflush (stdout) ;

	src_data.data_in = input ;
	src_data.input_frames = ARRAY_LEN (input) / channels ;

	src_data.data_out = output ;
	src_data.output_frames = ARRAY_LEN (output) / channels ;

	src_data.src_ratio = 0.99 ;

	sleep (2) ;

	start_time = clock () ;

	do
	{
		if ((error = src_simple (&src_data, converter, channels)) != 0)
		{	puts (src_strerror (error)) ;
			exit (1) ;
			} ;

		total_frames += src_data.output_frames_gen ;

		clock_time = clock () - start_time ;
		duration = (1.0 * clock_time) / CLOCKS_PER_SEC ;
	}
	while (duration < 5.0) ;

	if (src_data.input_frames_used != src_data.input_frames)
	{	printf ("\n\nLine %d : input frames used %ld should be %ld\n", __LINE__, src_data.input_frames_used, src_data.input_frames) ;
		exit (1) ;
		} ;

	if (fabs (src_data.src_ratio * src_data.input_frames_used - src_data.output_frames_gen) > 2)
	{	printf ("\n\nLine %d : input / output length mismatch.\n\n", __LINE__) ;
		printf ("    input len  : %d\n", ARRAY_LEN (input) / channels) ;
		printf ("    output len : %ld (should be %g +/- 2)\n\n", src_data.output_frames_gen,
				floor (0.5 + src_data.src_ratio * src_data.input_frames_used)) ;
		exit (1) ;
		} ;

	throughput = lrint (floor (total_frames / duration)) ;

	if (best_throughput == 0)
	{	best_throughput = MAX (throughput, best_throughput) ;
		printf ("%5.2f      %10ld\n", duration, throughput) ;
		}
	else
	{	best_throughput = MAX (throughput, best_throughput) ;
		printf ("%5.2f      %10ld       %10ld\n", duration, throughput, best_throughput) ;
		}

	return best_throughput ;
} /* throughput_test */

static void
single_run (void)
{	const int max_channels = 10 ;
	int k ;

	printf ("\n    CPU : ") ;
	print_cpu_name () ;

	puts (
		"\n"
		"    Converter                        Channels    Duration      Throughput\n"
		"    ---------------------------------------------------------------------"
		) ;

	for (k = 1 ; k <= max_channels / 2 ; k++)
		throughput_test (SRC_SINC_FASTEST, k, 0) ;

	puts ("") ;
	for (k = 1 ; k <= max_channels / 2 ; k++)
		throughput_test (SRC_SINC_MEDIUM_QUALITY, k, 0) ;

	puts ("") ;
	for (k = 1 ; k <= max_channels ; k++)
		throughput_test (SRC_SINC_BEST_QUALITY, k, 0) ;

	puts ("") ;
	return ;
} /* single_run */

static void
multi_run (int run_count)
{	int k, ch ;

	printf ("\n    CPU name : ") ;
	print_cpu_name () ;

	puts (
		"\n"
		"    Converter                        Channels    Duration      Throughput    Best Throughput\n"
		"    ----------------------------------------------------------------------------------------"
		) ;

	for (ch = 1 ; ch <= 5 ; ch++)
	{	long sinc_fastest = 0, sinc_medium = 0, sinc_best = 0 ;

		for (k = 0 ; k < run_count ; k++)
		{	sinc_fastest =		throughput_test (SRC_SINC_FASTEST, ch, sinc_fastest) ;
			sinc_medium =		throughput_test (SRC_SINC_MEDIUM_QUALITY, ch, sinc_medium) ;
			sinc_best =			throughput_test (SRC_SINC_BEST_QUALITY, ch, sinc_best) ;

			puts ("") ;

			/* Let the CPU cool down. We might be running on a laptop. */
			sleep (10) ;
			} ;

		puts (
			"\n"
			"    Converter                        Best Throughput\n"
			"    ------------------------------------------------"
			) ;

		printf ("    %-30s    %10ld\n", src_get_name (SRC_SINC_FASTEST), sinc_fastest) ;
		printf ("    %-30s    %10ld\n", src_get_name (SRC_SINC_MEDIUM_QUALITY), sinc_medium) ;
		printf ("    %-30s    %10ld\n", src_get_name (SRC_SINC_BEST_QUALITY), sinc_best) ;
		} ;

	puts ("") ;
} /* multi_run */

static void
usage_exit (const char * argv0)
{	const char * cptr ;

	if ((cptr = strrchr (argv0, '/')) != NULL)
		argv0 = cptr ;

	printf (
		"Usage :\n"
	 	"    %s                 - Single run of the throughput test.\n"
		"    %s --best-of N     - Do N runs of test a print bext result.\n"
		"\n",
		argv0, argv0) ;

	exit (0) ;
} /* usage_exit */

int
main (int argc, char ** argv)
{	double freq ;

	memset (input, 0, sizeof (input)) ;
	freq = 0.01 ;
	gen_windowed_sines (1, &freq, 1.0, input, BUFFER_LEN) ;

	if (argc == 1)
		single_run () ;
	else if (argc == 3 && strcmp (argv [1], "--best-of") == 0)
	{	int run_count = atoi (argv [2]) ;

		if (run_count < 1 || run_count > 20)
		{	printf ("Please be sensible. Run count should be in range (1, 10].\n") ;
			exit (1) ;
			} ;

		multi_run (run_count) ;
		}
	else
		usage_exit (argv [0]) ;

	puts (
		"            Duration is in seconds.\n"
		"            Throughput is in frames/sec (more is better).\n"
		) ;

	return 0 ;
} /* main */

