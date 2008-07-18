/* Getopt for GNU.
   NOTE: getopt is now part of the C library, so if you don't know what
   "Keep this file name-space clean" means, talk to roland@gnu.ai.mit.edu
   before changing it!

   Copyright (C) 1987, 88, 89, 90, 91, 92, 1993
    Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef __STDC__
#  ifndef const
#    define const
#  endif
#endif

/* This tells Alpha OSF/1 not to define a getopt prototype in <stdio.h>.  */
#ifndef _NO_PROTO
#define _NO_PROTO
#endif

#include <stdio.h>

/* Comment out all this code if we are using the GNU C Library, and are not
   actually compiling the library itself.  This code is part of the GNU C
   Library, but also included in many other GNU distributions.  Compiling
   and linking in this code is a waste when using the GNU C library
   (especially if it is a shared library).  Rather than having every GNU
   program understand `configure --with-gnu-libc' and omit the object files,
   it is simpler to just do this in the source for each such file.  */

#if defined (_LIBC) || !defined (__GNU_LIBRARY__) || !__MacOSX__


/* This needs to come after some library #include
   to get __GNU_LIBRARY__ defined.  */
#ifdef  __GNU_LIBRARY__
/* Don't include stdlib.h for non-GNU C libraries because some of them
   contain conflicting prototypes for getopt.  */
#include <stdlib.h>
#endif  /* GNU C library.  */

/* If GETOPT_COMPAT is defined, `+' as well as `--' can introduce a
   long-named option.  Because this is not POSIX.2 compliant, it is
   being phased out.  */
/* #define GETOPT_COMPAT */

/* This version of `getopt' appears to the caller like standard Unix `getopt'
   but it behaves differently for the user, since it allows the user
   to intersperse the options with the other arguments.

   As `getopt' works, it permutes the elements of ARGV so that,
   when it is done, all the options precede everything else.  Thus
   all application programs are extended to handle flexible argument order.

   Setting the environment variable POSIXLY_CORRECT disables permutation.
   Then the behavior is completely standard.

   GNU application programs can use a third alternative mode in which
   they can distinguish the relative order of options and other arguments.  */

#include "getopt.h"

/* For communication from `getopt' to the caller.
   When `getopt' finds an option that takes an argument,
   the argument value is returned here.
   Also, when `ordering' is RETURN_IN_ORDER,
   each non-option ARGV-element is returned here.  */

char *optarg = 0;

/* Index in ARGV of the next element to be scanned.
   This is used for communication to and from the caller
   and for communication between successive calls to `getopt'.

   On entry to `getopt', zero means this is the first call; initialize.

   When `getopt' returns EOF, this is the index of the first of the
   non-option elements that the caller should itself scan.

   Otherwise, `optind' communicates from one call to the next
   how much of ARGV has been scanned so far.  */

/* XXX 1003.2 says this must be 1 before any call.  */
int optind = 0;

/* The next char to be scanned in the option-element
   in which the last option character we returned was found.
   This allows us to pick up the scan where we left off.

   If this is zero, or a null string, it means resume the scan
   by advancing to the next ARGV-element.  */

static char *nextchar;

/* Callers store zero here to inhibit the error message
   for unrecognized options.  */

int opterr = 1;

/* Set to an option character which was unrecognized.
   This must be initialized on some systems to avoid linking in the
   system's own getopt implementation.  */

#define BAD_OPTION '\0'
int optopt = BAD_OPTION;

/* Describe how to deal with options that follow non-option ARGV-elements.

   If the caller did not specify anything,
   the default is REQUIRE_ORDER if the environment variable
   POSIXLY_CORRECT is defined, PERMUTE otherwise.

   REQUIRE_ORDER means don't recognize them as options;
   stop option processing when the first non-option is seen.
   This is what Unix does.
   This mode of operation is selected by either setting the environment
   variable POSIXLY_CORRECT, or using `+' as the first character
   of the list of option characters.

   PERMUTE is the default.  We permute the contents of ARGV as we scan,
   so that eventually all the non-options are at the end.  This allows options
   to be given in any order, even with programs that were not written to
   expect this.

   RETURN_IN_ORDER is an option available to programs that were written
   to expect options and other ARGV-elements in any order and that care about
   the ordering of the two.  We describe each non-option ARGV-element
   as if it were the argument of an option with character code 1.
   Using `-' as the first character of the list of option characters
   selects this mode of operation.

   The special argument `--' forces an end of option-scanning regardless
   of the value of `ordering'.  In the case of RETURN_IN_ORDER, only
   `--' can cause `getopt' to return EOF with `optind' != ARGC.  */

