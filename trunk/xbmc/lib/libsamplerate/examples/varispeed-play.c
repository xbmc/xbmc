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

#include "config.h"

#include <float_cast.h>

#if (HAVE_SNDFILE)

#include <samplerate.h>
#include <sndfile.h>

#include "audio_out.h"

#define ARRAY_LEN(x)	((int) (sizeof (x) / sizeof ((x) [0])))

#define	BUFFER_LEN		4096
#define	INPUT_FRAMES	100

#define	MIN(a,b)		((a) < (b) ? (a) : (b))

#define	MAGIC_NUMBER	((int) ('S' << 16) + ('R' << 8) + ('C'))

#ifndef	M_PI
#define	M_PI			3.14159265358979323846264338
#endif


typedef struct
{	int			magic ;

	SNDFILE 	*sndfile ;
	SF_INFO 	sfinfo ;

	SRC_STATE	*src_state ;
	SRC_DATA	src_data ;

	int			freq_point ;
	int			buffer_out_start, buffer_out_end ;

	float		buffer_in	[BUFFER_LEN] ;
	float		buffer_out	[BUFFER_LEN] ;
} CALLBACK_DATA ;

static int varispeed_get_data (CALLBACK_DATA *data, float *samples, int frames) ;
static void varispeed_play (const char *filename, int converter) ;

int
main (int argc, char *argv [])
{	const char	*cptr, *progname, *filename ;
	int			k, converter ;

	converter = SRC_SINC_FASTEST ;

	progname = argv [0] ;

	if ((cptr = strrchr (progname, '/')) != NULL)
		progname = cptr + 1 ;

	if ((cptr = strrchr (progname, '\\')) != NULL)
		progname = cptr + 1 ;

	printf ("\n"
		"  %s\n"
		"\n"
		"  This is a demo program which plays the given file at a slowly \n"
		"  varying speed. Lots of fun with drum loops and full mixes.\n"
		"\n"
		"  It uses Secret Rabbit Code (aka libsamplerate) to perform the \n"
		"  vari-speeding and libsndfile for file I/O.\n"
		"\n", progname) ;

	if (argc == 2)
		filename = argv [1] ;
	else if (argc == 4 && strcmp (argv [1], "-c") == 0)
	{	filename = argv [3] ;
		converter = atoi (argv [2]) ;
		}
	else
	{	printf ("  Usage :\n\n       %s [-c <number>] <input file>\n\n", progname) ;
		puts (
			"  The optional -c argument allows the converter type to be chosen from\n"
			"  the following list :"
			"\n"
			) ;

		for (k = 0 ; (cptr = src_get_name (k)) != NULL ; k++)
			printf ("       %d : %s\n", k, cptr) ;

		puts ("") ;
		exit (1) ;
		} ;

	varispeed_play (filename, converter) ;

	return 0 ;
} /* main */

/*==============================================================================
*/

static void
varispeed_play (const char *filename, int converter)
{	CALLBACK_DATA	*data ;
	AUDIO_OUT		*audio_out ;
	int				error ;

	/* Allocate memory for the callback data. */
	if ((data = calloc (1, sizeof (CALLBACK_DATA))) == NULL)
	{	printf ("\n\n%s:%d Calloc failed!\n", __FILE__, __LINE__) ;
		exit (1) ;
		} ;

	data->magic = MAGIC_NUMBER ;

	if ((data->sndfile = sf_open (filename, SFM_READ, &data->sfinfo)) == NULL)
	{	puts (sf_strerror (NULL)) ;
		exit (1) ;
		} ;

	/* Initialize the sample rate converter. */
	if ((data->src_state = src_new (converter, data->sfinfo.channels, &error)) == NULL)
	{	printf ("\n\nError : src_new() failed : %s.\n\n", src_strerror (error)) ;
		exit (1) ;
		} ;

	printf (

		"  Playing   : %s\n"
		"  Converter : %s\n"
		"\n"
		"  Press <control-c> to exit.\n"
		"\n",
		filename, src_get_name (converter)) ;

	if ((audio_out = audio_open (data->sfinfo.channels, data->sfinfo.samplerate)) == NULL)
	{	printf ("\n\nError : audio_open () failed.\n") ;
		exit (1) ;
		} ;

	/* Set up sample rate converter info. */
	data->src_data.end_of_input = 0 ; /* Set this later. */

	/* Start with zero to force load in while loop. */
	data->src_data.input_frames = 0 ;
	data->src_data.data_in = data->buffer_in ;

	/* Start with output frames also zero. */
	data->src_data.output_frames_gen = 0 ;

	data->buffer_out_start = data->buffer_out_end = 0 ;
	data->src_data.src_ratio = 1.0 ;

	/* Pass the data and the callbacl function to audio_play */
	audio_play ((get_audio_callback_t) varispeed_get_data, audio_out, data) ;

	/* Cleanup */
	audio_close (audio_out) ;
	sf_close (data->sndfile) ;
	src_delete (data->src_state) ;

	free (data) ;

} /* varispeed_play */

