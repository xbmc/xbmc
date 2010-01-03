/* Copyright (C) 1992,1995-1999,2000-2003,2005-2008 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#if !_LIBC
# include <config.h>
#endif
#include <alloca.h>

/* Specification.  */
#include <stdlib.h>

#include <errno.h>
#ifndef __set_errno
# define __set_errno(ev) ((errno) = (ev))
#endif

#include <string.h>
#if _LIBC || HAVE_UNISTD_H
# include <unistd.h>
#endif

#if _LIBC || !HAVE_SETENV

#if !_LIBC
# include "malloca.h"
#endif

#if !_LIBC
# define __environ	environ
#endif

#if _LIBC
/* This lock protects against simultaneous modifications of `environ'.  */
# include <bits/libc-lock.h>
__libc_lock_define_initialized (static, envlock)
# define LOCK	__libc_lock_lock (envlock)
# define UNLOCK	__libc_lock_unlock (envlock)
#else
# define LOCK
# define UNLOCK
#endif

/* In the GNU C library we must keep the namespace clean.  */
#ifdef _LIBC
# define setenv __setenv
# define clearenv __clearenv
# define tfind __tfind
# define tsearch __tsearch
#endif

/* In the GNU C library implementation we try to be more clever and
   allow arbitrarily many changes of the environment given that the used
   values are from a small set.  Outside glibc this will eat up all
   memory after a while.  */
#if defined _LIBC || (defined HAVE_SEARCH_H && defined HAVE_TSEARCH \
		      && defined __GNUC__)
# define USE_TSEARCH	1
# include <search.h>
typedef int (*compar_fn_t) (const void *, const void *);

/* This is a pointer to the root of the search tree with the known
   values.  */
static void *known_values;

# define KNOWN_VALUE(Str) \
  ({									      \
    void *value = tfind (Str, &known_values, (compar_fn_t) strcmp);	      \
    value != NULL ? *(char **) value : NULL;				      \
  })
# define STORE_VALUE(Str) \
  tsearch (Str, &known_values, (compar_fn_t) strcmp)

#else
# undef USE_TSEARCH

# define KNOWN_VALUE(Str) NULL
# define STORE_VALUE(Str) do { } while (0)

#endif


/* If this variable is not a null pointer we allocated the current
   environment.  */
static char **last_environ;


/* This function is used by `setenv' and `putenv'.  The difference between
   the two functions is that for the former must create a new string which
   is then placed in the environment, while the argument of `putenv'
   must be used directly.  This is all complicated by the fact that we try
   to reuse values once generated for a `setenv' call since we can never
   free the strings.  */
