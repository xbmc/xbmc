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

/*======================================================================
 *  A main program for fribidi.
 *----------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "getopt.h"
#include "fribidi.h"
#ifdef FRIBIDI_NO_CHARSETS
#include <iconv.h>
#endif

#define appname "fribidi"
#define appversion VERSION

extern char *fribidi_version_info;

#define MAX_STR_LEN 65000


#define ALLOCATE(tp,ln) ((tp *) malloc (sizeof (tp) * (ln)))
static void
die (char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);

  if (fmt)
    {
      fprintf (stderr, "%s: ", appname);
      vfprintf (stderr, fmt, ap);
    }
  fprintf (stderr, "Try `%s --help' for more information.\n", appname);
  exit (-1);
}

fribidi_boolean do_break, do_pad, do_mirror, do_reorder_nsm, do_clean,
  show_input, show_changes;
fribidi_boolean show_visual, show_basedir, show_ltov, show_vtol, show_levels;
int text_width;
char *char_set;
char *bol_text, *eol_text;
FriBidiCharType input_base_direction;
#ifdef FRIBIDI_NO_CHARSETS
iconv_t to_ucs4, from_ucs4;
#else
int char_set_num;
#endif

static void
help (void)
{
  /* Break help string into little ones, to assure ISO C89 conformance */
  printf ("Usage: " appname " [OPTION]... [FILE]...\n"
	  "A command line interface for the " FRIBIDI_PACKAGE " library,\n"
	  "Converts a logical string to visual.\n"
	  "\n"
	  "  -h, --help            Display this information and exit\n"
	  "  -V, --version         Display version information and exit\n"
	  "  -v, --verbose         Verbose mode, same as --basedir --ltov --vtol \\\n"
	  "                        --levels --changes\n");
  printf ("  -d, --debug           Output debug information\n"
	  "  -t, --test            Test " FRIBIDI_PACKAGE
	  ", same as --clean --nobreak --showinput \\\n"
	  "                        --reordernsm\n");
#ifdef FRIBIDI_NO_CHARSETS
  printf ("  -c, --charset CS      Specify character set, default is %s \\\n"
	  "                        CS should be a valid iconv character set name\n",
	  char_set);
#else
  printf ("  -c, --charset CS      Specify character set, default is %s\n"
	  "      --charsetdesc CS  Show descriptions for character set CS and exit\n"
	  "      --caprtl          Old style: set character set to CapRTL\n",
	  char_set);
#endif
  printf ("      --showinput       Output the input string too\n"
	  "      --nopad           Do not right justify RTL lines\n"
	  "      --nobreak         Do not break long lines\n"
	  "  -w, --width W         Screen width for padding, default is %d, but if \\\n"
	  "                        enviroment variable COLUMNS is defined, its value \\\n"
	  "                        will be used, --width overrides both of them.\\\n",
	  text_width);
  printf
    ("  -B, --bol BOL         Output string BOL before the visual string\n"
     "  -E, --eol EOL         Output string EOL after the visual string\n"
     "      --rtl             Force base direction to RTL\n"
     "      --ltr             Force base direction to LTR\n"
     "      --wrtl            Set base direction to RTL if no strong character found\n");
  printf
    ("      --wltr            Set base direction to LTR if no strong character found \\\n"
     "                        (default)\n"
     "      --nomirror        Turn mirroring off, to do it later\n"
     "      --reordernsm      Reorder NSM sequences to follow their base character\n"
     "      --clean           Remove explicit format codes in visual string \\\n"
     "                        output, currently does not affect other outputs\n"
     "      --basedir         Output Base Direction\n");
  printf ("      --ltov            Output Logical to Visual position map\n"
	  "      --vtol            Output Visual to Logical position map\n"
	  "      --levels          Output Embedding Levels\n"
	  "      --changes         Output information about changes between \\\n"
	  "                        logical and visual string (start, length)\n"
	  "      --novisual        Do not output the visual string, to be used with \\\n"
	  "                        --basedir, --ltov, --vtol, --levels, --changes\n");
  printf ("  All string indexes are zero based\n" "\n" "Output:\n"
	  "  For each line of input, output something like this:\n"
	  "    [input-str` => '][BOL][[padding space]visual-str][EOL]\n"
	  "    [\\n base-dir][\\n ltov-map][\\n vtol-map][\\n levels][\\n changes]\n");

