/* plugin_common - Routines common to several plugins
 * Copyright (C) 2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * Only slightly modified charset.h from:
 * charset.h - 2001/12/04
 *  EasyTAG - Tag editor for MP3 and OGG files
 *  Copyright (C) 1999-2001  H蛆ard Kv虱en <havardk@xmms.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifndef FLAC__PLUGIN_COMMON__CHARSET_H
#define FLAC__PLUGIN_COMMON__CHARSET_H


/**************
 * Prototypes *
 **************/

char *FLAC_plugin__charset_get_current(void);
char *FLAC_plugin__charset_convert_string(const char *string, char *from, char *to);

/* returns 1 for success, 0 for failure or no iconv */
int FLAC_plugin__charset_test_conversion(char *from, char *to);

#endif