static enum
{
  REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
} ordering;

#ifdef  __GNU_LIBRARY__
/* We want to avoid inclusion of string.h with non-GNU libraries
   because there are many ways it can cause trouble.
   On some systems, it contains special magic macros that don't work
   in GCC.  */
#include <string.h>
#define my_index    strchr
#define my_strlen   strlen
#else

/* Avoid depending on library functions or files
   whose names are inconsistent.  */

#if __STDC__ || defined(PROTO)
extern char *getenv(const char *name);
extern int  strcmp (const char *s1, const char *s2);
extern int  strncmp(const char *s1, const char *s2, unsigned int n);

static int my_strlen(const char *s);
static char *my_index (const char *str, int chr);
#else
extern char *getenv ();
#endif

static int
my_strlen (str)
     const char *str;
{
  int n = 0;
  while (*str++)
    n++;
  return n;
}

static char *
my_index (str, chr)
     const char *str;
     int chr;
{
  while (*str)
    {
      if (*str == chr)
    return (char *) str;
      str++;
    }
  return 0;
}

#endif              /* GNU C library.  */

/* Handle permutation of arguments.  */

/* Describe the part of ARGV that contains non-options that have
   been skipped.  `first_nonopt' is the index in ARGV of the first of them;
   `last_nonopt' is the index after the last of them.  */

static int first_nonopt;
static int last_nonopt;

/* Exchange two adjacent subsequences of ARGV.
   One subsequence is elements [first_nonopt,last_nonopt)
   which contains all the non-options that have been skipped so far.
   The other is elements [last_nonopt,optind), which contains all
   the options processed since those non-options were skipped.

   `first_nonopt' and `last_nonopt' are relocated so that they describe
   the new indices of the non-options in ARGV after they are moved.

   To perform the swap, we first reverse the order of all elements. So
   all options now come before all non options, but they are in the
   wrong order. So we put back the options and non options in original
   order by reversing them again. For example:
       original input:      a b c -x -y
       reverse all:         -y -x c b a
       reverse options:     -x -y c b a
       reverse non options: -x -y a b c
*/

#if __STDC__ || defined(PROTO)
static void exchange (char **argv);
#endif

static void
exchange (argv)
     char **argv;
{
  char *temp, **first, **last;

  /* Reverse all the elements [first_nonopt, optind) */
  first = &argv[first_nonopt];
  last  = &argv[optind-1];
  while (first < last) {
    temp = *first; *first = *last; *last = temp; first++; last--;
  }
  /* Put back the options in order */
  first = &argv[first_nonopt];
  first_nonopt += (optind - last_nonopt);
  last  = &argv[first_nonopt - 1];
  while (first < last) {
    temp = *first; *first = *last; *last = temp; first++; last--;
  }

  /* Put back the non options in order */
  first = &argv[first_nonopt];
  last_nonopt = optind;
  last  = &argv[last_nonopt-1];
  while (first < last) {
    temp = *first; *first = *last; *last = temp; first++; last--;
  }
}

