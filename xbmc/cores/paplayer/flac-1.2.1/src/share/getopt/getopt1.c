/*
	NOTE:
	I cannot get the vanilla getopt code to work (i.e. compile only what
	is needed and not duplicate symbols found in the standard library)
	on all the platforms that FLAC supports.  In particular the gating
	of code with the ELIDE_CODE #define is not accurate enough on systems
	that are POSIX but not glibc.  If someone has a patch that works on
	GNU/Linux, Darwin, AND Solaris please submit it on the project page:
		http://sourceforge.net/projects/flac

	In the meantime I have munged the global symbols and removed gates
	around code, while at the same time trying to touch the original as
	little as possible.
*/
/* getopt_long and getopt_long_only entry points for GNU getopt.
   Copyright (C) 1987,88,89,90,91,92,93,94,96,97,98
     Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "share/getopt.h"
/*[JEC] was:#include "getopt.h"*/

#if !defined __STDC__ || !__STDC__
/* This is a separate conditional since some stdc systems
   reject `defined (const)'.  */
#ifndef const
#define const
#endif
#endif

#include <stdio.h>

/* Comment out all this code if we are using the GNU C Library, and are not
   actually compiling the library itself.  This code is part of the GNU C
   Library, but also included in many other GNU distributions.  Compiling
   and linking in this code is a waste when using the GNU C library
   (especially if it is a shared library).  Rather than having every GNU
   program understand `configure --with-gnu-libc' and omit the object files,
   it is simpler to just do this in the source for each such file.  */

#define GETOPT_INTERFACE_VERSION 2
#if !defined _LIBC && defined __GLIBC__ && __GLIBC__ >= 2
#include <gnu-versions.h>
#if _GNU_GETOPT_INTERFACE_VERSION == GETOPT_INTERFACE_VERSION
#define ELIDE_CODE
#endif
#endif

#if 1
/*[JEC] was:#ifndef ELIDE_CODE*/


/* This needs to come after some library #include
   to get __GNU_LIBRARY__ defined.  */
#ifdef __GNU_LIBRARY__
#include <stdlib.h>
#endif

#ifndef	NULL
#define NULL 0
#endif

int
share__getopt_long (argc, argv, options, long_options, opt_index)
     int argc;
     char *const *argv;
     const char *options;
     const struct share__option *long_options;
     int *opt_index;
{
  return share___getopt_internal (argc, argv, options, long_options, opt_index, 0);
}

/* Like share__getopt_long, but '-' as well as '--' can indicate a long option.
   If an option that starts with '-' (not '--') doesn't match a long option,
   but does match a short option, it is parsed as a short option
   instead.  */

int
share__getopt_long_only (argc, argv, options, long_options, opt_index)
     int argc;
     char *const *argv;
     const char *options;
     const struct share__option *long_options;
     int *opt_index;
{
  return share___getopt_internal (argc, argv, options, long_options, opt_index, 1);
}


#endif	/* Not ELIDE_CODE.  */

#ifdef TEST

#include <stdio.h>

int
main (argc, argv)
     int argc;
     char **argv;
{
  int c;
  int digit_optind = 0;

  while (1)
    {
      int this_option_optind = share__optind ? share__optind : 1;
      int option_index = 0;
      static struct share__option long_options[] =
      {
	{"add", 1, 0, 0},
	{"append", 0, 0, 0},
	{"delete", 1, 0, 0},
	{"verbose", 0, 0, 0},
	{"create", 0, 0, 0},
	{"file", 1, 0, 0},
	{0, 0, 0, 0}
      };

      c = share__getopt_long (argc, argv, "abc:d:0123456789",
		       long_options, &option_index);
      if (c == -1)
	break;

      switch (c)
	{
	case 0:
	  printf ("option %s", long_options[option_index].name);
	  if (share__optarg)
	    printf (" with arg %s", share__optarg);
	  printf ("\n");
	  break;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  if (digit_optind != 0 && digit_optind != this_option_optind)
	    printf ("digits occur in two different argv-elements.\n");
	  digit_optind = this_option_optind;
	  printf ("option %c\n", c);
	  break;

	case 'a':
	  printf ("option a\n");
	  break;

	case 'b':
	  printf ("option b\n");
	  break;

	case 'c':
	  printf ("option c with value `%s'\n", share__optarg);
	  break;

	case 'd':
	  printf ("option d with value `%s'\n", share__optarg);
	  break;

	case '?':
	  break;

	default:
	  printf ("?? getopt returned character code 0%o ??\n", c);
	}
    }

  if (share__optind < argc)
    {
      printf ("non-option ARGV-elements: ");
      while (share__optind < argc)
	printf ("%s ", argv[share__optind++]);
      printf ("\n");
    }

  exit (0);
}

#endif /* TEST */