#ifndef FRIBIDI_NO_CHARSETS
  {
    int i;
    printf ("\n" "Available character sets:\n");
    for (i = 1; i <= FRIBIDI_CHAR_SETS_NUM; i++)
      printf ("  * %-10s: %-25s%1s\n",
	      fribidi_char_set_name (i), fribidi_char_set_title (i),
	      (fribidi_char_set_desc (i) ? "X" : ""));
    printf
      ("  X: Character set has descriptions, use --charsetdesc to see\n");
  }
#endif

  printf
    ("\nReport bugs online at <http://fribidi.org/bug>.\n");
  exit (0);
}

static void
version (void)
{
  printf ("%s", fribidi_version_info);
  exit (0);
}

int
main (int argc, char *argv[])
{
  int exit_val;
  fribidi_boolean file_found;
  char *s;
  FILE *IN;

  text_width = 80;
  do_break = FRIBIDI_TRUE;
  do_pad = FRIBIDI_TRUE;
  do_mirror = FRIBIDI_TRUE;
  do_clean = FRIBIDI_FALSE;
  do_reorder_nsm = FRIBIDI_FALSE;
  show_input = FRIBIDI_FALSE;
  show_visual = FRIBIDI_TRUE;
  show_basedir = FRIBIDI_FALSE;
  show_ltov = FRIBIDI_FALSE;
  show_vtol = FRIBIDI_FALSE;
  show_levels = FRIBIDI_FALSE;
  show_changes = FRIBIDI_FALSE;
  char_set = "UTF-8";
  bol_text = NULL;
  eol_text = NULL;
  input_base_direction = FRIBIDI_TYPE_ON;

  if ((s = getenv ("COLUMNS")))
    {
      int i;

      i = atoi (s);
      if (i > 0)
	text_width = i;
    }

#define CHARSETDESC 257
#define CAPRTL 258

  /* Parse the command line with getopt library */
  /* Must set argv[0], getopt uses it to generate error messages */
  argv[0] = appname;
  while (1)
    {
      int option_index = 0, c;
      static struct option long_options[] = {
	{"help", 0, 0, 'h'},
	{"version", 0, 0, 'V'},
	{"verbose", 0, 0, 'v'},
	{"debug", 0, 0, 'd'},
	{"test", 0, 0, 't'},
	{"charset", 1, 0, 'c'},
#ifndef FRIBIDI_NO_CHARSETS
	{"charsetdesc", 1, 0, CHARSETDESC},
	{"caprtl", 0, 0, CAPRTL},
#endif
	{"showinput", 0, &show_input, FRIBIDI_TRUE},
	{"nopad", 0, &do_pad, FRIBIDI_FALSE},
	{"nobreak", 0, &do_break, FRIBIDI_FALSE},
	{"width", 1, 0, 'w'},
	{"bol", 1, 0, 'B'},
	{"eol", 1, 0, 'E'},
	{"nomirror", 0, &do_mirror, FRIBIDI_FALSE},
	{"reordernsm", 0, &do_reorder_nsm, FRIBIDI_TRUE},
	{"clean", 0, &do_clean, FRIBIDI_TRUE},
	{"ltr", 0, (int *) &input_base_direction, FRIBIDI_TYPE_L},
	{"rtl", 0, (int *) &input_base_direction, FRIBIDI_TYPE_R},
	{"wltr", 0, (int *) &input_base_direction, FRIBIDI_TYPE_WL},
	{"wrtl", 0, (int *) &input_base_direction, FRIBIDI_TYPE_WR},
	{"basedir", 0, &show_basedir, FRIBIDI_TRUE},
	{"ltov", 0, &show_ltov, FRIBIDI_TRUE},
	{"vtol", 0, &show_vtol, FRIBIDI_TRUE},
	{"levels", 0, &show_levels, FRIBIDI_TRUE},
	{"changes", 0, &show_changes, FRIBIDI_TRUE},
	{"novisual", 0, &show_visual, FRIBIDI_FALSE},
	{0, 0, 0, 0}
      };

      c =
	getopt_long (argc, argv, "hVvdtc:w:B:E:", long_options,
		     &option_index);
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
	case 'v':
	  show_basedir = FRIBIDI_TRUE;
	  show_ltov = FRIBIDI_TRUE;
	  show_vtol = FRIBIDI_TRUE;
	  show_levels = FRIBIDI_TRUE;
	  show_changes = FRIBIDI_TRUE;
	  break;
	case 'w':
	  text_width = atoi (optarg);
	  if (text_width <= 0)
	    die ("invalid screen width `%s'\n", optarg);
	  break;
	case 'B':
	  bol_text = optarg;
	  break;
	case 'E':
	  eol_text = optarg;
	  break;
	case 'd':
	  if (!fribidi_set_debug (FRIBIDI_TRUE))
	    die
	      ("%s lib must be compiled with DEBUG option to enable\nturn debug info on.\n",
	       FRIBIDI_PACKAGE);
	  break;
	case 't':
	  do_clean = FRIBIDI_TRUE;
	  show_input = FRIBIDI_TRUE;
	  do_break = FRIBIDI_FALSE;
	  do_reorder_nsm = FRIBIDI_TRUE;
	  break;
	case 'c':
	  char_set = strdup (optarg);
	  break;
#ifndef FRIBIDI_NO_CHARSETS
	case CAPRTL:
	  char_set = "CapRTL";
	  break;
	case CHARSETDESC:
	  char_set = strdup (optarg);
	  char_set_num = fribidi_parse_charset (char_set);
	  if (!char_set_num)
	    die ("unrecognized character set `%s'\n", char_set);
	  if (!fribidi_char_set_desc (char_set_num))
	    die ("no description available for character set `%s'\n",
		 fribidi_char_set_name (char_set_num));
	  else
	    printf ("Descriptions for character set %s:\n"
		    "\n" "%s", fribidi_char_set_title (char_set_num),
		    fribidi_char_set_desc (char_set_num));
	  exit (0);
	  break;
#endif
	case ':':
	case '?':
	  die (NULL);
	  break;
	default:
	  break;
	}
    }