/*==============================================================================
*/

static int
varispeed_get_data (CALLBACK_DATA *data, float *samples, int frames)
{	int		error, readframes, frame_count, direct_out ;

	if (data->magic != MAGIC_NUMBER)
	{	printf ("\n\n%s:%d Eeeek, something really bad happened!\n", __FILE__, __LINE__) ;
		exit (1) ;
		} ;

	frame_count = 0 ;

	if (data->buffer_out_start < data->buffer_out_end)
	{	frame_count = MIN (data->buffer_out_end - data->buffer_out_start, frames) ;
		memcpy (samples, data->buffer_out + data->sfinfo.channels * data->buffer_out_start, data->sfinfo.channels * frame_count * sizeof (float)) ;
		data->buffer_out_start += frame_count ;
		} ;

	data->buffer_out_start = data->buffer_out_end = 0 ;

	while (frame_count < frames)
	{
		/* Read INPUT_FRAMES frames worth looping at end of file. */
		for (readframes = 0 ; readframes < INPUT_FRAMES ; )
		{	sf_count_t position ;

			readframes += sf_readf_float (data->sndfile, data->buffer_in + data->sfinfo.channels * readframes, INPUT_FRAMES - readframes) ;

			position = sf_seek (data->sndfile, 0, SEEK_CUR) ;

			if (position < 0 || position == data->sfinfo.frames)
				sf_seek (data->sndfile, 0, SEEK_SET) ;
			} ;

		data->src_data.input_frames = readframes ;

		data->src_data.src_ratio = 1.0 - 0.5 * sin (data->freq_point * 2 * M_PI / 20000) ;
		data->freq_point ++ ;

		direct_out = (data->src_data.src_ratio * readframes < frames - frame_count) ? 1 : 0 ;

		if (direct_out)
		{	data->src_data.data_out = samples + frame_count * data->sfinfo.channels ;
			data->src_data.output_frames = frames - frame_count ;
			}
		else
		{	data->src_data.data_out = data->buffer_out ;
			data->src_data.output_frames = BUFFER_LEN / data->sfinfo.channels ;
			} ;

		if ((error = src_process (data->src_state, &data->src_data)))
		{	printf ("\nError : %s\n\n", src_strerror (error)) ;
			exit (1) ;
			} ;

		if (direct_out)
		{	frame_count += data->src_data.output_frames_gen ;
			continue ;
			} ;

		memcpy (samples + frame_count * data->sfinfo.channels, data->buffer_out, (frames - frame_count) * data->sfinfo.channels * sizeof (float)) ;

		data->buffer_out_start = frames - frame_count ;
		data->buffer_out_end = data->src_data.output_frames_gen ;

		frame_count += frames - frame_count ;
		} ;

	return frame_count ;
} /* varispeed_get_data */

/*==============================================================================
*/

#else /* (HAVE_SNFILE == 0) */

/* Alternative main function when libsndfile is not available. */

int
main (void)
{	puts (
		"\n"
		"****************************************************************\n"
		" This example program was compiled without libsndfile \n"
		" (http://www.zip.com.au/~erikd/libsndfile/).\n"
		" It is therefore completely broken and non-functional.\n"
		"****************************************************************\n"
		"\n"
		) ;

	return 0 ;
} /* main */

#endif

