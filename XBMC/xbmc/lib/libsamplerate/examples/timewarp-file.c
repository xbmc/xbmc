/*
** Copyright (C) 2005-2008 Erik de Castro Lopo <erikd@mega-nerd.com>
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
#include <unistd.h>
#include <string.h>
#include <math.h>

#if (HAVE_SNDFILE)

#include <samplerate.h>
#include <sndfile.h>

#define ARRAY_LEN(x)	((int) (sizeof (x) / sizeof ((x) [0])))

#define DEFAULT_CONVERTER 	SRC_SINC_MEDIUM_QUALITY

#define	BUFFER_LEN			1024
#define	INPUT_STEP_SIZE		8

typedef struct
{	sf_count_t	index ;
	double		ratio ;
} TIMEWARP_FACTOR ;

static void usage_exit (const char *progname) ;
static sf_count_t timewarp_convert (SNDFILE *infile, SNDFILE *outfile, int converter, int channels) ;

int
main (int argc, char *argv [])
{	SNDFILE	*infile, *outfile ;
	SF_INFO sfinfo ;
	sf_count_t	count ;

	if (argc != 3)
		usage_exit (argv [0]) ;

	putchar ('\n') ;
	printf ("Input File    : %s\n", argv [argc - 2]) ;
	if ((infile = sf_open (argv [argc - 2], SFM_READ, &sfinfo)) == NULL)
	{	printf ("Error : Not able to open input file '%s'\n", argv [argc - 2]) ;
		exit (1) ;
		} ;

	if (INPUT_STEP_SIZE * sfinfo.channels > BUFFER_LEN)
	{	printf ("\n\nError : INPUT_STEP_SIZE * sfinfo.channels > BUFFER_LEN\n\n") ;
		exit (1) ;
		} ;


	/* Delete the output file length to zero if already exists. */
	remove (argv [argc - 1]) ;

	if ((outfile = sf_open (argv [argc - 1], SFM_WRITE, &sfinfo)) == NULL)
	{	printf ("Error : Not able to open output file '%s'\n", argv [argc - 1]) ;
		sf_close (infile) ;
		exit (1) ;
		} ;

	sf_command (outfile, SFC_SET_CLIPPING, NULL, SF_TRUE) ;

	printf ("Output file   : %s\n", argv [argc - 1]) ;
	printf ("Converter     : %s\n", src_get_name (DEFAULT_CONVERTER)) ;

	count = timewarp_convert (infile, outfile, DEFAULT_CONVERTER, sfinfo.channels) ;

	printf ("Output Frames : %ld\n\n", (long) count) ;

	sf_close (infile) ;
	sf_close (outfile) ;

	return 0 ;
} /* main */

/*==============================================================================
*/

static TIMEWARP_FACTOR warp [] =
{	{	0		, 1.00000001 },
	{	20000	, 1.01000000 },
	{	20200	, 1.00000001 },
	{	40000	, 1.20000000 },
	{	40300	, 1.00000001 },
	{	60000	, 1.10000000 },
	{	60400	, 1.00000001 },
	{	80000	, 1.50000000 },
	{	81000	, 1.00000001 },
} ;

static sf_count_t
timewarp_convert (SNDFILE *infile, SNDFILE *outfile, int converter, int channels)
{	static float input [BUFFER_LEN] ;
	static float output [BUFFER_LEN] ;

	SRC_STATE	*src_state ;
	SRC_DATA	src_data ;
	int			error, warp_index = 0 ;
	sf_count_t	input_count = 0, output_count = 0 ;

	sf_seek (infile, 0, SEEK_SET) ;
	sf_seek (outfile, 0, SEEK_SET) ;

	/* Initialize the sample rate converter. */
	if ((src_state = src_new (converter, channels, &error)) == NULL)
	{	printf ("\n\nError : src_new() failed : %s.\n\n", src_strerror (error)) ;
		exit (1) ;
		} ;

	src_data.end_of_input = 0 ; /* Set this later. */

	/* Start with zero to force load in while loop. */
	src_data.input_frames = 0 ;
	src_data.data_in = input ;

	if (warp [0].index > 0)
		src_data.src_ratio = 1.0 ;
	else
	{	src_data.src_ratio = warp [0].ratio ;
		warp_index ++ ;
		} ;

	src_data.data_out = output ;
	src_data.output_frames = BUFFER_LEN /channels ;

	while (1)
	{
		if (warp_index < ARRAY_LEN (warp) - 1 && input_count >= warp [warp_index].index)
		{	src_data.src_ratio = warp [warp_index].ratio ;
			warp_index ++ ;
			} ;

		/* If the input buffer is empty, refill it. */
		if (src_data.input_frames == 0)
		{	src_data.input_frames = sf_readf_float (infile, input, INPUT_STEP_SIZE) ;
			input_count += src_data.input_frames ;
			src_data.data_in = input ;

			/* The last read will not be a full buffer, so snd_of_input. */
			if (src_data.input_frames < INPUT_STEP_SIZE)
				src_data.end_of_input = SF_TRUE ;
			} ;

		/* Process current block. */
		if ((error = src_process (src_state, &src_data)))
		{	printf ("\nError : %s\n", src_strerror (error)) ;
			exit (1) ;
			} ;

		/* Terminate if done. */
		if (src_data.end_of_input && src_data.output_frames_gen == 0)
			break ;

		/* Write output. */
		sf_writef_float (outfile, output, src_data.output_frames_gen) ;
		output_count += src_data.output_frames_gen ;

		src_data.data_in += src_data.input_frames_used * channels ;
		src_data.input_frames -= src_data.input_frames_used ;
		} ;

	src_state = src_delete (src_state) ;

	return output_count ;
} /* timewarp_convert */

/*------------------------------------------------------------------------------
*/

static void
usage_exit (const char *progname)
{	const char	*cptr ;

	if ((cptr = strrchr (progname, '/')) != NULL)
		progname = cptr + 1 ;

	if ((cptr = strrchr (progname, '\\')) != NULL)
		progname = cptr + 1 ;

	printf ("\n"
		"  A program demonstrating the time warping capabilities of libsamplerate."
		"  It uses libsndfile for file I/O and Secret Rabbit Code (aka libsamplerate)"
		"  for performing the warping.\n"
		"  It works on any file format supported by libsndfile with any \n"
		"  number of channels (limited only by host memory).\n"
		"\n"
		"  The warping is dependant on a table hard code into the source code.\n"
		"\n"
		"  libsamplerate version : %s\n"
		"\n"
		"  Usage : \n"
		"       %s <input file> <output file>\n"
		"\n", src_get_version (), progname) ;

	puts ("") ;

	exit (1) ;
} /* usage_exit */

/*==============================================================================
*/

#else /* (HAVE_SNFILE == 0) */

/* Alternative main function when libsndfile is not available. */

int
main (void)
{	puts (
		"\n"
		"****************************************************************\n"
		"  This example program was compiled without libsndfile \n"
		"  (http://www.mega-nerd.com/libsndfile/).\n"
		"  It is therefore completely broken and non-functional.\n"
		"****************************************************************\n"
		"\n"
		) ;

	return 0 ;
} /* main */

#endif

