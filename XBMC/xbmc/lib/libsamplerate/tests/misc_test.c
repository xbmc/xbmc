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

#include <samplerate.h>

#include "util.h"

static void name_test (void) ;
static void error_test (void) ;
static void src_ratio_test (void) ;

int
main (void)
{
	puts ("") ;

	printf ("    version : %s\n\n", src_get_version ()) ;

	/* Current max converter is SRC_LINEAR. */
	name_test () ;

	error_test () ;

	src_ratio_test () ;

	return 0 ;
} /* main */

static void
name_test (void)
{	const char	*name ;
	int	k = 0 ;

	puts ("    name_test :") ;

	while (1)
	{	name = src_get_name (k) ;
		if (name == NULL)
			break ;
		printf ("\tName %d : %s\n", k, name) ;
		printf ("\tDesc %d : %s\n", k, src_get_description (k)) ;
		k ++ ;
		} ;

	puts ("") ;

	return ;
} /* name_test */

/*------------------------------------------------------------------------------
*/

typedef struct
{	double	ratio ;
	int		should_pass ;
} RATIO_TEST ;

static RATIO_TEST ratio_test [] =
{	{	1.0 / 256.1,	0 },
	{	1.0 / 256.0,	1 },
	{	1.0,			1 },
	{	256.0, 			1 },
	{	256.1,			0 },
	{	-1.0,			0 }
} ;

static void
src_ratio_test (void)
{	int k ;

	puts ("    src_ratio_test (SRC ratio must be in range [1/256, 256]):" ) ;


	for (k = 0 ; k < ARRAY_LEN (ratio_test) ; k++)
	{	if (ratio_test [k].should_pass && src_is_valid_ratio (ratio_test [k].ratio) == 0)
		{	printf ("\n\nLine %d : SRC ratio %f should have passed.\n\n", __LINE__, ratio_test [k].ratio) ;
			exit (1) ;
			} ;
		if (! ratio_test [k].should_pass && src_is_valid_ratio (ratio_test [k].ratio) != 0)
		{	printf ("\n\nLine %d : SRC ratio %f should not have passed.\n\n", __LINE__, ratio_test [k].ratio) ;
			exit (1) ;
			} ;
		printf ("\t SRC ratio (%9.5f) : %s ................... ok\n", ratio_test [k].ratio,
			(ratio_test [k].should_pass ? "pass" : "fail")) ;
		} ;

	puts ("") ;

	return ;
} /* src_ratio_test */

static void
error_test (void)
{	const char *errorstr ;
	int		k, errors = 0 ;

	puts ("    error_test :") ;

	for (k = 0 ; 1 ; k++)
	{	errorstr = src_strerror (k) ;
		printf ("\t%-2d : %s\n", k, errorstr) ;
		if (errorstr == NULL)
		{	errors ++ ;
			continue ;
			} ;
		if (strstr (errorstr, "Placeholder.") == errorstr)
			break ;
		} ;

	if (errors != 0)
	{	printf ("\n\nLine %d : Missing error numbers above.\n\n", __LINE__) ;
		exit (1) ;
		} ;

	puts ("") ;

	return ;
} /* error_test */
