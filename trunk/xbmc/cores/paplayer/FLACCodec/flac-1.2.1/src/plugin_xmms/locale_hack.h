/* plugin_common - Routines common to several plugins
 * Copyright (C) 2002,2003,2004,2006,2007  Josh Coalson
 *
 * Based on:
 * locale.h - 2000/05/05 13:10 Jerome Couderc
 *  EasyTAG - Tag editor for MP3 and OGG files
 *  Copyright (C) 1999-2001  H蛆ard Kv虱en <havardk@xmms.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
/*
 * Gettext support for EasyTAG
 */


#ifndef FLAC__PLUGIN_COMMON__LOCALE_HACK_H
#define FLAC__PLUGIN_COMMON__LOCALE_HACK_H

#include <locale.h>

/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  define _(String) gettext (String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif


#endif
