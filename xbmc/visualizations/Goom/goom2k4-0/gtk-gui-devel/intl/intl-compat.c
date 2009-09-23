/* intl-compat.c - Stub functions to call gettext functions from GNU gettext
   Library.
   Copyright (C) 1995 Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "libgettext.h"

/* @@ end of prolog @@ */


#undef gettext
#undef dgettext
#undef dcgettext
#undef textdomain
#undef bindtextdomain


char *
bindtextdomain (domainname, dirname)
     const char *domainname;
     const char *dirname;
{
  return bindtextdomain__ (domainname, dirname);
}


char *
dcgettext (domainname, msgid, category)
     const char *domainname;
     const char *msgid;
     int category;
{
  return dcgettext__ (domainname, msgid, category);
}


char *
dgettext (domainname, msgid)
     const char *domainname;
     const char *msgid;
{
  return dgettext__ (domainname, msgid);
}


char *
gettext (msgid)
     const char *msgid;
{
  return gettext__ (msgid);
}


char *
textdomain (domainname)
     const char *domainname;
{
  return textdomain__ (domainname);
}