/* Scan elements of ARGV (whose length is ARGC) for option characters
   given in OPTSTRING.

   If an element of ARGV starts with '-', and is not exactly "-" or "--",
   then it is an option element.  The characters of this element
   (aside from the initial '-') are option characters.  If `getopt'
   is called repeatedly, it returns successively each of the option characters
   from each of the option elements.

   If `getopt' finds another option character, it returns that character,
   updating `optind' and `nextchar' so that the next call to `getopt' can
   resume the scan with the following option character or ARGV-element.

   If there are no more option characters, `getopt' returns `EOF'.
   Then `optind' is the index in ARGV of the first ARGV-element
   that is not an option.  (The ARGV-elements have been permuted
   so that those that are not options now come last.)

   OPTSTRING is a string containing the legitimate option characters.
   If an option character is seen that is not listed in OPTSTRING,
   return BAD_OPTION after printing an error message.  If you set `opterr' to
   zero, the error message is suppressed but we still return BAD_OPTION.

   If a char in OPTSTRING is followed by a colon, that means it wants an arg,
   so the following text in the same ARGV-element, or the text of the following
   ARGV-element, is returned in `optarg'.  Two colons mean an option that
   wants an optional arg; if there is text in the current ARGV-element,
   it is returned in `optarg', otherwise `optarg' is set to zero.

   If OPTSTRING starts with `-' or `+', it requests different methods of
   handling the non-option ARGV-elements.
   See the comments about RETURN_IN_ORDER and REQUIRE_ORDER, above.

   Long-named options begin with `--' instead of `-'.
   Their names may be abbreviated as long as the abbreviation is unique
   or is an exact match for some defined option.  If they have an
   argument, it follows the option name in the same ARGV-element, separated
   from the option name by a `=', or else the in next ARGV-element.
   When `getopt' finds a long-named option, it returns 0 if that option's
   `flag' field is nonzero, the value of the option's `val' field
   if the `flag' field is zero.

   The elements of ARGV aren't really const, because we permute them.
   But we pretend they're const in the prototype to be compatible
   with other systems.

   LONGOPTS is a vector of `struct option' terminated by an
   element containing a name which is zero.

   LONGIND returns the index in LONGOPT of the long-named option found.
   It is only valid when a long-named option has been found by the most
   recent call.

   If LONG_ONLY is nonzero, '-' as well as '--' can introduce
   long-named options.  */

