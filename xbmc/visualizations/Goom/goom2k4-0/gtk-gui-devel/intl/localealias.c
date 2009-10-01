/* Handle aliases for locale names.
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef __GNUC__
# define alloca __builtin_alloca
# define HAVE_ALLOCA 1
#else
# if defined HAVE_ALLOCA_H || defined _LIBC
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca
char *alloca ();
#   endif
#  endif
# endif
#endif

#if defined STDC_HEADERS || defined _LIBC
# include <stdlib.h>
#else
char *getenv ();
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# else
void free ();
# endif
#endif

#if defined HAVE_STRING_H || defined _LIBC
# ifndef _GNU_SOURCE
#  define _GNU_SOURCE	1
# endif
# include <string.h>
#else
# include <strings.h>
# ifndef memcpy
#  define memcpy(Dst, Src, Num) bcopy (Src, Dst, Num)
# endif
#endif
#if !HAVE_STRCHR && !defined _LIBC
# ifndef strchr
#  define strchr index
# endif
#endif

#include "gettext.h"
#include "gettextP.h"

/* @@ end of prolog @@ */

#ifdef _LIBC
/* Rename the non ANSI C functions.  This is required by the standard
   because some ANSI C functions will require linking with this object
   file and the name space must not be polluted.  */
# define strcasecmp __strcasecmp

# define mempcpy __mempcpy
# define HAVE_MEMPCPY	1

/* We need locking here since we can be called from different places.  */
# include <bits/libc-lock.h>

__libc_lock_define_initialized (static, lock);
#endif


/* For those loosing systems which don't have `alloca' we have to add
   some additional code emulating it.  */
#ifdef HAVE_ALLOCA
/* Nothing has to be done.  */
# define ADD_BLOCK(list, address) /* nothing */
# define FREE_BLOCKS(list) /* nothing */
#else
struct block_list
{
  void *address;
  struct block_list *next;
};
# define ADD_BLOCK(list, addr)						      \
  do {									      \
    struct block_list *newp = (struct block_list *) malloc (sizeof (*newp));  \
    /* If we cannot get a free block we cannot add the new element to	      \
       the list.  */							      \
    if (newp != NULL) {							      \
      newp->address = (addr);						      \
      newp->next = (list);						      \
      (list) = newp;							      \
    }									      \
  } while (0)
# define FREE_BLOCKS(list)						      \
  do {									      \
    while (list != NULL) {						      \
      struct block_list *old = list;					      \
      list = list->next;						      \
      free (old);							      \
    }									      \
  } while (0)
# undef alloca
# define alloca(size) (malloc (size))
#endif	/* have alloca */


struct alias_map
{
  const char *alias;
  const char *value;
};


static char *string_space = NULL;
static size_t string_space_act = 0;
static size_t string_space_max = 0;
static struct alias_map *map;
static size_t nmap = 0;
static size_t maxmap = 0;


/* Prototypes for local functions.  */
static size_t read_alias_file PARAMS ((const char *fname, int fname_len))
     internal_function;
static void extend_alias_table PARAMS ((void));
static int alias_compare PARAMS ((const struct alias_map *map1,
				  const struct alias_map *map2));


const char *
_nl_expand_alias (name)
    const char *name;
{
  static const char *locale_alias_path = LOCALE_ALIAS_PATH;
  struct alias_map *retval;
  const char *result = NULL;
  size_t added;

#ifdef _LIBC
  __libc_lock_lock (lock);
#endif

  do
    {
      struct alias_map item;

      item.alias = name;

      if (nmap > 0)
	retval = (struct alias_map *) bsearch (&item, map, nmap,
					       sizeof (struct alias_map),
					       (int (*) PARAMS ((const void *,
								 const void *))
						) alias_compare);
      else
	retval = NULL;

      /* We really found an alias.  Return the value.  */
      if (retval != NULL)
	{
	  result = retval->value;
	  break;
	}

      /* Perhaps we can find another alias file.  */
      added = 0;
      while (added == 0 && locale_alias_path[0] != '\0')
	{
	  const char *start;

	  while (locale_alias_path[0] == ':')
	    ++locale_alias_path;
	  start = locale_alias_path;

	  while (locale_alias_path[0] != '\0' && locale_alias_path[0] != ':')
	    ++locale_alias_path;

	  if (start < locale_alias_path)
	    added = read_alias_file (start, locale_alias_path - start);
	}
    }
  while (added != 0);

#ifdef _LIBC
  __libc_lock_unlock (lock);
#endif

  return result;
}


