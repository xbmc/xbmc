/* Message catalogs for internationalization.
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

/* Because on some systems (e.g. Solaris) we sometimes have to include
   the systems libintl.h as well as this file we have more complex
   include protection above.  But the systems header might perhaps also
   define _LIBINTL_H and therefore we have to protect the definition here.  */

#if !defined _LIBINTL_H || !defined _LIBGETTEXT_H
#ifndef _LIBINTL_H
# define _LIBINTL_H	1
#endif
#define _LIBGETTEXT_H	1

/* We define an additional symbol to signal that we use the GNU
   implementation of gettext.  */
#define __USE_GNU_GETTEXT 1

#include <sys/types.h>

#if HAVE_LOCALE_H
# include <locale.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* @@ end of prolog @@ */

#ifndef PARAMS
# if __STDC__ || defined __cplusplus
#  define PARAMS(args) args
# else
#  define PARAMS(args) ()
# endif
#endif

#ifndef NULL
# if !defined __cplusplus || defined __GNUC__
#  define NULL ((void *) 0)
# else
#  define NULL (0)
# endif
#endif

#if !HAVE_LC_MESSAGES
/* This value determines the behaviour of the gettext() and dgettext()
   function.  But some system does not have this defined.  Define it
   to a default value.  */
# define LC_MESSAGES (-1)
#endif


/* Declarations for gettext-using-catgets interface.  Derived from
   Jim Meyering's libintl.h.  */
struct _msg_ent
{
  const char *_msg;
  int _msg_number;
};


#if HAVE_CATGETS
/* These two variables are defined in the automatically by po-to-tbl.sed
   generated file `cat-id-tbl.c'.  */
extern const struct _msg_ent _msg_tbl[];
extern int _msg_tbl_length;
#endif


/* For automatical extraction of messages sometimes no real
   translation is needed.  Instead the string itself is the result.  */
#define gettext_noop(Str) (Str)

/* Look up MSGID in the current default message catalog for the current
   LC_MESSAGES locale.  If not found, returns MSGID itself (the default
   text).  */
extern char *gettext PARAMS ((const char *__msgid));
extern char *gettext__ PARAMS ((const char *__msgid));

/* Look up MSGID in the DOMAINNAME message catalog for the current
   LC_MESSAGES locale.  */
extern char *dgettext PARAMS ((const char *__domainname, const char *__msgid));
extern char *dgettext__ PARAMS ((const char *__domainname,
				 const char *__msgid));

/* Look up MSGID in the DOMAINNAME message catalog for the current CATEGORY
   locale.  */
extern char *dcgettext PARAMS ((const char *__domainname, const char *__msgid,
				int __category));
extern char *dcgettext__ PARAMS ((const char *__domainname,
				  const char *__msgid, int __category));


/* Set the current default message catalog to DOMAINNAME.
   If DOMAINNAME is null, return the current default.
   If DOMAINNAME is "", reset to the default of "messages".  */
extern char *textdomain PARAMS ((const char *__domainname));
extern char *textdomain__ PARAMS ((const char *__domainname));

/* Specify that the DOMAINNAME message catalog will be found
   in DIRNAME rather than in the system locale data base.  */
extern char *bindtextdomain PARAMS ((const char *__domainname,
				  const char *__dirname));
extern char *bindtextdomain__ PARAMS ((const char *__domainname,
				    const char *__dirname));

#if ENABLE_NLS

/* Solaris 2.3 has the gettext function but dcgettext is missing.
   So we omit this optimization for Solaris 2.3.  BTW, Solaris 2.4
   has dcgettext.  */
# if !HAVE_CATGETS && (!HAVE_GETTEXT || HAVE_DCGETTEXT)

#  define gettext(Msgid)						      \
     dgettext (NULL, Msgid)

#  define dgettext(Domainname, Msgid)					      \
     dcgettext (Domainname, Msgid, LC_MESSAGES)

#  if defined __GNUC__ && __GNUC__ == 2 && __GNUC_MINOR__ >= 7
/* This global variable is defined in loadmsgcat.c.  We need a sign,
   whether a new catalog was loaded, which can be associated with all
   translations.  */
extern int _nl_msg_cat_cntr;

#   define dcgettext(Domainname, Msgid, Category)			      \
  (__extension__							      \
   ({									      \
     char *__result;							      \
     if (__builtin_constant_p (Msgid))					      \
       {								      \
	 static char *__translation__;					      \
	 static int __catalog_counter__;				      \
	 if (! __translation__ || __catalog_counter__ != _nl_msg_cat_cntr)    \
	   {								      \
	     __translation__ =						      \
	       dcgettext__ (Domainname, Msgid, Category);		      \
	     __catalog_counter__ = _nl_msg_cat_cntr;			      \
	   }								      \
	 __result = __translation__;					      \
       }								      \
     else								      \
       __result = dcgettext__ (Domainname, Msgid, Category);		      \
     __result;								      \
    }))
#  endif
# endif

#else

# define gettext(Msgid) (Msgid)
# define dgettext(Domainname, Msgid) (Msgid)
# define dcgettext(Domainname, Msgid, Category) (Msgid)
# define textdomain(Domainname) ((char *) Domainname)
# define bindtextdomain(Domainname, Dirname) ((char *) Dirname)

#endif

/* @@ begin of epilog @@ */

#ifdef __cplusplus
}
#endif

#endif
