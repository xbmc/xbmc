/* 
   Unix SMB/CIFS implementation.
   Name mapping code 
   Copyright (C) Jeremy Allison 1998
   Copyright (C) Andrew Tridgell 2002
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "includes.h"


/* ************************************************************************** **
 * Used only in do_fwd_mangled_map(), below.
 * ************************************************************************** **
 */
static char *map_filename( char *s,         /* This is null terminated */
                           const char *pattern,   /* This isn't. */
                           int len )        /* This is the length of pattern. */
  {
  static pstring matching_bit;  /* The bit of the string which matches */
                                /* a * in pattern if indeed there is a * */
  char *sp;                     /* Pointer into s. */
  char *pp;                     /* Pointer into p. */
  char *match_start;            /* Where the matching bit starts. */
  pstring pat;

  StrnCpy( pat, pattern, len ); /* Get pattern into a proper string! */
  pstrcpy( matching_bit, "" );  /* Match but no star gets this. */
  pp = pat;                     /* Initialize the pointers. */
  sp = s;

  if( strequal(s, ".") || strequal(s, ".."))
    {
    return NULL;                /* Do not map '.' and '..' */
    }

  if( (len == 1) && (*pattern == '*') )
    {
    return NULL;                /* Impossible, too ambiguous for */
    }                           /* words! */

  while( (*sp)                  /* Not the end of the string. */
      && (*pp)                  /* Not the end of the pattern. */
      && (*sp == *pp)           /* The two match. */
      && (*pp != '*') )         /* No wildcard. */
    {
    sp++;                       /* Keep looking. */
    pp++;
    }

  if( !*sp && !*pp )            /* End of pattern. */
    return( matching_bit );     /* Simple match.  Return empty string. */

  if( *pp == '*' )
    {
    pp++;                       /* Always interrested in the chacter */
                                /* after the '*' */
    if( !*pp )                  /* It is at the end of the pattern. */
      {
      StrnCpy( matching_bit, s, sp-s );
      return( matching_bit );
      }
    else
      {
      /* The next character in pattern must match a character further */
      /* along s than sp so look for that character. */
      match_start = sp;
      while( (*sp)              /* Not the end of s. */
          && (*sp != *pp) )     /* Not the same  */
        sp++;                   /* Keep looking. */
      if( !*sp )                /* Got to the end without a match. */
        {
        return( NULL );
        }                       /* Still hope for a match. */
      else
        {
        /* Now sp should point to a matching character. */
        StrnCpy(matching_bit, match_start, sp-match_start);
        /* Back to needing a stright match again. */
        while( (*sp)            /* Not the end of the string. */
            && (*pp)            /* Not the end of the pattern. */
            && (*sp == *pp) )   /* The two match. */
          {
          sp++;                 /* Keep looking. */
          pp++;
          }
        if( !*sp && !*pp )      /* Both at end so it matched */
          return( matching_bit );
        else
          return( NULL );
        }
      }
    }
  return( NULL );               /* No match. */
  } /* map_filename */


/* ************************************************************************** **
 * MangledMap is a series of name pairs in () separated by spaces.
 * If s matches the first of the pair then the name given is the
 * second of the pair.  A * means any number of any character and if
 * present in the second of the pair as well as the first the
 * matching part of the first string takes the place of the * in the
 * second.
 *
 * I wanted this so that we could have RCS files which can be used
 * by UNIX and DOS programs.  My mapping string is (RCS rcs) which
 * converts the UNIX RCS file subdirectory to lowercase thus
 * preventing mangling.
 *
 * See 'mangled map' in smb.conf(5).
 *
 * ************************************************************************** **
 */
static void mangled_map(char *s, const char *MangledMap)
{
	const char *start=MangledMap;       /* Use this to search for mappings. */
	const char *end;                    /* Used to find the end of strings. */
	char *match_string;
	pstring new_string;           /* Make up the result here. */
	char *np;                     /* Points into new_string. */

	DEBUG( 5, ("Mangled Mapping '%s' map '%s'\n", s, MangledMap) );
	while( *start ) {
		while( (*start) && (*start != '(') )
			start++;
		if( !*start )
			continue;                 /* Always check for the end. */
		start++;                    /* Skip the ( */
		end = start;                /* Search for the ' ' or a ')' */
		DEBUG( 5, ("Start of first in pair '%s'\n", start) );
		while( (*end) && !((*end == ' ') || (*end == ')')) )
			end++;
		if( !*end ) {
			start = end;
			continue;                 /* Always check for the end. */
		}
		DEBUG( 5, ("End of first in pair '%s'\n", end) );
		if( (match_string = map_filename( s, start, end-start )) ) {
			int size_left = sizeof(new_string) - 1;
			DEBUG( 5, ("Found a match\n") );
			/* Found a match. */
			start = end + 1; /* Point to start of what it is to become. */
			DEBUG( 5, ("Start of second in pair '%s'\n", start) );
			end = start;
			np = new_string;
			while( (*end && size_left > 0)    /* Not the end of string. */
			       && (*end != ')')      /* Not the end of the pattern. */
			       && (*end != '*') ) {   /* Not a wildcard. */
				*np++ = *end++;
				size_left--;
			}

			if( !*end ) {
				start = end;
				continue;               /* Always check for the end. */
			}
			if( *end == '*' ) {
				if (size_left > 0 )
					safe_strcpy( np, match_string, size_left );
				np += strlen( match_string );
				size_left -= strlen( match_string );
				end++;                  /* Skip the '*' */
				while ((*end && size_left >  0)   /* Not the end of string. */
				       && (*end != ')') /* Not the end of the pattern. */
				       && (*end != '*')) { /* Not a wildcard. */
					*np++ = *end++;
					size_left--;
				}
			}
			if (!*end) {
				start = end;
				continue;               /* Always check for the end. */
			}
			if (size_left > 0)
				*np++ = '\0';             /* NULL terminate it. */
			DEBUG(5,("End of second in pair '%s'\n", end));
			new_string[sizeof(new_string)-1] = '\0';
			pstrcpy( s, new_string );  /* Substitute with the new name. */
			DEBUG( 5, ("s is now '%s'\n", s) );
		}
		start = end;  /* Skip a bit which cannot be wanted anymore. */
		start++;
	}
}

/*
  front end routine to the mangled map code 
  personally I think that the whole idea of "mangled map" is completely bogus
*/
void mangle_map_filename(fstring fname, int snum)
{
	char *map;

	map = lp_mangled_map(snum);
	if (!map || !*map) return;

	mangled_map(fname, map);
}
