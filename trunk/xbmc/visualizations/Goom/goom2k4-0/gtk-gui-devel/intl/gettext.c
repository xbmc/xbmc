/* Implementation of gettext(3) function.
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

#ifdef _LIBC
# define __need_NULL
# include <stddef.h>
#else
# ifdef STDC_HEADERS
#  include <stdlib.h>		/* Just for NULL.  */
# else
#  ifdef HAVE_STRING_H
#   include <string.h>
#  else
#   define NULL ((void *) 0)
#  endif
# endif
#endif

#ifdef _LIBC
# include <libintl.h>
#else
# include "libgettext.h"
#endif

/* @@ end of prolog @@ */

/* Names for the libintl functions are a problem.  They must not clash
   with existing names and they should follow ANSI C.  But this source
   code is also used in GNU C Library where the names have a __
   prefix.  So we have to make a difference here.  */
#ifdef _LIBC
# define GETTEXT __gettext
# define DGETTEXT __dgettext
#else
# define GETTEXT gettext__
# define DGETTEXT dgettext__
#endif

/* Look up MSGID in the current default message catalog for the current
   LC_MESSAGES locale.  If not found, returns MSGID itself (the default
   text).  */
char *
GETTEXT (msgid)
     const char *msgid;
{
  return DGETTEXT (NULL, msgid);
}

#ifdef _LIBC
/* Alias for function name in GNU C Library.  */
weak_alias (__gettext, gettext);
#endif