int
__add_to_environ (const char *name, const char *value, const char *combined,
		  int replace)
{
  register char **ep;
  register size_t size;
  const size_t namelen = strlen (name);
  const size_t vallen = value != NULL ? strlen (value) + 1 : 0;

  LOCK;

  /* We have to get the pointer now that we have the lock and not earlier
     since another thread might have created a new environment.  */
  ep = __environ;

  size = 0;
  if (ep != NULL)
    {
      for (; *ep != NULL; ++ep)
	if (!strncmp (*ep, name, namelen) && (*ep)[namelen] == '=')
	  break;
	else
	  ++size;
    }

  if (ep == NULL || *ep == NULL)
    {
      char **new_environ;
#ifdef USE_TSEARCH
      char *new_value;
#endif

      /* We allocated this space; we can extend it.  */
      new_environ =
	(char **) (last_environ == NULL
		   ? malloc ((size + 2) * sizeof (char *))
		   : realloc (last_environ, (size + 2) * sizeof (char *)));
      if (new_environ == NULL)
	{
	  UNLOCK;
	  return -1;
	}

      /* If the whole entry is given add it.  */
      if (combined != NULL)
	/* We must not add the string to the search tree since it belongs
	   to the user.  */
	new_environ[size] = (char *) combined;
      else
	{
	  /* See whether the value is already known.  */
#ifdef USE_TSEARCH
# ifdef _LIBC
	  new_value = (char *) alloca (namelen + 1 + vallen);
	  __mempcpy (__mempcpy (__mempcpy (new_value, name, namelen), "=", 1),
		     value, vallen);
# else
	  new_value = (char *) malloca (namelen + 1 + vallen);
	  if (new_value == NULL)
	    {
	      __set_errno (ENOMEM);
	      UNLOCK;
	      return -1;
	    }
	  memcpy (new_value, name, namelen);
	  new_value[namelen] = '=';
	  memcpy (&new_value[namelen + 1], value, vallen);
# endif

	  new_environ[size] = KNOWN_VALUE (new_value);
	  if (new_environ[size] == NULL)
#endif
	    {
	      new_environ[size] = (char *) malloc (namelen + 1 + vallen);
	      if (new_environ[size] == NULL)
		{
#if defined USE_TSEARCH && !defined _LIBC
		  freea (new_value);
#endif
		  __set_errno (ENOMEM);
		  UNLOCK;
		  return -1;
		}

#ifdef USE_TSEARCH
	      memcpy (new_environ[size], new_value, namelen + 1 + vallen);
#else
	      memcpy (new_environ[size], name, namelen);
	      new_environ[size][namelen] = '=';
	      memcpy (&new_environ[size][namelen + 1], value, vallen);
#endif
	      /* And save the value now.  We cannot do this when we remove
		 the string since then we cannot decide whether it is a
		 user string or not.  */
	      STORE_VALUE (new_environ[size]);
	    }
#if defined USE_TSEARCH && !defined _LIBC
	  freea (new_value);
#endif
	}

      if (__environ != last_environ)
	memcpy ((char *) new_environ, (char *) __environ,
		size * sizeof (char *));

      new_environ[size + 1] = NULL;

      last_environ = __environ = new_environ;
    }
  else if (replace)
    {
      char *np;

      /* Use the user string if given.  */
      if (combined != NULL)
	np = (char *) combined;
      else
	{
#ifdef USE_TSEARCH
	  char *new_value;
# ifdef _LIBC
	  new_value = alloca (namelen + 1 + vallen);
	  __mempcpy (__mempcpy (__mempcpy (new_value, name, namelen), "=", 1),
		     value, vallen);
# else
	  new_value = malloca (namelen + 1 + vallen);
	  if (new_value == NULL)
	    {
	      __set_errno (ENOMEM);
	      UNLOCK;
	      return -1;
	    }
	  memcpy (new_value, name, namelen);
	  new_value[namelen] = '=';
	  memcpy (&new_value[namelen + 1], value, vallen);
# endif

	  np = KNOWN_VALUE (new_value);
	  if (np == NULL)
#endif
	    {
	      np = malloc (namelen + 1 + vallen);
	      if (np == NULL)
		{
#if defined USE_TSEARCH && !defined _LIBC
		  freea (new_value);
#endif
		  __set_errno (ENOMEM);
		  UNLOCK;
		  return -1;
		}

#ifdef USE_TSEARCH
	      memcpy (np, new_value, namelen + 1 + vallen);
#else
	      memcpy (np, name, namelen);
	      np[namelen] = '=';
	      memcpy (&np[namelen + 1], value, vallen);
#endif
	      /* And remember the value.  */
	      STORE_VALUE (np);
	    }
#if defined USE_TSEARCH && !defined _LIBC
	  freea (new_value);
#endif
	}

      *ep = np;
    }

  UNLOCK;

  return 0;
}

int
setenv (const char *name, const char *value, int replace)
{
  return __add_to_environ (name, value, NULL, replace);
}

/* The `clearenv' was planned to be added to POSIX.1 but probably
   never made it.  Nevertheless the POSIX.9 standard (POSIX bindings
   for Fortran 77) requires this function.  */
int
clearenv (void)
{
  LOCK;

  if (__environ == last_environ && __environ != NULL)
    {
      /* We allocated this environment so we can free it.  */
      free (__environ);
      last_environ = NULL;
    }

  /* Clear the environment pointer removes the whole environment.  */
  __environ = NULL;

  UNLOCK;

  return 0;
}

#ifdef _LIBC
static void
free_mem (void)
{
  /* Remove all traces.  */
  clearenv ();

  /* Now remove the search tree.  */
  __tdestroy (known_values, free);
  known_values = NULL;
}
text_set_element (__libc_subfreeres, free_mem);


# undef setenv
# undef clearenv
weak_alias (__setenv, setenv)
weak_alias (__clearenv, clearenv)
#endif

#endif /* _LIBC || !HAVE_SETENV */
