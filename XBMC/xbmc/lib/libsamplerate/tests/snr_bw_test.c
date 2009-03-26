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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#if (HAVE_FFTW3)

#include <samplerate.h>

#include "util.h"

#define	BUFFER_LEN		50000
#define	MAX_FREQS		4
#define	MAX_RATIOS		6
#define	MAX_SPEC_LEN	(1<<15)

#ifndef	M_PI
#define	M_PI			3.14159265358979323846264338
#endif

enum
{	BOOLEAN_FALSE	= 0,
	BOOLEAN_TRUE	= 1
} ;

typedef struct
{	int		freq_count ;
	double	freqs [MAX_FREQS] ;

	double	src_ratio ;
	int		pass_band_peaks ;

	double	snr ;
	double	peak_value ;
} SINGLE_TEST ;

typedef struct
{	int			converter ;
	int			tests ;
	int			do_bandwidth_test ;
	SINGLE_TEST	test_data [10] ;
} CONVERTER_TEST ;

static double snr_test (SINGLE_TEST *snr_test_data, int number, int converter, int verbose) ;
static double find_peak (float *output, int output_len) ;
static double bandwidth_test (int converter, int verbose) ;

int
main (int argc, char *argv [])
{	CONVERTER_TEST snr_test_data [] =
	{
		{	SRC_ZERO_ORDER_HOLD,
			8,
			BOOLEAN_FALSE,
			{	{	1,	{ 0.01111111111 },		3.0,		1,	 28.0,	1.0 },
				{	1,	{ 0.01111111111 },		0.6,		1,	 36.0,	1.0 },
				{	1,	{ 0.01111111111 },		0.3,		1,	 36.0,	1.0 },
				{	1,	{ 0.01111111111 },		1.0,		1,	150.0,	1.0 },
				{	1,	{ 0.01111111111 },		1.001,		1,	 38.0,	1.0 },
				{	2,	{ 0.011111, 0.324 },	1.9999,		2,	 14.0,	1.0 },
				{	2,	{ 0.012345, 0.457 },	0.456789,	1,	 12.0,	1.0 },
				{	1,	{ 0.3511111111 },		1.33,		1,	 10.0,	1.0 }
				}
			},

		{	SRC_LINEAR,
			8,
			BOOLEAN_FALSE,
			{	{	1,	{ 0.01111111111 },		3.0,		1,	 73.0,	1.0 },
				{	1,	{ 0.01111111111 },		0.6,		1,	 73.0,	1.0 },
				{	1,	{ 0.01111111111 },		0.3,		1,	 73.0,	1.0 },
				{	1,	{ 0.01111111111 },		1.0,		1,	150.0,	1.0 },
				{	1,	{ 0.01111111111 },		1.001,		1,	 77.0,	1.0 },
				{	2,	{ 0.011111, 0.324 },	1.9999,		2,	 16.0,	0.94 },
				{	2,	{ 0.012345, 0.457 },	0.456789,	1,	 26.0,	0.96 },
				{	1,	{ 0.3511111111 },		1.33,		1,	 22.0,	0.99 }
				}
			},

		{	SRC_SINC_FASTEST,
			9,
			BOOLEAN_TRUE,
			{	{	1,	{ 0.01111111111 },		3.0,		1,	100.0,	1.0 },
				{	1,	{ 0.01111111111 },		0.6,		1,	 99.0,	1.0 },
				{	1,	{ 0.01111111111 },		0.3,		1,	100.0,	1.0 },
				{	1,	{ 0.01111111111 },		1.0,		1,	150.0,	1.0 },
				{	1,	{ 0.01111111111 },		1.001,		1,	100.0,	1.0 },
				{	2,	{ 0.011111, 0.324 },	1.9999,		2,	 97.0,	1.0 },
				{	2,	{ 0.012345, 0.457 },	0.456789,	1,	100.0,	0.5 },
				{	2,	{ 0.011111, 0.45 },		0.6,		1,	 97.0,	0.5 },
				{	1,	{ 0.3511111111 },		1.33,		1,	 97.0,	1.0 }
				}
			},

		{	SRC_SINC_MEDIUM_QUALITY,
			9,
			BOOLEAN_TRUE,
			{	{	1,	{ 0.01111111111 },		3.0,		1,	145.0,	1.0 },
				{	1,	{ 0.01111111111 },		0.6,		1,	132.0,	1.0 },
				{	1,	{ 0.01111111111 },		0.3,		1,	138.0,	1.0 },
				{	1,	{ 0.01111111111 },		1.0,		1,	157.0,	1.0 },
				{	1,	{ 0.01111111111 },		1.001,		1,	148.0,	1.0 },
				{	2,	{ 0.011111, 0.324 },	1.9999,		2,	127.0,	1.0 },
				{	2,	{ 0.012345, 0.457 },	0.456789,	1,	124.0,	0.5 },
				{	2,	{ 0.011111, 0.45 },		0.6,		1,	126.0,	0.5 },
				{	1,	{ 0.43111111111 },		1.33,		1,	121.0,	1.0 }
				}
			},

		{	SRC_SINC_BEST_QUALITY,
			9,
			BOOLEAN_TRUE,
			{	{	1,	{ 0.01111111111 },		3.0,		1,	149.0,	1.0 },
				{	1,	{ 0.01111111111 },		0.6,		1,	147.0,	1.0 },
				{	1,	{ 0.01111111111 },		0.3,		1,	148.0,	1.0 },
				{	1,	{ 0.01111111111 },		1.0,		1,	155.0,	1.0 },
				{	1,	{ 0.01111111111 },		1.001,		1,	148.0,	1.0 },
				{	2,	{ 0.011111, 0.324 },	1.9999,		2,	147.0,	1.0 },
				{	2,	{ 0.012345, 0.457 },	0.456789,	1,	148.0,	0.5 },
				{	2,	{ 0.011111, 0.45 },		0.6,		1,	149.0,	0.5 },
				{	1,	{ 0.43111111111 },		1.33,		1,	145.0,	1.0 }
				}
			},
		} ; /* snr_test_data */

	double	best_snr, snr, freq3dB ;
	int 	j, k, converter, verbose = 0 ;

	if (argc == 2 && strcmp (argv [1], "--verbose") == 0)
		verbose = 1 ;

	puts ("") ;

	for (j = 0 ; j < ARRAY_LEN (snr_test_data) ; j++)
	{	best_snr = 5000.0 ;

		converter = snr_test_data [j].converter ;

		printf ("    Converter %d : %s\n", converter, src_get_name (converter)) ;
		printf ("    %s\n", src_get_description (converter)) ;

		for (k = 0 ; k < snr_test_data [j].tests ; k++)
		{	snr = snr_test (&(snr_test_data [j].test_data [k]), k, converter, verbose) ;
			if (best_snr > snr)
				best_snr = snr ;
			} ;

		printf ("    Worst case Signal-to-Noise Ratio : %.2f dB.\n", best_snr) ;

		if (snr_test_data [j].do_bandwidth_test == BOOLEAN_FALSE)
		{	puts ("    Bandwith test not performed on this converter.\n") ;
			continue ;
			}

		freq3dB = bandwidth_test (converter, verbose) ;

		printf ("    Measured -3dB rolloff point      : %5.2f %%.\n\n", freq3dB) ;
		} ;

	return 0 ;
} /* main */

