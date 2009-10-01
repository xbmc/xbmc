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
#include <assert.h>

#include <samplerate.h>

#include "util.h"
#define	BUFFER_LEN		50000
#define	BLOCK_LEN		(12)

#define	MAX_CHANNELS	10

static void simple_test (int converter, int channel_count, double target_snr) ;
static void process_test (int converter, int channel_count, double target_snr) ;
static void callback_test (int converter, int channel_count, double target_snr) ;

int
main (void)
{	double target ;
	int k ;

	puts ("\n    Zero Order Hold interpolator :") ;
	target = 38.0 ;
	for (k = 1 ; k <= 3 ; k++)
	{	simple_test		(SRC_ZERO_ORDER_HOLD, k, target) ;
		process_test	(SRC_ZERO_ORDER_HOLD, k, target) ;
		callback_test	(SRC_ZERO_ORDER_HOLD, k, target) ;
		} ;

	puts ("\n    Linear interpolator :") ;
	target = 79.0 ;
	for (k = 1 ; k <= 3 ; k++)
	{	simple_test		(SRC_LINEAR, k, target) ;
		process_test	(SRC_LINEAR, k, target) ;
		callback_test	(SRC_LINEAR, k, target) ;
		} ;

	puts ("\n    Sinc interpolator :") ;
	target = 100.0 ;
	for (k = 1 ; k <= MAX_CHANNELS ; k++)
	{	simple_test		(SRC_SINC_FASTEST, k, target) ;
		process_test	(SRC_SINC_FASTEST, k, target) ;
		callback_test	(SRC_SINC_FASTEST, k, target) ;
		} ;

	puts ("") ;

	return 0 ;
} /* main */

/*==============================================================================
*/

static float input_serial		[BUFFER_LEN * MAX_CHANNELS] ;
static float input_interleaved	[BUFFER_LEN * MAX_CHANNELS] ;
static float output_interleaved	[BUFFER_LEN * MAX_CHANNELS] ;
static float output_serial		[BUFFER_LEN * MAX_CHANNELS] ;