static size_t
internal_function
read_alias_file (fname, fname_len)
     const char *fname;
     int fname_len;
{
#ifndef HAVE_ALLOCA
  struct block_list *block_list = NULL;
#endif
  FILE *fp;
  char *full_fname;
  size_t added;
  static const char aliasfile[] = "/locale.alias";

  full_fname = (char *) alloca (fname_len + sizeof aliasfile);
  ADD_BLOCK (block_list, full_fname);
#ifdef HAVE_MEMPCPY
  mempcpy (mempcpy (full_fname, fname, fname_len),
	   aliasfile, sizeof aliasfile);
#else
  memcpy (full_fname, fname, fname_len);
  memcpy (&full_fname[fname_len], aliasfile, sizeof aliasfile);
#endif

  fp = fopen (full_fname, "r");
  if (fp == NULL)
    {
      FREE_BLOCKS (block_list);
      return 0;
    }

  added = 0;
  while (!feof (fp))
    {
      /* It is a reasonable approach to use a fix buffer here because
	 a) we are only interested in the first two fields
	 b) these fields must be usable as file names and so must not
	    be that long
       */
      unsigned char buf[BUFSIZ];
      unsigned char *alias;
      unsigned char *value;
      unsigned char *cp;

      if (fgets (buf, sizeof buf, fp) == NULL)
	/* EOF reached.  */
	break;

      /* Possibly not the whole line fits into the buffer.  Ignore
	 the rest of the line.  */
      if (strchr (buf, '\n') == NULL)
	{
	  char altbuf[BUFSIZ];
	  do
	    if (fgets (altbuf, sizeof altbuf, fp) == NULL)
	      /* Make sure the inner loop will be left.  The outer loop
		 will exit at the `feof' test.  */
	      break;
	  while (strchr (altbuf, '\n') == NULL);
	}

      cp = buf;
      /* Ignore leading white space.  */
      while (isspace (cp[0]))
	++cp;

      /* A leading '#' signals a comment line.  */
      if (cp[0] != '\0' && cp[0] != '#')
	{
	  alias = cp++;
	  while (cp[0] != '\0' && !isspace (cp[0]))
	    ++cp;
	  /* Terminate alias name.  */
	  if (cp[0] != '\0')
	    *cp++ = '\0';

	  /* Now look for the beginning of the value.  */
	  while (isspace (cp[0]))
	    ++cp;

	  if (cp[0] != '\0')
	    {
	      size_t alias_len;
	      size_t value_len;

	      value = cp++;
	      while (cp[0] != '\0' && !isspace (cp[0]))
		++cp;
	      /* Terminate value.  */
	      if (cp[0] == '\n')
		{
		  /* This has to be done to make the following test
		     for the end of line possible.  We are looking for
		     the terminating '\n' which do not overwrite here.  */
		  *cp++ = '\0';
		  *cp = '\n';
		}
	      else if (cp[0] != '\0')
		*cp++ = '\0';

	      if (nmap >= maxmap)
		extend_alias_table ();

	      alias_len = strlen (alias) + 1;
	      value_len = strlen (value) + 1;

	      if (string_space_act + alias_len + value_len > string_space_max)
		{
		  /* Increase size of memory pool.  */
		  size_t new_size = (string_space_max
				     + (alias_len + value_len > 1024
					? alias_len + value_len : 1024));
		  char *new_pool = (char *) realloc (string_space, new_size);
		  if (new_pool == NULL)
		    {
		      FREE_BLOCKS (block_list);
		      return added;
		    }
		  string_space = new_pool;
		  string_space_max = new_size;
		}

	      map[nmap].alias = memcpy (&string_space[string_space_act],
					alias, alias_len);
	      string_space_act += alias_len;

	      map[nmap].value = memcpy (&string_space[string_space_act],
					value, value_len);
	      string_space_act += value_len;

	      ++nmap;
	      ++added;
	    }
	}
    }

  /* Should we test for ferror()?  I think we have to silently ignore
     errors.  --drepper  */
  fclose (fp);

  if (added > 0)
    qsort (map, nmap, sizeof (struct alias_map),
	   (int (*) PARAMS ((const void *, const void *))) alias_compare);

  FREE_BLOCKS (block_list);
  return added;
}


static void
extend_alias_table ()
{
  size_t new_size;
  struct alias_map *new_map;

  new_size = maxmap == 0 ? 100 : 2 * maxmap;
  new_map = (struct alias_map *) realloc (map, (new_size
						* sizeof (struct alias_map)));
  if (new_map == NULL)
    /* Simply don't extend: we don't have any more core.  */
    return;

  map = new_map;
  maxmap = new_size;
}


#ifdef _LIBC
static void __attribute__ ((unused))
free_mem (void)
{
  if (string_space != NULL)
    free (string_space);
  if (map != NULL)
    free (map);
}
text_set_element (__libc_subfreeres, free_mem);
#endif


static int
alias_compare (map1, map2)
     const struct alias_map *map1;
     const struct alias_map *map2;
{
#if defined _LIBC || defined HAVE_STRCASECMP
  return strcasecmp (map1->alias, map2->alias);
#else
  const unsigned char *p1 = (const unsigned char *) map1->alias;
  const unsigned char *p2 = (const unsigned char *) map2->alias;
  unsigned char c1, c2;

  if (p1 == p2)
    return 0;

  do
    {
      /* I know this seems to be odd but the tolower() function in
	 some systems libc cannot handle nonalpha characters.  */
      c1 = isupper (*p1) ? tolower (*p1) : *p1;
      c2 = isupper (*p2) ? tolower (*p2) : *p2;
      if (c1 == '\0')
	break;
      ++p1;
      ++p2;
    }
  while (c1 == c2);

  return c1 - c2;
#endif
}
