/*
** Copyright (C) 2003-2008 Erik de Castro Lopo <erikd@mega-nerd.com>
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

#define	BUFFER_LEN		10000
#define CB_READ_LEN		256

static void callback_test (int converter, double ratio) ;
static void end_of_stream_test (int converter) ;

int
main (void)
{	static double src_ratios [] =
	{	1.0, 0.099, 0.1, 0.33333333, 0.789, 1.0001, 1.9, 3.1, 9.9
	} ;

	int k ;

	puts ("") ;

	puts ("    Zero Order Hold interpolator :") ;
	for (k = 0 ; k < ARRAY_LEN (src_ratios) ; k++)
		callback_test (SRC_ZERO_ORDER_HOLD, src_ratios [k]) ;

	puts ("    Linear interpolator :") ;
	for (k = 0 ; k < ARRAY_LEN (src_ratios) ; k++)
		callback_test (SRC_LINEAR, src_ratios [k]) ;

	puts ("    Sinc interpolator :") ;
	for (k = 0 ; k < ARRAY_LEN (src_ratios) ; k++)
		callback_test (SRC_SINC_FASTEST, src_ratios [k]) ;

	puts ("") ;

	puts ("    End of stream test :") ;
	end_of_stream_test (SRC_ZERO_ORDER_HOLD) ;
	end_of_stream_test (SRC_LINEAR) ;
	end_of_stream_test (SRC_SINC_FASTEST) ;

	puts ("") ;
	return 0 ;
} /* main */

/*=====================================================================================
*/

typedef struct
{	int channels ;
	long count, total ;
	int end_of_data ;
	float data [BUFFER_LEN] ;
} TEST_CB_DATA ;

static long
test_callback_func (void *cb_data, float **data)
{	TEST_CB_DATA *pcb_data ;

	long frames ;

	if ((pcb_data = cb_data) == NULL)
		return 0 ;

	if (data == NULL)
		return 0 ;

	if (pcb_data->total - pcb_data->count > CB_READ_LEN)
		frames = CB_READ_LEN / pcb_data->channels ;
	else
		frames = (pcb_data->total - pcb_data->count) / pcb_data->channels ;

	*data = pcb_data->data + pcb_data->count ;
	pcb_data->count += frames ;

	return frames ;
} /* test_callback_func */


static void
callback_test (int converter, double src_ratio)
{	static TEST_CB_DATA test_callback_data ;
	static float output [BUFFER_LEN] ;

	SRC_STATE	*src_state ;

	long	read_count, read_total ;
	int 	error ;

	printf ("\tcallback_test    (SRC ratio = %6.4f) ........... ", src_ratio) ;
	fflush (stdout) ;

	test_callback_data.channels = 2 ;
	test_callback_data.count = 0 ;
	test_callback_data.end_of_data = 0 ;
	test_callback_data.total = ARRAY_LEN (test_callback_data.data) ;

	if ((src_state = src_callback_new (test_callback_func, converter, test_callback_data.channels, &error, &test_callback_data)) == NULL)
	{	printf ("\n\nLine %d : %s\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	read_total = 0 ;
	do
	{	/* We will be throwing away output data, so just grab as much as possible. */
		read_count = ARRAY_LEN (output) / test_callback_data.channels ;
		read_count = src_callback_read (src_state, src_ratio, read_count, output) ;
		read_total += read_count ;
		}
	while (read_count > 0) ;

	if ((error = src_error (src_state)) != 0)
	{	printf ("\n\nLine %d : %s\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	src_state = src_delete (src_state) ;

	if (fabs (read_total - src_ratio * ARRAY_LEN (test_callback_data.data)) > src_ratio)
	{	printf ("\n\nLine %d : input / output length mismatch.\n\n", __LINE__) ;
		printf ("    input len  : %d\n", ARRAY_LEN (test_callback_data.data)) ;
		printf ("    output len : %ld (should be %g +/- %g)\n\n", read_total,
					floor (0.5 + src_ratio * ARRAY_LEN (test_callback_data.data)), ceil (src_ratio)) ;
		exit (1) ;
		} ;

	puts ("ok") ;

	return ;
} /* callback_test */

/*=====================================================================================
*/

static long
eos_callback_func (void *cb_data, float **data)
{
	TEST_CB_DATA *pcb_data ;
	long frames ;

	if (data == NULL)
		return 0 ;

	if ((pcb_data = cb_data) == NULL)
		return 0 ;

	/*
	**	Return immediately if there is no more data.
	**	In this case, the output pointer 'data' will not be set and
	**	valgrind should not warn about it.
	*/
	if (pcb_data->end_of_data)
		return 0 ;

	if (pcb_data->total - pcb_data->count > CB_READ_LEN)
		frames = CB_READ_LEN / pcb_data->channels ;
	else
		frames = (pcb_data->total - pcb_data->count) / pcb_data->channels ;

	*data = pcb_data->data + pcb_data->count ;
	pcb_data->count += frames ;

	/*
	**	Set end_of_data so that the next call to the callback function will
	**	return zero ocunt without setting the 'data' pointer.
	*/
	if (pcb_data->total < 2 * pcb_data->count)
		pcb_data->end_of_data = 1 ;

	return frames ;
} /* eos_callback_data */


static void
end_of_stream_test (int converter)
{	static TEST_CB_DATA test_callback_data ;
	static float output [BUFFER_LEN] ;

	SRC_STATE	*src_state ;

	double	src_ratio = 0.3 ;
	long	read_count, read_total ;
	int 	error ;

	printf ("\t%-30s        ........... ", src_get_name (converter)) ;
	fflush (stdout) ;

	test_callback_data.channels = 2 ;
	test_callback_data.count = 0 ;
	test_callback_data.end_of_data = 0 ;
	test_callback_data.total = ARRAY_LEN (test_callback_data.data) ;

	if ((src_state = src_callback_new (eos_callback_func, converter, test_callback_data.channels, &error, &test_callback_data)) == NULL)
	{	printf ("\n\nLine %d : %s\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	read_total = 0 ;
	do
	{	/* We will be throwing away output data, so just grab as much as possible. */
		read_count = ARRAY_LEN (output) / test_callback_data.channels ;
		read_count = src_callback_read (src_state, src_ratio, read_count, output) ;
		read_total += read_count ;
		}
	while (read_count > 0) ;

	if ((error = src_error (src_state)) != 0)
	{	printf ("\n\nLine %d : %s\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	src_state = src_delete (src_state) ;

	if (test_callback_data.end_of_data == 0)
	{	printf ("\n\nLine %d : test_callback_data.end_of_data should not be 0."
				" This is a bug in the test.\n\n", __LINE__) ;
		exit (1) ;
		} ;

	puts ("ok") ;
	return ;
} /* end_of_stream_test */