/*==============================================================================
*/

static double
snr_test (SINGLE_TEST *test_data, int number, int converter, int verbose)
{	static float data [BUFFER_LEN + 1] ;
	static float output [MAX_SPEC_LEN] ;

	SRC_STATE	*src_state ;
	SRC_DATA	src_data ;

	double		output_peak, snr ;
	int 		k, output_len, input_len, error ;

	if (verbose != 0)
	{	printf ("\tSignal-to-Noise Ratio Test %d.\n"
				"\t=====================================\n", number) ;
		printf ("\tFrequencies : [ ") ;
		for (k = 0 ; k < test_data->freq_count ; k++)
			printf ("%6.4f ", test_data->freqs [k]) ;

		printf ("]\n\tSRC Ratio   : %8.4f\n", test_data->src_ratio) ;
		}
	else
	{	printf ("\tSignal-to-Noise Ratio Test %d : ", number) ;
		fflush (stdout) ;
		} ;

	/* Set up the output array. */
	if (test_data->src_ratio >= 1.0)
	{	output_len = MAX_SPEC_LEN ;
		input_len = (int) ceil (MAX_SPEC_LEN / test_data->src_ratio) ;
		if (input_len > BUFFER_LEN)
			input_len = BUFFER_LEN ;
		}
	else
	{	input_len = BUFFER_LEN ;
		output_len = (int) ceil (BUFFER_LEN * test_data->src_ratio) ;
		output_len &= ((-1) << 4) ;
		if (output_len > MAX_SPEC_LEN)
			output_len = MAX_SPEC_LEN ;
		input_len = (int) ceil (output_len / test_data->src_ratio) ;
		} ;

	memset (output, 0, sizeof (output)) ;

	/* Generate input data array. */
	gen_windowed_sines (test_data->freq_count, test_data->freqs, 1.0, data, input_len) ;

	/* Perform sample rate conversion. */
	if ((src_state = src_new (converter, 1, &error)) == NULL)
	{	printf ("\n\nLine %d : src_new() failed : %s.\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	src_data.end_of_input = 1 ; /* Only one buffer worth of input. */

	src_data.data_in = data ;
	src_data.input_frames = input_len ;

	src_data.src_ratio = test_data->src_ratio ;

	src_data.data_out = output ;
	src_data.output_frames = output_len ;

	if ((error = src_process (src_state, &src_data)))
	{	printf ("\n\nLine %d : %s\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	src_state = src_delete (src_state) ;

	if (verbose != 0)
		printf ("\tOutput Len  :   %ld\n", src_data.output_frames_gen) ;

	if (abs (src_data.output_frames_gen - output_len) > 4)
	{	printf ("\n\nLine %d : output data length should be %d.\n\n", __LINE__, output_len) ;
		exit (1) ;
		} ;

	/* Check output peak. */
	output_peak = find_peak (output, src_data.output_frames_gen) ;

	if (verbose != 0)
		printf ("\tOutput Peak :   %6.4f\n", output_peak) ;

	if (fabs (output_peak - test_data->peak_value) > 0.01)
	{	printf ("\n\nLine %d : output peak (%6.4f) should be %6.4f\n\n", __LINE__, output_peak, test_data->peak_value) ;
		save_oct_float ("snr_test.dat", data, BUFFER_LEN, output, output_len) ;
		exit (1) ;
		} ;

	/* Calculate signal-to-noise ratio. */
	snr = calculate_snr (output, src_data.output_frames_gen, test_data->pass_band_peaks) ;

	if (snr < 0.0)
	{	/* An error occurred. */
		save_oct_float ("snr_test.dat", data, BUFFER_LEN, output, src_data.output_frames_gen) ;
		exit (1) ;
		} ;

	if (verbose != 0)
		printf ("\tSNR Ratio   :   %.2f dB\n", snr) ;

	if (snr < test_data->snr)
	{	printf ("\n\nLine %d : SNR (%5.2f) should be > %6.2f dB\n\n", __LINE__, snr, test_data->snr) ;
		exit (1) ;
		} ;

	if (verbose != 0)
		puts ("\t-------------------------------------\n\tPass\n") ;
	else
		puts ("Pass") ;

	return snr ;
} /* snr_test */

static double
find_peak (float *data, int len)
{	double 	peak = 0.0 ;
	int		k = 0 ;

	for (k = 0 ; k < len ; k++)
		if (fabs (data [k]) > peak)
			peak = fabs (data [k]) ;

	return peak ;
} /* find_peak */


static double
find_attenuation (double freq, int converter, int verbose)
{	static float input	[BUFFER_LEN] ;
	static float output [2 * BUFFER_LEN] ;

	SRC_DATA	src_data ;
	double 		output_peak ;
	int			error ;

	gen_windowed_sines (1, &freq, 1.0, input, BUFFER_LEN) ;

	src_data.end_of_input = 1 ; /* Only one buffer worth of input. */

	src_data.data_in = input ;
	src_data.input_frames = BUFFER_LEN ;

	src_data.src_ratio = 1.999 ;

	src_data.data_out = output ;
	src_data.output_frames = ARRAY_LEN (output) ;

	if ((error = src_simple (&src_data, converter, 1)))
	{	printf ("\n\nLine %d : %s\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	output_peak = find_peak (output, ARRAY_LEN (output)) ;

	if (verbose)
		printf ("\tFreq : %6f   InPeak : %6f    OutPeak : %6f   Atten : %6.2f dB\n",
				freq, 1.0, output_peak, 20.0 * log10 (1.0 / output_peak)) ;

	return 20.0 * log10 (1.0 / output_peak) ;
} /* find_attenuation */

static double
bandwidth_test (int converter, int verbose)
{	double	f1, f2, a1, a2 ;
	double	freq, atten ;

	f1 = 0.35 ;
	a1 = find_attenuation (f1, converter, verbose) ;

	f2 = 0.495 ;
	a2 = find_attenuation (f2, converter, verbose) ;

	if (a1 > 3.0 || a2 < 3.0)
	{	printf ("\n\nLine %d : cannot bracket 3dB point.\n\n", __LINE__) ;
		exit (1) ;
		} ;

	while (a2 - a1 > 1.0)
	{	freq = f1 + 0.5 * (f2 - f1) ;
		atten = find_attenuation (freq, converter, verbose) ;

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

#else /* (HAVE_FFTW3) == 0 */

/* Alternative main function when librfftw is not available. */

int
main (void)
{	puts ("\n"
		"****************************************************************\n"
		" This test cannot be run without FFTW (http://www.fftw.org/).\n"
		" Both the real and the complex versions of the library are\n"
		" required.") ;
	puts ("****************************************************************\n") ;

	return 0 ;
} /* main */

#endif

