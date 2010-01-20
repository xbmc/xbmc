/* Implementation of the bindtextdomain(3) function
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.

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

#if defined STDC_HEADERS || defined _LIBC
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# else
void free ();
# endif
#endif

#if defined HAVE_STRING_H || defined _LIBC
# include <string.h>
#else
# include <strings.h>
# ifndef memcpy
#  define memcpy(Dst, Src, Num) bcopy (Src, Dst, Num)
# endif
#endif

#ifdef _LIBC
# include <libintl.h>
#else
# include "libgettext.h"
#endif
#include "gettext.h"
#include "gettextP.h"

/* @@ end of prolog @@ */

/* Contains the default location of the message catalogs.  */
extern const char _nl_default_dirname[];

/* List with bindings of specific domains.  */
extern struct binding *_nl_domain_bindings;


/* Names for the libintl functions are a problem.  They must not clash
   with existing names and they should follow ANSI C.  But this source
   code is also used in GNU C Library where the names have a __
   prefix.  So we have to make a difference here.  */
#ifdef _LIBC
# define BINDTEXTDOMAIN __bindtextdomain
# ifndef strdup
#  define strdup(str) __strdup (str)
# endif
#else
# define BINDTEXTDOMAIN bindtextdomain__
#endif

/* Specify that the DOMAINNAME message catalog will be found
   in DIRNAME rather than in the system locale data base.  */
char *
BINDTEXTDOMAIN (domainname, dirname)
     const char *domainname;
     const char *dirname;
{
  struct binding *binding;

  /* Some sanity checks.  */
  if (domainname == NULL || domainname[0] == '\0')
    return NULL;

  for (binding = _nl_domain_bindings; binding != NULL; binding = binding->next)
    {
      int compare = strcmp (domainname, binding->domainname);
      if (compare == 0)
	/* We found it!  */
	break;
      if (compare < 0)
	{
	  /* It is not in the list.  */
	  binding = NULL;
	  break;
	}
    }

  if (dirname == NULL)
    /* The current binding has be to returned.  */
    return binding == NULL ? (char *) _nl_default_dirname : binding->dirname;

  if (binding != NULL)
    {
      /* The domain is already bound.  If the new value and the old
	 one are equal we simply do nothing.  Otherwise replace the
	 old binding.  */
      if (strcmp (dirname, binding->dirname) != 0)
	{
	  char *new_dirname;

	  if (strcmp (dirname, _nl_default_dirname) == 0)
	    new_dirname = (char *) _nl_default_dirname;
	  else
	    {
#if defined _LIBC || defined HAVE_STRDUP
	      new_dirname = strdup (dirname);
	      if (new_dirname == NULL)
		return NULL;
#else
	      size_t len = strlen (dirname) + 1;
	      new_dirname = (char *) malloc (len);
	      if (new_dirname == NULL)
		return NULL;

	      memcpy (new_dirname, dirname, len);
#endif
	    }

	  if (binding->dirname != _nl_default_dirname)
	    free (binding->dirname);

	  binding->dirname = new_dirname;
	}
    }
  else
    {
      /* We have to create a new binding.  */
#if !defined _LIBC && !defined HAVE_STRDUP
      size_t len;
#endif
      struct binding *new_binding =
	(struct binding *) malloc (sizeof (*new_binding));

      if (new_binding == NULL)
	return NULL;

#if defined _LIBC || defined HAVE_STRDUP
      new_binding->domainname = strdup (domainname);
      if (new_binding->domainname == NULL)
	return NULL;
#else
      len = strlen (domainname) + 1;
      new_binding->domainname = (char *) malloc (len);
      if (new_binding->domainname == NULL)
	return NULL;
      memcpy (new_binding->domainname, domainname, len);
#endif

      if (strcmp (dirname, _nl_default_dirname) == 0)
	new_binding->dirname = (char *) _nl_default_dirname;
      else
	{
#if defined _LIBC || defined HAVE_STRDUP
	  new_binding->dirname = strdup (dirname);
	  if (new_binding->dirname == NULL)
	    return NULL;
#else
	  len = strlen (dirname) + 1;
	  new_binding->dirname = (char *) malloc (len);
	  if (new_binding->dirname == NULL)
	    return NULL;
	  memcpy (new_binding->dirname, dirname, len);
#endif
	}

      /* Now enqueue it.  */
      if (_nl_domain_bindings == NULL
	  || strcmp (domainname, _nl_domain_bindings->domainname) < 0)
	{
	  new_binding->next = _nl_domain_bindings;
	  _nl_domain_bindings = new_binding;
	}
      else
	{
	  binding = _nl_domain_bindings;
	  while (binding->next != NULL
		 && strcmp (domainname, binding->next->domainname) > 0)
	    binding = binding->next;

	  new_binding->next = binding->next;
	  binding->next = new_binding;
	}

      binding = new_binding;
    }

  return binding->dirname;
}

#ifdef _LIBC
/* Alias for function name in GNU C Library.  */
weak_alias (__bindtextdomain, bindtextdomain);
#endif