#ifdef FRIBIDI_NO_CHARSETS
  to_ucs4 = iconv_open ("WCHAR_T", char_set);
  from_ucs4 = iconv_open (char_set, "WCHAR_T");
#else
  char_set_num = fribidi_parse_charset (char_set);
#endif

#ifdef FRIBIDI_NO_CHARSETS
  if (to_ucs4 == (iconv_t) (-1) || from_ucs4 == (iconv_t) (-1))
#else
  if (!char_set_num)
#endif
    die ("unrecognized character set `%s'\n", char_set);

  fribidi_set_mirroring (do_mirror);
  fribidi_set_reorder_nsm (do_reorder_nsm);
  exit_val = 0;
  file_found = FRIBIDI_FALSE;
  while (optind < argc || !file_found)
    {
      char *S_;

      S_ = optind < argc ? argv[optind++] : "-";
      file_found = FRIBIDI_TRUE;

      /* Open the infile for reading */
      if (S_[0] == '-' && !S_[1])
	{
	  IN = stdin;
	}
      else
	{
	  IN = fopen (S_, "r");
	  if (!IN)
	    {
	      fprintf (stderr, "%s: %s: no such file or directory\n",
		       appname, S_);
	      exit_val = 1;
	      continue;
	    }
	}

      /* Read and process input one line at a time */
      {
	char S_[MAX_STR_LEN];
	int padding_width, break_width;

	padding_width = show_input ? (text_width - 10) / 2 : text_width;
	break_width = do_break ? padding_width : 3 * MAX_STR_LEN;

	while (fgets (S_, sizeof (S_) - 1, IN))
	  {
	    char *new_line, *nl_found;
	    FriBidiChar logical[MAX_STR_LEN];
	    char outstring[MAX_STR_LEN];
	    FriBidiCharType base;
	    FriBidiStrIndex len;

	    nl_found = "";
	    S_[sizeof (S_) - 1] = 0;
	    len = strlen (S_);
	    /* chop */
	    if (S_[len - 1] == '\n')
	      {
		len--;
		S_[len] = '\0';
		new_line = "\n";
	      }
	    else
	      new_line = "";

#ifdef FRIBIDI_NO_CHARSETS
	    {
	      char *st = S_, *ust = (char *) logical;
	      int in_len = (int) len;
	      len = sizeof logical;
	      iconv (to_ucs4, &st, &in_len, &ust, (int *) &len);
	      len = (FriBidiChar *) ust - logical;
	    }
#else
	    len = fribidi_charset_to_unicode (char_set_num, S_, len, logical);
#endif

	    {
	      FriBidiChar *visual;
	      FriBidiStrIndex *ltov, *vtol;
	      FriBidiLevel *levels;
	      FriBidiStrIndex new_len;
	      fribidi_boolean log2vis;

	      visual = show_visual ? ALLOCATE (FriBidiChar, len + 1) : NULL;
	      ltov = show_ltov ? ALLOCATE (FriBidiStrIndex, len + 1) : NULL;
	      vtol = show_vtol ? ALLOCATE (FriBidiStrIndex, len + 1) : NULL;
	      levels = show_levels ? ALLOCATE (FriBidiLevel, len + 1) : NULL;

	      /* Create a bidi string. */
	      base = input_base_direction;
	      log2vis = fribidi_log2vis (logical, len, &base,
					 /* output */
					 visual, ltov, vtol, levels);
	      if (log2vis)
		{

		  if (show_input)
		    printf ("%-*s => ", padding_width, S_);

		  new_len = len;

		  /* Remove explicit marks, if asked for. */
		  if (do_clean)
		    len =
		      fribidi_remove_bidi_marks (visual, len, ltov, vtol,
						 levels);

		  if (show_visual)
		    {
		      printf (nl_found);

		      if (bol_text)
			printf ("%s", bol_text);

		      /* Convert it to input charset and print. */
		      {
			FriBidiStrIndex idx, st;
			for (idx = 0; idx < len;)
			  {
			    FriBidiStrIndex wid, inlen;

			    wid = break_width;
			    st = idx;
#ifndef FRIBIDI_NO_CHARSETS
			    if (char_set_num != FRIBIDI_CHAR_SET_CAP_RTL)
#endif
			      while (wid > 0 && idx < len)
				wid -= fribidi_wcwidth (visual[idx++]);
#ifndef FRIBIDI_NO_CHARSETS
			    else
			      while (wid > 0 && idx < len)
				{
				  wid--;
				  idx++;
				}
#endif
			    if (wid < 0 && idx > st + 1)
			      idx--;
			    inlen = idx - st;

#ifdef FRIBIDI_NO_CHARSETS
			    {
			      char *str = outstring, *ust =
				(char *) (visual + st);
			      int in_len = inlen * sizeof visual[0];
			      new_len = sizeof outstring;
			      iconv (from_ucs4, &ust, &in_len, &str,
				     (int *) &new_len);
			      *str = '\0';
			      new_len = str - outstring;
			    }
#else
			    new_len =
			      fribidi_unicode_to_charset (char_set_num,
							  visual + st, inlen,
							  outstring);
#endif
			    if (FRIBIDI_IS_RTL (base))
			      printf ("%*s",
				      (int) (do_pad ? (padding_width +
						       strlen (outstring) -
						       (break_width -
							wid)) : 0),
				      outstring);
			    else
			      printf ("%s", outstring);
			    if (idx < len)
			      printf ("\n");
			  }
		      }
		      if (eol_text)
			printf ("%s", eol_text);

		      nl_found = "\n";
		    }
		  if (show_basedir)
		    {
		      printf (nl_found);
		      printf ("Base direction: %s",
			      (FRIBIDI_DIR_TO_LEVEL (base) ? "R" : "L"));
		      nl_found = "\n";
		    }
		  if (show_ltov)
		    {
		      FriBidiStrIndex i;

		      printf (nl_found);
		      for (i = 0; i < len; i++)
			printf ("%ld ", (long) ltov[i]);
		      nl_found = "\n";
		    }
		  if (show_vtol)
		    {
		      FriBidiStrIndex i;

		      printf (nl_found);
		      for (i = 0; i < len; i++)
			printf ("%ld ", (long) vtol[i]);
		      nl_found = "\n";
		    }
		  if (show_levels)
		    {
		      FriBidiStrIndex i;

		      printf (nl_found);
		      for (i = 0; i < len; i++)
			printf ("%d ", (int) levels[i]);
		      nl_found = "\n";
		    }
		  if (show_changes)
		    {
		      FriBidiStrIndex change_start, change_len;
		      fribidi_find_string_changes (logical, len,
						   visual, new_len,
						   &change_start,
						   &change_len);
		      printf ("%sChange start[length] = %d[%d]", nl_found,
			      change_start, change_len);
		      nl_found = "\n";
		    }
		}
	      else
		{
		  exit_val = 2;
		}

	      if (show_visual)
		free (visual);
	      if (show_ltov)
		free (ltov);
	      if (show_vtol)
		free (vtol);
	      if (show_levels)
		free (levels);
	    }

	    if (*nl_found)
	      printf (new_line);
	  }
      }
    }

  return exit_val;
}
