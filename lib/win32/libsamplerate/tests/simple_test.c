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
#include <math.h>

#include <samplerate.h>

#include "util.h"

#define	BUFFER_LEN		2048

static void simple_test (int converter, double ratio) ;

int
main (void)
{	static double src_ratios [] =
	{	1.0001, 0.099, 0.1, 0.33333333, 0.789, 1.9, 3.1, 9.9
	} ;

	int k ;

	puts ("") ;

	puts ("    Zero Order Hold interpolator :") ;
	for (k = 0 ; k < ARRAY_LEN (src_ratios) ; k++)
		simple_test (SRC_ZERO_ORDER_HOLD, src_ratios [k]) ;

	puts ("    Linear interpolator :") ;
	for (k = 0 ; k < ARRAY_LEN (src_ratios) ; k++)
		simple_test (SRC_LINEAR, src_ratios [k]) ;

	puts ("    Sinc interpolator :") ;
	for (k = 0 ; k < ARRAY_LEN (src_ratios) ; k++)
		simple_test (SRC_SINC_FASTEST, src_ratios [k]) ;

	puts ("") ;

	return 0 ;
} /* main */

static void
simple_test (int converter, double src_ratio)
{	static float input [BUFFER_LEN], output [BUFFER_LEN] ;

	SRC_DATA	src_data ;

	int input_len, output_len, error, terminate ;

	printf ("\tsimple_test      (SRC ratio = %6.4f) ........... ", src_ratio) ;
	fflush (stdout) ;

	/* Calculate maximun input and output lengths. */
	if (src_ratio >= 1.0)
	{	output_len = BUFFER_LEN ;
		input_len = (int) floor (BUFFER_LEN / src_ratio) ;
		}
	else
	{	input_len = BUFFER_LEN ;
		output_len = (int) floor (BUFFER_LEN * src_ratio) ;
		} ;

	/* Reduce input_len by 10 so output is longer than necessary. */
	input_len -= 10 ;

	if (output_len > BUFFER_LEN)
	{	printf ("\n\nLine %d : output_len > BUFFER_LEN\n\n", __LINE__) ;
		exit (1) ;
		} ;

	memset (&src_data, 0, sizeof (src_data)) ;

	src_data.data_in = input ;
	src_data.input_frames = input_len ;

	src_data.src_ratio = src_ratio ;

	src_data.data_out = output ;
	src_data.output_frames = BUFFER_LEN ;

	if ((error = src_simple (&src_data, converter, 1)))
	{	printf ("\n\nLine %d : %s\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	terminate = (int) ceil ((src_ratio >= 1.0) ? src_ratio : 1.0 / src_ratio) ;

	if (fabs (src_data.output_frames_gen - src_ratio * input_len) > 2 * terminate)
	{	printf ("\n\nLine %d : bad output data length %ld should be %d.\n", __LINE__,
					src_data.output_frames_gen, (int) floor (src_ratio * input_len)) ;
		printf ("\tsrc_ratio  : %.4f\n", src_ratio) ;
		printf ("\tinput_len  : %d\n\toutput_len : %d\n\n", input_len, output_len) ;
		exit (1) ;
		} ;

	puts ("ok") ;

	return ;
} /* simple_test */

