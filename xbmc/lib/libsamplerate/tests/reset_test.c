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

#include <samplerate.h>

#include "util.h"

#define	BUFFER_LEN		2048
#define CB_READ_LEN		256

static void process_reset_test (int converter) ;
static void callback_reset_test (int converter) ;

static float data_one [BUFFER_LEN] ;
static float data_zero [BUFFER_LEN] ;

int
main (void)
{
	puts ("") ;

	process_reset_test (SRC_ZERO_ORDER_HOLD) ;
	process_reset_test (SRC_LINEAR) ;
	process_reset_test (SRC_SINC_FASTEST) ;

	callback_reset_test (SRC_ZERO_ORDER_HOLD) ;
	callback_reset_test (SRC_LINEAR) ;
	callback_reset_test (SRC_SINC_FASTEST) ;

	puts ("") ;

	return 0 ;
} /* main */

static void
process_reset_test (int converter)
{	static float output [BUFFER_LEN] ;

	SRC_STATE *src_state ;
	SRC_DATA src_data ;
	int k, error ;

	printf ("\tprocess_reset_test  (%-28s) ....... ", src_get_name (converter)) ;
	fflush (stdout) ;

	for (k = 0 ; k < BUFFER_LEN ; k++)
	{	data_one [k] = 1.0 ;
		data_zero [k] = 0.0 ;
		} ;

	/* Get a converter. */
	if ((src_state = src_new (converter, 1, &error)) == NULL)
	{	printf ("\n\nLine %d : src_new() failed : %s.\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	/* Process a bunch of 1.0 valued samples. */
	src_data.data_in		= data_one ;
	src_data.data_out		= output ;
	src_data.input_frames	= BUFFER_LEN ;
	src_data.output_frames	= BUFFER_LEN ;
	src_data.src_ratio		= 0.9 ;
	src_data.end_of_input	= 1 ;

	if ((error = src_process (src_state, &src_data)) != 0)
	{	printf ("\n\nLine %d : src_simple () returned error : %s\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	/* Reset the state of the converter.*/
	src_reset (src_state) ;

	/* Now process some zero data. */
	src_data.data_in		= data_zero ;
	src_data.data_out		= output ;
	src_data.input_frames	= BUFFER_LEN ;
	src_data.output_frames	= BUFFER_LEN ;
	src_data.src_ratio		= 0.9 ;
	src_data.end_of_input	= 1 ;

	if ((error = src_process (src_state, &src_data)) != 0)
	{	printf ("\n\nLine %d : src_simple () returned error : %s\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	/* Finally make sure that the output data is zero ie reset was sucessful. */
	for (k = 0 ; k < BUFFER_LEN / 2 ; k++)
		if (output [k] != 0.0)
		{	printf ("\n\nLine %d : output [%d] should be 0.0, is %f.\n", __LINE__, k, output [k]) ;
			exit (1) ;
			} ;

	/* Make sure that this function has been exported. */
	src_set_ratio (src_state, 1.0) ;

	/* Delete converter. */
	src_state = src_delete (src_state) ;

	puts ("ok") ;
} /* process_reset_test */

/*==============================================================================
*/

typedef struct
{	int channels ;
	long count, total ;
	float *data ;
} TEST_CB_DATA ;

static long
test_callback_func (void *cb_data, float **data)
{	TEST_CB_DATA *pcb_data ;

	long frames ;

	if ((pcb_data = cb_data) == NULL)
		return 0 ;

	if (data == NULL)
		return 0 ;

	if (pcb_data->total - pcb_data->count > 0)
		frames = pcb_data->total - pcb_data->count ;
	else
		frames = 0 ;

	*data = pcb_data->data + pcb_data->count ;
	pcb_data->count += frames ;

	return frames ;
} /* test_callback_func */

static void
callback_reset_test (int converter)
{	static TEST_CB_DATA test_callback_data ;

	static float output [BUFFER_LEN] ;

	SRC_STATE *src_state ;

	double src_ratio = 1.1 ;
	long read_count, read_total ;
	int k, error ;

	printf ("\tcallback_reset_test (%-28s) ....... ", src_get_name (converter)) ;
	fflush (stdout) ;

	for (k = 0 ; k < ARRAY_LEN (data_one) ; k++)
	{	data_one [k] = 1.0 ;
		data_zero [k] = 0.0 ;
		} ;

	if ((src_state = src_callback_new (test_callback_func, converter, 1, &error, &test_callback_data)) == NULL)
	{	printf ("\n\nLine %d : %s\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	/* Process a bunch of 1.0 valued samples. */
	test_callback_data.channels = 1 ;
	test_callback_data.count = 0 ;
	test_callback_data.total = ARRAY_LEN (data_one) ;
	test_callback_data.data = data_one ;

	read_total = 0 ;
	do
	{	read_count = (ARRAY_LEN (output) - read_total > CB_READ_LEN) ? CB_READ_LEN : ARRAY_LEN (output) - read_total ;
		read_count = src_callback_read (src_state, src_ratio, read_count, output + read_total) ;
		read_total += read_count ;
		}
	while (read_count > 0) ;

	/* Check for errors. */
	if ((error = src_error (src_state)) != 0)
	{	printf ("\n\nLine %d : %s\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	/* Reset the state of the converter.*/
	src_reset (src_state) ;

	/* Process a bunch of 0.0 valued samples. */
	test_callback_data.channels = 1 ;
	test_callback_data.count = 0 ;
	test_callback_data.total = ARRAY_LEN (data_zero) ;
	test_callback_data.data = data_zero ;

	/* Now process some zero data. */
	read_total = 0 ;
	do
	{	read_count = (ARRAY_LEN (output) - read_total > CB_READ_LEN) ? CB_READ_LEN : ARRAY_LEN (output) - read_total ;
		read_count = src_callback_read (src_state, src_ratio, read_count, output + read_total) ;
		read_total += read_count ;
		}
	while (read_count > 0) ;

	/* Check for errors. */
	if ((error = src_error (src_state)) != 0)
	{	printf ("\n\nLine %d : %s\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	/* Finally make sure that the output data is zero ie reset was sucessful. */
	for (k = 0 ; k < BUFFER_LEN / 2 ; k++)
		if (output [k] != 0.0)
		{	printf ("\n\nLine %d : output [%d] should be 0.0, is %f.\n\n", __LINE__, k, output [k]) ;
			save_oct_float ("output.dat", data_one, ARRAY_LEN (data_one), output, ARRAY_LEN (output)) ;
			exit (1) ;
			} ;

	/* Make sure that this function has been exported. */
	src_set_ratio (src_state, 1.0) ;

	/* Delete converter. */
	src_state = src_delete (src_state) ;

	puts ("ok") ;
} /* callback_reset_test */


