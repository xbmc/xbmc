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
#include <samplerate.h>

#include "util.h"

static void
downsample_test (int converter)
{	static float in [1000], out [10] ;
	SRC_DATA data ;

    printf ("        downsample_test     (%-28s) ....... ", src_get_name (converter)) ;
	fflush (stdout) ;

	data.src_ratio = 1.0 / 255.0 ;
	data.input_frames = ARRAY_LEN (in) ;
	data.output_frames = ARRAY_LEN (out) ;
	data.data_in = in ;
	data.data_out = out ;

	if (src_simple (&data, converter, 1))
	{	puts ("src_simple failed.") ;
		exit (1) ;
		} ;

	puts ("ok") ;
} /* downsample_test */

int
main (void)
{
	puts ("") ;

	downsample_test (SRC_ZERO_ORDER_HOLD) ;
	downsample_test (SRC_LINEAR) ;
	downsample_test (SRC_SINC_FASTEST) ;
	downsample_test (SRC_SINC_MEDIUM_QUALITY) ;
	downsample_test (SRC_SINC_BEST_QUALITY) ;

	puts ("") ;

	return 0 ;
} /* main */
