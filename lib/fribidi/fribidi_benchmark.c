/* FriBidi - Library of BiDi algorithm
 * Copyright (C) 1999,2000 Dov Grobgeld, and
 * Copyright (C) 2001,2002 Behdad Esfahbod. 
 * 
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this library, in a file named COPYING; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA  
 * 
 * For licensing issues, contact <dov@imagic.weizmann.ac.il> and 
 * <fwpg@sharif.edu>. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/times.h>
#include "getopt.h"
#include "fribidi.h"

#define appname "fribidi_benchmark"
#define appversion VERSION

extern char *fribidi_version_info;

#define MAX_STR_LEN 1000

static void
die (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);

  fprintf (stderr, "%s: ", appname);
  vfprintf (stderr, fmt, ap);
  fprintf (stderr, "Try `%s --help' for more information.\n", appname);
  exit (-1);
}

#define TEST_STRING \
  "a THE QUICK -123,456 (FOX JUMPS ) DOG the quick !1@7#4&5^ over the dog " \
  "123,456 OVER THE 5%+ 4.0 LAZY"
#define TEST_STRING_EXPLICIT \
  "this is _LJUST_o a _lsimple _Rte%ST_o th_oat  HAS A _LPDF missing" \
  "AnD hOw_L AbOuT, 123,987 tHiS_o a GO_oOD - _L_oTE_oST. " \
  "here_L is_o_o_o _R a good one_o And _r 123,987_LT_oHE_R next_o oNE:" \
  "_R_r and the last _LONE_o IS THE _rbest _lONE and" \
  "a _L_L_L_LL_L_L_L_L_L_L_L_L_Rbug_o_o_o_o_o_o" \
  "_R_r and the last _LONE_o IS THE _rbest _lONE and" \
  "A REAL BIG_l_o BUG! _L _l_r_R_L_laslaj siw_o_Rlkj sslk" \
  "a _L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_L_Rbug" \
  "here_L is_o_o_o _R ab  one_o _r 123,987_LT_oHE_R t_o oNE:" \

int niter;

static void
help (void)
{
  printf
    ("Usage: " appname " [OPTION]...\n"
     "A program for benchmarking the speed of the " FRIBIDI_PACKAGE
     " library.\n" "\n"
     "  -h, --help            Display this information and exit\n"
     "  -V, --version         Display version information and exit\n"
     "  -n, --niter N         Number of iterations. Default is %d.\n"
     "\nReport bugs online at <http://fribidi.sourceforge.net/bugs.php>.\n",
     niter);
  exit (0);
}

static void
version (void)
{
  printf (appname " " appversion "\n%s", fribidi_version_info);
  exit (0);
}

static double
utime (void)
{
  struct tms tb;
  times (&tb);
  return 0.01 * tb.tms_utime;
}

static void
benchmark (char *S_, int niter)
{
  int len, i;
  FriBidiChar us[MAX_STR_LEN], out_us[MAX_STR_LEN];
  FriBidiStrIndex positionLtoV[MAX_STR_LEN], positionVtoL[MAX_STR_LEN];
  FriBidiLevel embedding_list[MAX_STR_LEN];
  FriBidiCharType base;
  double time0, time1;

  {
    int j;
    len = strlen (S_);
    for (i = 0, j = 0; i < len; i++)
      {
	if (S_[i] == '_')
	  switch (S_[++i])
	    {
	    case '>':
	      us[j++] = UNI_LRM;
	      break;
	    case '<':
	      us[j++] = UNI_RLM;
	      break;
	    case 'l':
	      us[j++] = UNI_LRE;
	      break;
	    case 'r':
	      us[j++] = UNI_RLE;
	      break;
	    case 'L':
	      us[j++] = UNI_LRO;
	      break;
	    case 'R':
	      us[j++] = UNI_RLO;
	      break;
	    case 'o':
	      us[j++] = UNI_PDF;
	      break;
	    case '_':
	      us[j++] = '_';
	      break;
	    default:
	      us[j++] = '_';
	      i--;
	      break;
	    }
	else
	  us[j++] = S_[i];
	if (us[j] >= 'A' && us[j] <= 'F')
	  us[j] += UNI_ARABIC_ALEF - 'A';
	else if (us[j] >= 'G' && us[j] <= 'Z')
	  us[j] += UNI_HEBREW_ALEF - 'G';
	else if (us[j] >= '6' && us[j] <= '9')
	  us[j] += UNI_ARABIC_ZERO - '0';
      }
    len = j;
  }

  /* Start timer */
  time0 = utime ();

  for (i = 0; i < niter; i++)
    {
      /* Create a bidi string */
      base = FRIBIDI_TYPE_ON;
      fribidi_log2vis (us, len, &base,
		       /* output */
		       out_us, positionVtoL, positionLtoV, embedding_list);
    }

  /* stop timer */
  time1 = utime ();

  /* output result */
  printf ("Length = %d\n", len);
  printf ("Iterations = %d\n", niter);
  printf ("%d len*iterations in %f seconds\n", len * niter, time1 - time0);
  printf ("= %.0f kilo.length.iterations/second\n",
	  1.0 * len * niter / 1000 / (time1 - time0));

  return;
}

int
main (int argc, char *argv[])
{
  niter = 2000;

  /* Parse the command line */
  argv[0] = appname;
  while (1)
    {
      int option_index = 0, c;
      static struct option long_options[] = {
	{"help", 0, 0, 'h'},
	{"version", 0, 0, 'V'},
	{"niter", 0, 0, 'n'},
	{0, 0, 0, 0}
      };

      c = getopt_long (argc, argv, "hVn:", long_options, &option_index);
      if (c == -1)
	break;

      switch (c)
	{
	case 0:
	  break;
	case 'h':
	  help ();
	  break;
	case 'V':
	  version ();
	  break;
	case 'n':
	  niter = atoi (optarg);
	  if (niter <= 0)
	    die ("invalid number of iterations `%s'\n", optarg);
	  break;
	case ':':
	case '?':
	  die (NULL);
	  break;
	default:
	  break;
	}
    }

  printf ("* Without explicit marks:\n");
  benchmark (TEST_STRING, niter);
  printf ("\n");
  printf ("* With explicit marks:\n");
  benchmark (TEST_STRING_EXPLICIT, niter);

  return 0;
}
