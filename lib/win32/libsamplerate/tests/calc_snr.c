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

#include "util.h"

#if (HAVE_FFTW3 == 1)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <fftw3.h>

#define	MAX_SPEC_LEN	(1<<18)
#define	MAX_PEAKS		10

static void log_mag_spectrum (double *input, int len, double *magnitude) ;
static void smooth_mag_spectrum (double *magnitude, int len) ;
static double find_snr (const double *magnitude, int len, int expected_peaks) ;

typedef struct
{	double	peak ;
	int		index ;
} PEAK_DATA ;

double
calculate_snr (float *data, int len, int expected_peaks)
{	static double magnitude [MAX_SPEC_LEN] ;
	static double datacopy [MAX_SPEC_LEN] ;

	double snr = 200.0 ;
	int k ;

	if (len > MAX_SPEC_LEN)
	{	printf ("%s : line %d : data length too large.\n", __FILE__, __LINE__) ;
		exit (1) ;
		} ;

	for (k = 0 ; k < len ; k++)
		datacopy [k] = data [k] ;

	/* Pad the data just a little to speed up the FFT. */
	while ((len & 0x1F) && len < MAX_SPEC_LEN)
	{	datacopy [len] = 0.0 ;
		len ++ ;
		} ;

	log_mag_spectrum (datacopy, len, magnitude) ;
	smooth_mag_spectrum (magnitude, len / 2) ;

	snr = find_snr (magnitude, len, expected_peaks) ;

	return snr ;
} /* calculate_snr */

/*==============================================================================
** There is a slight problem with trying to measure SNR with the method used
** here; the side lobes of the windowed FFT can look like a noise/aliasing peak.
** The solution is to smooth the magnitude spectrum by wiping out troughs
** between adjacent peaks as done here.
** This removes side lobe peaks without affecting noise/aliasing peaks.
*/

static void linear_smooth (double *mag, PEAK_DATA *larger, PEAK_DATA *smaller) ;

static void
smooth_mag_spectrum (double *mag, int len)
{	PEAK_DATA peaks [2] ;

	int k ;

	memset (peaks, 0, sizeof (peaks)) ;

	/* Find first peak. */
	for (k = 1 ; k < len - 1 ; k++)
	{	if (mag [k - 1] < mag [k] && mag [k] >= mag [k + 1])
		{	peaks [0].peak = mag [k] ;
			peaks [0].index = k ;
			break ;
			} ;
		} ;

	/* Find subsequent peaks ans smooth between peaks. */
	for (k = peaks [0].index + 1 ; k < len - 1 ; k++)
	{	if (mag [k - 1] < mag [k] && mag [k] >= mag [k + 1])
		{	peaks [1].peak = mag [k] ;
			peaks [1].index = k ;

			if (peaks [1].peak > peaks [0].peak)
				linear_smooth (mag, &peaks [1], &peaks [0]) ;
			else
				linear_smooth (mag, &peaks [0], &peaks [1]) ;
			peaks [0] = peaks [1] ;
			} ;
		} ;

} /* smooth_mag_spectrum */

static void
linear_smooth (double *mag, PEAK_DATA *larger, PEAK_DATA *smaller)
{	int k ;

	if (smaller->index < larger->index)
	{	for (k = smaller->index + 1 ; k < larger->index ; k++)
			mag [k] = (mag [k] < mag [k - 1]) ? 0.999 * mag [k - 1] : mag [k] ;
		}
	else
	{	for (k = smaller->index - 1 ; k >= larger->index ; k--)
			mag [k] = (mag [k] < mag [k + 1]) ? 0.999 * mag [k + 1] : mag [k] ;
		} ;

} /* linear_smooth */

/*==============================================================================
*/

static int
peak_compare (const void *vp1, const void *vp2)
{	const PEAK_DATA *peak1, *peak2 ;

	peak1 = (const PEAK_DATA*) vp1 ;
	peak2 = (const PEAK_DATA*) vp2 ;

	return (peak1->peak < peak2->peak) ? 1 : -1 ;
} /* peak_compare */

static double
find_snr (const double *magnitude, int len, int expected_peaks)
{	PEAK_DATA peaks [MAX_PEAKS] ;

	int		k, peak_count = 0 ;
	double	snr ;

	memset (peaks, 0, sizeof (peaks)) ;

	/* Find the MAX_PEAKS largest peaks. */
	for (k = 1 ; k < len - 1 ; k++)
	{	if (magnitude [k - 1] < magnitude [k] && magnitude [k] >= magnitude [k + 1])
		{	if (peak_count < MAX_PEAKS)
			{	peaks [peak_count].peak = magnitude [k] ;
				peaks [peak_count].index = k ;
				peak_count ++ ;
				qsort (peaks, peak_count, sizeof (PEAK_DATA), peak_compare) ;
				}
			else if (magnitude [k] > peaks [MAX_PEAKS - 1].peak)
			{	peaks [MAX_PEAKS - 1].peak = magnitude [k] ;
				peaks [MAX_PEAKS - 1].index = k ;
				qsort (peaks, MAX_PEAKS, sizeof (PEAK_DATA), peak_compare) ;
				} ;
			} ;
		} ;

	if (peak_count < expected_peaks)
	{	printf ("\n%s : line %d : bad peak_count (%d), expected %d.\n\n", __FILE__, __LINE__, peak_count, expected_peaks) ;
		return -1.0 ;
		} ;

	/* Sort the peaks. */
	qsort (peaks, peak_count, sizeof (PEAK_DATA), peak_compare) ;

	snr = peaks [0].peak ;
	for (k = 1 ; k < peak_count ; k++)
		if (fabs (snr - peaks [k].peak) > 10.0)
			return fabs (peaks [k].peak) ;

	return snr ;
} /* find_snr */

static void
log_mag_spectrum (double *input, int len, double *magnitude)
{	fftw_plan plan = NULL ;

	double	maxval ;
	int		k ;

	if (input == NULL || magnitude == NULL)
		return ;

	plan = fftw_plan_r2r_1d (len, input, magnitude, FFTW_R2HC, FFTW_ESTIMATE | FFTW_PRESERVE_INPUT) ;
	if (plan == NULL)
	{	printf ("%s : line %d : create plan failed.\n", __FILE__, __LINE__) ;
		exit (1) ;
		} ;

	fftw_execute (plan) ;

	fftw_destroy_plan (plan) ;

	/* (k < N/2 rounded up) */
	maxval = 0.0 ;
	for (k = 1 ; k < len / 2 ; k++)
	{	magnitude [k] = sqrt (magnitude [k] * magnitude [k] + magnitude [len - k - 1] * magnitude [len - k - 1]) ;
		maxval = (maxval < magnitude [k]) ? magnitude [k] : maxval ;
		} ;

	memset (magnitude + len / 2, 0, len / 2 * sizeof (magnitude [0])) ;

	/* Don't care about DC component. Make it zero. */
	magnitude [0] = 0.0 ;

	/* log magnitude. */
	for (k = 0 ; k < len ; k++)
	{	magnitude [k] = magnitude [k] / maxval ;
		magnitude [k] = (magnitude [k] < 1e-15) ? -200.0 : 20.0 * log10 (magnitude [k]) ;
		} ;

	return ;
} /* log_mag_spectrum */

#else /* ! (HAVE_LIBFFTW && HAVE_LIBRFFTW) */

double
calculate_snr (float *data, int len, int expected_peaks)
{	double snr = 200.0 ;

	data = data ;
	len = len ;
	expected_peaks = expected_peaks ;

	return snr ;
} /* calculate_snr */

#endif

