/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef ___SUPPORT_H_
#define ___SUPPORT_H_

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef WIN32
#include <unistd.h>
#endif

#ifdef WIN32
#define sleep(time) Sleep(time)
#endif

#include <sys/stat.h>

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode)&0xF000) == 0x4000)
#endif /* S_ISDIR */

#ifndef HAVE_STRLCPY
#include <stddef.h>
extern size_t strlcpy(char *dst, const char *src, size_t size);
#endif

#ifndef HAVE_STRLCAT
#include <stddef.h>
extern size_t strlcat(char *dst, const char *src, size_t size);
#endif


#endif
