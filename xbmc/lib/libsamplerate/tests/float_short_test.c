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

#include <samplerate.h>

#include "util.h"

#define	BUFFER_LEN		10000

static void float_to_short_test (void) ;
static void short_to_float_test (void) ;

static void float_to_int_test (void) ;
static void int_to_float_test (void) ;

int
main (void)
{
	puts ("") ;

	float_to_short_test () ;
	short_to_float_test () ;

	float_to_int_test () ;
	int_to_float_test () ;

	puts ("") ;

	return 0 ;
} /* main */

/*=====================================================================================
*/

static void
float_to_short_test (void)
{
	static float fpos [] =
	{	0.95, 0.99, 1.0, 1.01, 1.1, 2.0, 11.1, 111.1, 2222.2, 33333.3
		} ;
	static float fneg [] =
	{	-0.95, -0.99, -1.0, -1.01, -1.1, -2.0, -11.1, -111.1, -2222.2, -33333.3
		} ;

	static short out [MAX (ARRAY_LEN (fpos), ARRAY_LEN (fneg))] ;

	int k ;

	printf ("\tfloat_to_short_test ............................. ") ;

	src_float_to_short_array (fpos, out, ARRAY_LEN (fpos)) ;

	for (k = 0 ; k < ARRAY_LEN (fpos) ; k++)
		if (out [k] < 30000)
		{	printf ("\n\n\tLine %d : out [%d] == %d\n", __LINE__, k, out [k]) ;
			exit (1) ;
			} ;

	src_float_to_short_array (fneg, out, ARRAY_LEN (fneg)) ;

	for (k = 0 ; k < ARRAY_LEN (fneg) ; k++)
		if (out [k] > -30000)
		{	printf ("\n\n\tLine %d : out [%d] == %d\n", __LINE__, k, out [k]) ;
			exit (1) ;
			} ;

	puts ("ok") ;

	return ;
} /* float_to_short_test */

/*-------------------------------------------------------------------------------------
*/

static void
short_to_float_test (void)
{
	static short input	[BUFFER_LEN] ;
	static short output	[BUFFER_LEN] ;
	static float temp	[BUFFER_LEN] ;

	int k ;

	printf ("\tshort_to_float_test ............................. ") ;

	for (k = 0 ; k < ARRAY_LEN (input) ; k++)
		input [k] = (k * 0x8000) / ARRAY_LEN (input) ;

	src_short_to_float_array (input, temp, ARRAY_LEN (temp)) ;
	src_float_to_short_array (temp, output, ARRAY_LEN (output)) ;

	for (k = 0 ; k < ARRAY_LEN (input) ; k++)
		if (ABS (input [k] - output [k]) > 0)
		{	printf ("\n\n\tLine %d : index %d   %d -> %d\n", __LINE__, k, input [k], output [k]) ;
			exit (1) ;
			} ;

	puts ("ok") ;

	return ;
} /* short_to_float_test */

/*=====================================================================================
*/

static void
float_to_int_test (void)
{
	static float fpos [] =
	{	0.95, 0.99, 1.0, 1.01, 1.1, 2.0, 11.1, 111.1, 2222.2, 33333.3
		} ;
	static float fneg [] =
	{	-0.95, -0.99, -1.0, -1.01, -1.1, -2.0, -11.1, -111.1, -2222.2, -33333.3
		} ;

	static int out [MAX (ARRAY_LEN (fpos), ARRAY_LEN (fneg))] ;

	int k ;

	printf ("\tfloat_to_int_test ............................... ") ;

	src_float_to_int_array (fpos, out, ARRAY_LEN (fpos)) ;

	for (k = 0 ; k < ARRAY_LEN (fpos) ; k++)
		if (out [k] < 30000 * 0x10000)
		{	printf ("\n\n\tLine %d : out [%d] == %d\n", __LINE__, k, out [k]) ;
			exit (1) ;
			} ;

	src_float_to_int_array (fneg, out, ARRAY_LEN (fneg)) ;

	for (k = 0 ; k < ARRAY_LEN (fneg) ; k++)
		if (out [k] > -30000 * 0x1000)
		{	printf ("\n\n\tLine %d : out [%d] == %d\n", __LINE__, k, out [k]) ;
			exit (1) ;
			} ;

	puts ("ok") ;

	return ;
} /* float_to_int_test */

/*-------------------------------------------------------------------------------------
*/

static void
int_to_float_test (void)
{
	static int input	[BUFFER_LEN] ;
	static int output	[BUFFER_LEN] ;
	static float temp	[BUFFER_LEN] ;

	int k ;

	printf ("\tint_to_float_test ............................... ") ;

	for (k = 0 ; k < ARRAY_LEN (input) ; k++)
		input [k] = (k * 0x80000000) / ARRAY_LEN (input) ;

	src_int_to_float_array (input, temp, ARRAY_LEN (temp)) ;
	src_float_to_int_array (temp, output, ARRAY_LEN (output)) ;

	for (k = 0 ; k < ARRAY_LEN (input) ; k++)
		if (ABS (input [k] - output [k]) > 0)
		{	printf ("\n\n\tLine %d : index %d   %d -> %d\n", __LINE__, k, input [k], output [k]) ;
			exit (1) ;
			} ;

	puts ("ok") ;

	return ;
} /* int_to_float_test */