static void
simple_test (int converter, int channel_count, double target_snr)
{	SRC_DATA	src_data ;

	double	freq, snr ;
	int		ch, error, frames ;

	printf ("\t%-22s (%2d channel%c) ............ ", "simple_test", channel_count, channel_count > 1 ? 's' : ' ') ;
	fflush (stdout) ;

	assert (channel_count <= MAX_CHANNELS) ;

	memset (input_serial, 0, sizeof (input_serial)) ;
	memset (input_interleaved, 0, sizeof (input_interleaved)) ;
	memset (output_interleaved, 0, sizeof (output_interleaved)) ;
	memset (output_serial, 0, sizeof (output_serial)) ;

	frames = BUFFER_LEN ;

	/* Calculate channel_count separate windowed sine waves. */
	for (ch = 0 ; ch < channel_count ; ch++)
	{	freq = (200.0 + 33.333333333 * ch) / 44100.0 ;
		gen_windowed_sines (1, &freq, 1.0, input_serial + ch * frames, frames) ;
		} ;

	/* Interleave the data in preparation for SRC. */
	interleave_data (input_serial, input_interleaved, frames, channel_count) ;

	/* Choose a converstion ratio <= 1.0. */
	src_data.src_ratio = 0.95 ;

	src_data.data_in = input_interleaved ;
	src_data.input_frames = frames ;

	src_data.data_out = output_interleaved ;
	src_data.output_frames = frames ;

	if ((error = src_simple (&src_data, converter, channel_count)))
	{	printf ("\n\nLine %d : %s\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	if (fabs (src_data.output_frames_gen - src_data.src_ratio * src_data.input_frames) > 2)
	{	printf ("\n\nLine %d : bad output data length %ld should be %d.\n", __LINE__,
					src_data.output_frames_gen, (int) floor (src_data.src_ratio * src_data.input_frames)) ;
		printf ("\tsrc_ratio  : %.4f\n", src_data.src_ratio) ;
		printf ("\tinput_len  : %ld\n", src_data.input_frames) ;
		printf ("\toutput_len : %ld\n\n", src_data.output_frames_gen) ;
		exit (1) ;
		} ;

	/* De-interleave data so SNR can be calculated for each channel. */
	deinterleave_data (output_interleaved, output_serial, frames, channel_count) ;

	for (ch = 0 ; ch < channel_count ; ch++)
	{	snr = calculate_snr (output_serial + ch * frames, frames, 1) ;
		if (snr < target_snr)
		{	printf ("\n\nLine %d: channel %d snr %f should be %f\n", __LINE__, ch, snr, target_snr) ;
			save_oct_float ("output.dat", input_serial, channel_count * frames, output_serial, channel_count * frames) ;
			exit (1) ;
			} ;
		} ;

	puts ("ok") ;

	return ;
} /* simple_test */

/*==============================================================================
*/

static void
process_test (int converter, int channel_count, double target_snr)
{	SRC_STATE	*src_state ;
	SRC_DATA	src_data ;

	double	freq, snr ;
	int		ch, error, frames, current_in, current_out ;

	printf ("\t%-22s (%2d channel%c) ............ ", "process_test", channel_count, channel_count > 1 ? 's' : ' ') ;
	fflush (stdout) ;

	assert (channel_count <= MAX_CHANNELS) ;

	memset (input_serial, 0, sizeof (input_serial)) ;
	memset (input_interleaved, 0, sizeof (input_interleaved)) ;
	memset (output_interleaved, 0, sizeof (output_interleaved)) ;
	memset (output_serial, 0, sizeof (output_serial)) ;

	frames = BUFFER_LEN ;

	/* Calculate channel_count separate windowed sine waves. */
	for (ch = 0 ; ch < channel_count ; ch++)
	{	freq = (400.0 + 11.333333333 * ch) / 44100.0 ;
		gen_windowed_sines (1, &freq, 1.0, input_serial + ch * frames, frames) ;
		} ;

	/* Interleave the data in preparation for SRC. */
	interleave_data (input_serial, input_interleaved, frames, channel_count) ;

	/* Perform sample rate conversion. */
	if ((src_state = src_new (converter, channel_count, &error)) == NULL)
	{	printf ("\n\nLine %d : src_new() failed : %s\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	src_data.end_of_input = 0 ; /* Set this later. */

	/* Choose a converstion ratio < 1.0. */
	src_data.src_ratio = 0.95 ;

	src_data.data_in = input_interleaved ;
	src_data.data_out = output_interleaved ;

	current_in = current_out = 0 ;

	while (1)
	{	src_data.input_frames	= MAX (MIN (BLOCK_LEN, frames - current_in), 0) ;
		src_data.output_frames	= MAX (MIN (BLOCK_LEN, frames - current_out), 0) ;

		if ((error = src_process (src_state, &src_data)))
		{	printf ("\n\nLine %d : %s\n\n", __LINE__, src_strerror (error)) ;
			exit (1) ;
			} ;

		if (src_data.end_of_input && src_data.output_frames_gen == 0)
			break ;

		current_in	+= src_data.input_frames_used ;
		current_out += src_data.output_frames_gen ;

		src_data.data_in	+= src_data.input_frames_used * channel_count ;
		src_data.data_out	+= src_data.output_frames_gen * channel_count ;

		src_data.end_of_input = (current_in >= frames) ? 1 : 0 ;
		} ;

	src_state = src_delete (src_state) ;

	if (fabs (current_out - src_data.src_ratio * current_in) > 2)
	{	printf ("\n\nLine %d : bad output data length %d should be %d.\n", __LINE__,
					current_out, (int) floor (src_data.src_ratio * current_in)) ;
		printf ("\tsrc_ratio  : %.4f\n", src_data.src_ratio) ;
		printf ("\tinput_len  : %d\n", frames) ;
		printf ("\toutput_len : %d\n\n", current_out) ;
		exit (1) ;
		} ;

	/* De-interleave data so SNR can be calculated for each channel. */
	deinterleave_data (output_interleaved, output_serial, frames, channel_count) ;

	for (ch = 0 ; ch < channel_count ; ch++)
	{	snr = calculate_snr (output_serial + ch * frames, frames, 1) ;
		if (snr < target_snr)
		{	printf ("\n\nLine %d: channel %d snr %f should be %f\n", __LINE__, ch, snr, target_snr) ;
			save_oct_float ("output.dat", input_serial, channel_count * frames, output_serial, channel_count * frames) ;
			exit (1) ;
			} ;
		} ;

	puts ("ok") ;

	return ;
} /* process_test */

/*==============================================================================
*/

typedef struct
{	int channels ;
	long total_frames ;
	long current_frame ;
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

	*data = pcb_data->data + (pcb_data->current_frame * pcb_data->channels) ;

	if (pcb_data->total_frames - pcb_data->current_frame < BLOCK_LEN)
		frames = pcb_data->total_frames - pcb_data->current_frame ;
	else
		frames = BLOCK_LEN ;

	pcb_data->current_frame += frames ;

	return frames ;
} /* test_callback_func */

static void
callback_test (int converter, int channel_count, double target_snr)
{	TEST_CB_DATA test_callback_data ;
	SRC_STATE	*src_state = NULL ;

	double	freq, snr, src_ratio ;
	int		ch, error, frames, read_total, read_count ;

	printf ("\t%-22s (%2d channel%c) ............ ", "callback_test", channel_count, channel_count > 1 ? 's' : ' ') ;
	fflush (stdout) ;

	assert (channel_count <= MAX_CHANNELS) ;

	memset (input_serial, 0, sizeof (input_serial)) ;
	memset (input_interleaved, 0, sizeof (input_interleaved)) ;
	memset (output_interleaved, 0, sizeof (output_interleaved)) ;
	memset (output_serial, 0, sizeof (output_serial)) ;
	memset (&test_callback_data, 0, sizeof (test_callback_data)) ;

	frames = BUFFER_LEN ;

	/* Calculate channel_count separate windowed sine waves. */
	for (ch = 0 ; ch < channel_count ; ch++)
	{	freq = (200.0 + 33.333333333 * ch) / 44100.0 ;
		gen_windowed_sines (1, &freq, 1.0, input_serial + ch * frames, frames) ;
		} ;

	/* Interleave the data in preparation for SRC. */
	interleave_data (input_serial, input_interleaved, frames, channel_count) ;

	/* Perform sample rate conversion. */
	src_ratio = 0.95 ;
	test_callback_data.channels = channel_count ;
	test_callback_data.total_frames = frames ;
	test_callback_data.current_frame = 0 ;
	test_callback_data.data = input_interleaved ;

	if ((src_state = src_callback_new (test_callback_func, converter, channel_count, &error, &test_callback_data)) == NULL)
	{	printf ("\n\nLine %d : %s\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	read_total = 0 ;
	while (read_total < frames)
	{	read_count = src_callback_read (src_state, src_ratio, frames - read_total, output_interleaved + read_total * channel_count) ;

		if (read_count <= 0)
			break ;

		read_total += read_count ;
		} ;

	if ((error = src_error (src_state)) != 0)
	{	printf ("\n\nLine %d : %s\n\n", __LINE__, src_strerror (error)) ;
		exit (1) ;
		} ;

	src_state = src_delete (src_state) ;

	if (fabs (read_total - src_ratio * frames) > 2)
	{	printf ("\n\nLine %d : bad output data length %d should be %d.\n", __LINE__,
					read_total, (int) floor (src_ratio * frames)) ;
		printf ("\tsrc_ratio  : %.4f\n", src_ratio) ;
		printf ("\tinput_len  : %d\n", frames) ;
		printf ("\toutput_len : %d\n\n", read_total) ;
		exit (1) ;
		} ;

	/* De-interleave data so SNR can be calculated for each channel. */
	deinterleave_data (output_interleaved, output_serial, frames, channel_count) ;

	for (ch = 0 ; ch < channel_count ; ch++)
	{	snr = calculate_snr (output_serial + ch * frames, frames, 1) ;
		if (snr < target_snr)
		{	printf ("\n\nLine %d: channel %d snr %f should be %f\n", __LINE__, ch, snr, target_snr) ;
			save_oct_float ("output.dat", input_serial, channel_count * frames, output_serial, channel_count * frames) ;
			exit (1) ;
			} ;
		} ;

	puts ("ok") ;

	return ;
} /* callback_test */