int
_getopt_internal (argc, argv, optstring, longopts, longind, long_only)
     int argc;
     char *const *argv;
     const char *optstring;
     const struct option *longopts;
     int *longind;
     int long_only;
{
  int option_index;

  optarg = 0;

  /* Initialize the internal data when the first call is made.
     Start processing options with ARGV-element 1 (since ARGV-element 0
     is the program name); the sequence of previously skipped
     non-option ARGV-elements is empty.  */

  if (optind == 0)
    {
      first_nonopt = last_nonopt = optind = 1;

      nextchar = NULL;

      /* Determine how to handle the ordering of options and nonoptions.  */

      if (optstring[0] == '-')
    {
      ordering = RETURN_IN_ORDER;
      ++optstring;
    }
      else if (optstring[0] == '+')
    {
      ordering = REQUIRE_ORDER;
      ++optstring;
    }
      else if (getenv ("POSIXLY_CORRECT") != NULL)
    ordering = REQUIRE_ORDER;
      else
    ordering = PERMUTE;
    }

  if (nextchar == NULL || *nextchar == '\0')
    {
      if (ordering == PERMUTE)
    {
      /* If we have just processed some options following some non-options,
         exchange them so that the options come first.  */

      if (first_nonopt != last_nonopt && last_nonopt != optind)
        exchange ((char **) argv);
      else if (last_nonopt != optind)
        first_nonopt = optind;

      /* Now skip any additional non-options
         and extend the range of non-options previously skipped.  */

      while (optind < argc
         && (argv[optind][0] != '-' || argv[optind][1] == '\0')
#ifdef GETOPT_COMPAT
         && (longopts == NULL
             || argv[optind][0] != '+' || argv[optind][1] == '\0')
#endif              /* GETOPT_COMPAT */
         )
        optind++;
      last_nonopt = optind;
    }

      /* Special ARGV-element `--' means premature end of options.
     Skip it like a null option,
     then exchange with previous non-options as if it were an option,
     then skip everything else like a non-option.  */

      if (optind != argc && !strcmp (argv[optind], "--"))
    {
      optind++;

      if (first_nonopt != last_nonopt && last_nonopt != optind)
        exchange ((char **) argv);
      else if (first_nonopt == last_nonopt)
        first_nonopt = optind;
      last_nonopt = argc;

      optind = argc;
    }

      /* If we have done all the ARGV-elements, stop the scan
     and back over any non-options that we skipped and permuted.  */

      if (optind == argc)
    {
      /* Set the next-arg-index to point at the non-options
         that we previously skipped, so the caller will digest them.  */
      if (first_nonopt != last_nonopt)
        optind = first_nonopt;
      return EOF;
    }

      /* If we have come to a non-option and did not permute it,
     either stop the scan or describe it to the caller and pass it by.  */

      if ((argv[optind][0] != '-' || argv[optind][1] == '\0')
#ifdef GETOPT_COMPAT
      && (longopts == NULL
          || argv[optind][0] != '+' || argv[optind][1] == '\0')
#endif              /* GETOPT_COMPAT */
      )
    {
      if (ordering == REQUIRE_ORDER)
        return EOF;
      optarg = argv[optind++];
      return 1;
    }

      /* We have found another option-ARGV-element.
     Start decoding its characters.  */

      nextchar = (argv[optind] + 1
          + (longopts != NULL && argv[optind][1] == '-'));
    }

  if (longopts != NULL
      && ((argv[optind][0] == '-'
       && (argv[optind][1] == '-' || long_only))
#ifdef GETOPT_COMPAT
      || argv[optind][0] == '+'
#endif              /* GETOPT_COMPAT */
      ))
    {
      const struct option *p;
      char *s = nextchar;
      int exact = 0;
      int ambig = 0;
      const struct option *pfound = NULL;
      int indfound = 0;

      while (*s && *s != '=')
    s++;

      /* Test all options for either exact match or abbreviated matches.  */
      for (p = longopts, option_index = 0; p->name;
       p++, option_index++)
    if (!strncmp (p->name, nextchar, s - nextchar))
      {
        if (s - nextchar == my_strlen (p->name))
          {
        /* Exact match found.  */
        pfound = p;
        indfound = option_index;
        exact = 1;
        break;
          }
        else if (pfound == NULL)
          {
        /* First nonexact match found.  */
        pfound = p;
        indfound = option_index;
          }
        else
          /* Second nonexact match found.  */
          ambig = 1;
      }

      if (ambig && !exact)
    {
      if (opterr)
        fprintf (stderr, "%s: option `%s' is ambiguous\n",
             argv[0], argv[optind]);
      nextchar += my_strlen (nextchar);
      optind++;
      return BAD_OPTION;
    }

      if (pfound != NULL)
    {
      option_index = indfound;
      optind++;
      if (*s)
        {
          /* Don't test has_arg with >, because some C compilers don't
         allow it to be used on enums.  */
          if (pfound->has_arg)
        optarg = s + 1;
          else
        {
          if (opterr)
            {
              if (argv[optind - 1][1] == '-')
            /* --option */
            fprintf (stderr,
                 "%s: option `--%s' doesn't allow an argument\n",
                 argv[0], pfound->name);
              else
            /* +option or -option */
            fprintf (stderr,
                 "%s: option `%c%s' doesn't allow an argument\n",
                 argv[0], argv[optind - 1][0], pfound->name);
            }
          nextchar += my_strlen (nextchar);
          return BAD_OPTION;
        }
        }
      else if (pfound->has_arg == 1)
        {
          if (optind < argc)
        optarg = argv[optind++];
          else
        {
          if (opterr)
            fprintf (stderr, "%s: option `%s' requires an argument\n",
                 argv[0], argv[optind - 1]);
          nextchar += my_strlen (nextchar);
          return optstring[0] == ':' ? ':' : BAD_OPTION;
        }
        }
      nextchar += my_strlen (nextchar);
      if (longind != NULL)
        *longind = option_index;
      if (pfound->flag)
        {
          *(pfound->flag) = pfound->val;
          return 0;
        }
      return pfound->val;
    }
      /* Can't find it as a long option.  If this is not getopt_long_only,
     or the option starts with '--' or is not a valid short
     option, then it's an error.
     Otherwise interpret it as a short option.  */
      if (!long_only || argv[optind][1] == '-'
#ifdef GETOPT_COMPAT
      || argv[optind][0] == '+'
#endif              /* GETOPT_COMPAT */
      || my_index (optstring, *nextchar) == NULL)
    {
      if (opterr)
        {
          if (argv[optind][1] == '-')
        /* --option */
        fprintf (stderr, "%s: unrecognized option `--%s'\n",
             argv[0], nextchar);
          else
        /* +option or -option */
        fprintf (stderr, "%s: unrecognized option `%c%s'\n",
             argv[0], argv[optind][0], nextchar);
        }
      nextchar = (char *) "";
      optind++;
      return BAD_OPTION;
    }
    }

  /* Look at and handle the next option-character.  */

  {
    char c = *nextchar++;
    char *temp = my_index (optstring, c);

    /* Increment `optind' when we start to process its last character.  */
    if (*nextchar == '\0')
      ++optind;

    if (temp == NULL || c == ':')
      {
    if (opterr)
      {
#if 0
        if (c < 040 || c >= 0177)
          fprintf (stderr, "%s: unrecognized option, character code 0%o\n",
               argv[0], c);
        else
          fprintf (stderr, "%s: unrecognized option `-%c'\n", argv[0], c);
#else
        /* 1003.2 specifies the format of this message.  */
        fprintf (stderr, "%s: illegal option -- %c\n", argv[0], c);
#endif
      }
    optopt = c;
    return BAD_OPTION;
      }
    if (temp[1] == ':')
      {
    if (temp[2] == ':')
      {
        /* This is an option that accepts an argument optionally.  */
        if (*nextchar != '\0')
          {
        optarg = nextchar;
        optind++;
          }
        else
          optarg = 0;
        nextchar = NULL;
      }
    else
      {
        /* This is an option that requires an argument.  */
        if (*nextchar != '\0')
          {
        optarg = nextchar;
        /* If we end this ARGV-element by taking the rest as an arg,
           we must advance to the next element now.  */
        optind++;
          }
        else if (optind == argc)
          {
        if (opterr)
          {
#if 0
            fprintf (stderr, "%s: option `-%c' requires an argument\n",
                 argv[0], c);
#else
            /* 1003.2 specifies the format of this message.  */
            fprintf (stderr, "%s: option requires an argument -- %c\n",
                 argv[0], c);
#endif
          }
        optopt = c;
        if (optstring[0] == ':')
          c = ':';
        else
          c = BAD_OPTION;
          }
        else
          /* We already incremented `optind' once;
         increment it again when taking next ARGV-elt as argument.  */
          optarg = argv[optind++];
        nextchar = NULL;
      }
      }
    return c;
  }
}

