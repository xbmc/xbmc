/* Compatibility code for gettext-using-catgets interface.
   Copyright (C) 1995, 1997 Free Software Foundation, Inc.

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

#include <stdio.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <string.h>
#else
char *getenv ();
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif

#ifdef HAVE_NL_TYPES_H
# include <nl_types.h>
#endif

#include "libgettext.h"

/* @@ end of prolog @@ */

/* XPG3 defines the result of `setlocale (category, NULL)' as:
   ``Directs `setlocale()' to query `category' and return the current
     setting of `local'.''
   However it does not specify the exact format.  And even worse: POSIX
   defines this not at all.  So we can use this feature only on selected
   system (e.g. those using GNU C Library).  */
#ifdef _LIBC
# define HAVE_LOCALE_NULL
#endif

/* The catalog descriptor.  */
static nl_catd catalog = (nl_catd) -1;

/* Name of the default catalog.  */
static const char default_catalog_name[] = "messages";

/* Name of currently used catalog.  */
static const char *catalog_name = default_catalog_name;

/* Get ID for given string.  If not found return -1.  */
static int msg_to_cat_id PARAMS ((const char *msg));

/* Substitution for systems lacking this function in their C library.  */
#if !_LIBC && !HAVE_STPCPY
static char *stpcpy PARAMS ((char *dest, const char *src));
#endif


/* Set currently used domain/catalog.  */
char *
textdomain (domainname)
     const char *domainname;
{
  nl_catd new_catalog;
  char *new_name;
  size_t new_name_len;
  char *lang;

#if defined HAVE_SETLOCALE && defined HAVE_LC_MESSAGES \
    && defined HAVE_LOCALE_NULL
  lang = setlocale (LC_MESSAGES, NULL);
#else
  lang = getenv ("LC_ALL");
  if (lang == NULL || lang[0] == '\0')
    {
      lang = getenv ("LC_MESSAGES");
      if (lang == NULL || lang[0] == '\0')
	lang = getenv ("LANG");
    }
#endif
  if (lang == NULL || lang[0] == '\0')
    lang = "C";

  /* See whether name of currently used domain is asked.  */
  if (domainname == NULL)
    return (char *) catalog_name;

  if (domainname[0] == '\0')
    domainname = default_catalog_name;

  /* Compute length of added path element.  */
  new_name_len = sizeof (LOCALEDIR) - 1 + 1 + strlen (lang)
		 + sizeof ("/LC_MESSAGES/") - 1 + sizeof (PACKAGE) - 1
		 + sizeof (".cat");

  new_name = (char *) malloc (new_name_len);
  if (new_name == NULL)
    return NULL;

  strcpy (new_name, PACKAGE);
  new_catalog = catopen (new_name, 0);

  if (new_catalog == (nl_catd) -1)
    {
      /* NLSPATH search didn't work, try absolute path */
      sprintf (new_name, "%s/%s/LC_MESSAGES/%s.cat", LOCALEDIR, lang,
	       PACKAGE);
      new_catalog = catopen (new_name, 0);

      if (new_catalog == (nl_catd) -1)
	{
	  free (new_name);
	  return (char *) catalog_name;
	}
    }

  /* Close old catalog.  */
  if (catalog != (nl_catd) -1)
    catclose (catalog);
  if (catalog_name != default_catalog_name)
    free ((char *) catalog_name);

  catalog = new_catalog;
  catalog_name = new_name;

  return (char *) catalog_name;
}

char *
bindtextdomain (domainname, dirname)
     const char *domainname;
     const char *dirname;
{
#if HAVE_SETENV || HAVE_PUTENV
  char *old_val, *new_val, *cp;
  size_t new_val_len;

  /* This does not make much sense here but to be compatible do it.  */
  if (domainname == NULL)
    return NULL;

  /* Compute length of added path element.  If we use setenv we don't need
     the first byts for NLSPATH=, but why complicate the code for this
     peanuts.  */
  new_val_len = sizeof ("NLSPATH=") - 1 + strlen (dirname)
		+ sizeof ("/%L/LC_MESSAGES/%N.cat");

  old_val = getenv ("NLSPATH");
  if (old_val == NULL || old_val[0] == '\0')
    {
      old_val = NULL;
      new_val_len += 1 + sizeof (LOCALEDIR) - 1
	             + sizeof ("/%L/LC_MESSAGES/%N.cat");
    }
  else
    new_val_len += strlen (old_val);

  new_val = (char *) malloc (new_val_len);
  if (new_val == NULL)
    return NULL;

# if HAVE_SETENV
  cp = new_val;
# else
  cp = stpcpy (new_val, "NLSPATH=");
# endif

  cp = stpcpy (cp, dirname);
  cp = stpcpy (cp, "/%L/LC_MESSAGES/%N.cat:");

  if (old_val == NULL)
    {
# if __STDC__
      stpcpy (cp, LOCALEDIR "/%L/LC_MESSAGES/%N.cat");
# else

      cp = stpcpy (cp, LOCALEDIR);
      stpcpy (cp, "/%L/LC_MESSAGES/%N.cat");
# endif
    }
  else
    stpcpy (cp, old_val);

# if HAVE_SETENV
  setenv ("NLSPATH", new_val, 1);
  free (new_val);
# else
  putenv (new_val);
  /* Do *not* free the environment entry we just entered.  It is used
     from now on.   */
# endif

#endif

  return (char *) domainname;
}

#undef gettext
char *
gettext (msg)
     const char *msg;
{
  int msgid;

  if (msg == NULL || catalog == (nl_catd) -1)
    return (char *) msg;

  /* Get the message from the catalog.  We always use set number 1.
     The message ID is computed by the function `msg_to_cat_id'
     which works on the table generated by `po-to-tbl'.  */
  msgid = msg_to_cat_id (msg);
  if (msgid == -1)
    return (char *) msg;

  return catgets (catalog, 1, msgid, (char *) msg);
}

/* Look through the table `_msg_tbl' which has `_msg_tbl_length' entries
   for the one equal to msg.  If it is found return the ID.  In case when
   the string is not found return -1.  */
static int
msg_to_cat_id (msg)
     const char *msg;
{
  int cnt;

  for (cnt = 0; cnt < _msg_tbl_length; ++cnt)
    if (strcmp (msg, _msg_tbl[cnt]._msg) == 0)
      return _msg_tbl[cnt]._msg_number;

  return -1;
}


/* @@ begin of epilog @@ */

/* We don't want libintl.a to depend on any other library.  So we
   avoid the non-standard function stpcpy.  In GNU C Library this
   function is available, though.  Also allow the symbol HAVE_STPCPY
   to be defined.  */
#if !_LIBC && !HAVE_STPCPY
static char *
stpcpy (dest, src)
     char *dest;
     const char *src;
{
  while ((*dest++ = *src++) != '\0')
    /* Do nothing. */ ;
  return dest - 1;
}
#endif
