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
#include <string.h>
#include <ctype.h>
#include <math.h>

#include	"util.h"

#ifndef	M_PI
#define	M_PI			3.14159265358979323846264338
#endif

void
gen_windowed_sines (int freq_count, const double *freqs, double max, float *output, int output_len)
{	int 	k, freq ;
	double	amplitude, phase ;

	amplitude = max / freq_count ;

	for (k = 0 ; k < output_len ; k++)
		output [k] = 0.0 ;

	for (freq = 0 ; freq < freq_count ; freq++)
	{	phase = 0.9 * M_PI / freq_count ;

		if (freqs [freq] <= 0.0 || freqs [freq] >= 0.5)
		{	printf ("\n%s : Error : freq [%d] == %g is out of range. Should be < 0.5.\n", __FILE__, freq, freqs [freq]) ;
			exit (1) ;
			} ;

		for (k = 0 ; k < output_len ; k++)
			output [k] += amplitude * sin (freqs [freq] * (2 * k) * M_PI + phase) ;
		} ;

	/* Apply Hanning Window. */
	for (k = 0 ; k < output_len ; k++)
		output [k] *= 0.5 - 0.5 * cos ((2 * k) * M_PI / (output_len - 1)) ;

	/*	data [k] *= 0.3635819 - 0.4891775 * cos ((2 * k) * M_PI / (output_len - 1))
					+ 0.1365995 * cos ((4 * k) * M_PI / (output_len - 1))
					- 0.0106411 * cos ((6 * k) * M_PI / (output_len - 1)) ;
		*/

	return ;
} /* gen_windowed_sines */

void
save_oct_float (char *filename, float *input, int in_len, float *output, int out_len)
{	FILE 	*file ;
	int		k ;

	printf ("Dumping input and output data to file : %s.\n\n", filename) ;

	if (! (file = fopen (filename, "w")))
		return ;

	fprintf (file, "# Not created by Octave\n") ;

	fprintf (file, "# name: input\n") ;
	fprintf (file, "# type: matrix\n") ;
	fprintf (file, "# rows: %d\n", in_len) ;
	fprintf (file, "# columns: 1\n") ;

	for (k = 0 ; k < in_len ; k++)
		fprintf (file, "% g\n", input [k]) ;

	fprintf (file, "# name: output\n") ;
	fprintf (file, "# type: matrix\n") ;
	fprintf (file, "# rows: %d\n", out_len) ;
	fprintf (file, "# columns: 1\n") ;

	for (k = 0 ; k < out_len ; k++)
		fprintf (file, "% g\n", output [k]) ;

	fclose (file) ;
	return ;
} /* save_oct_float */

void
save_oct_double (char *filename, double *input, int in_len, double *output, int out_len)
{	FILE 	*file ;
	int		k ;

	printf ("Dumping input and output data to file : %s.\n\n", filename) ;

	if (! (file = fopen (filename, "w")))
		return ;

	fprintf (file, "# Not created by Octave\n") ;

	fprintf (file, "# name: input\n") ;
	fprintf (file, "# type: matrix\n") ;
	fprintf (file, "# rows: %d\n", in_len) ;
	fprintf (file, "# columns: 1\n") ;

	for (k = 0 ; k < in_len ; k++)
		fprintf (file, "% g\n", input [k]) ;

	fprintf (file, "# name: output\n") ;
	fprintf (file, "# type: matrix\n") ;
	fprintf (file, "# rows: %d\n", out_len) ;
	fprintf (file, "# columns: 1\n") ;

	for (k = 0 ; k < out_len ; k++)
		fprintf (file, "% g\n", output [k]) ;

	fclose (file) ;
	return ;
} /* save_oct_double */

void
interleave_data (const float *in, float *out, int frames, int channels)
{	int fr, ch ;

	for (fr = 0 ; fr < frames ; fr++)
		for (ch = 0 ; ch < channels ; ch++)
			out [ch + channels * fr] = in [fr + frames * ch] ;

	return ;
} /* interleave_data */

void
deinterleave_data (const float *in, float *out, int frames, int channels)
{	int fr, ch ;

	for (ch = 0 ; ch < channels ; ch++)
		for (fr = 0 ; fr < frames ; fr++)
			out [fr + frames * ch] = in [ch + channels * fr] ;

	return ;
} /* deinterleave_data */

void
reverse_data (float *data, int datalen)
{	int left, right ;
	float temp ;

	left = 0 ;
	right = datalen - 1 ;

	while (left < right)
	{	temp = data [left] ;
		data [left] = data [right] ;
		data [right] = temp ;
		left ++ ;
		right -- ;
		} ;

} /* reverse_data */

void
print_cpu_name (void)
{	char buffer [512] ;
	FILE * file ;

	if ((file = fopen ("/proc/cpuinfo", "r")) == NULL)
	{	puts ("Unknown") ;
		return ;
		} ;

	while (fgets (buffer, sizeof (buffer), file) != NULL)
		if (strstr (buffer, "model name") == buffer)
		{	const char * cptr ;
	
			if ((cptr = strchr (buffer, ':')) != NULL)
			{	cptr ++ ;
				while (isspace (cptr [0])) cptr ++ ;
				printf ("%s", cptr) ;
				goto complete ;
				} ;
			} ;

	fclose (file) ;
	puts ("Unknown") ;
	return ;

complete :
	fclose (file) ;
	return ;
} /* print_cpu_name */