int
getopt (argc, argv, optstring)
     int argc;
     char *const *argv;
     const char *optstring;
{
  return _getopt_internal (argc, argv, optstring,
               (const struct option *) 0,
               (int *) 0,
               0);
}

int
getopt_long (argc, argv, options, long_options, opt_index)
     int argc;
     char *const *argv;
     const char *options;
     const struct option *long_options;
     int *opt_index;
{
  return _getopt_internal (argc, argv, options, long_options, opt_index, 0);
}

#endif  /* _LIBC or not __GNU_LIBRARY__.  */

#ifdef TEST

/* Compile with -DTEST to make an executable for use in testing
   the above definition of `getopt'.  */

int
main (argc, argv)
     int argc;
     char **argv;
{
  int c;
  int digit_optind = 0;

  while (1)
    {
      int this_option_optind = optind ? optind : 1;

      c = getopt (argc, argv, "abc:d:0123456789");
      if (c == EOF)
    break;

      switch (c)
    {
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
      printf ("option c with value `%s'\n", optarg);
      break;

    case BAD_OPTION:
      break;

    default:
      printf ("?? getopt returned character code 0%o ??\n", c);
    }
    }

  if (optind < argc)
    {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
    printf ("%s ", argv[optind++]);
      printf ("\n");
    }

  exit (0);
}

#endif /* TEST */
