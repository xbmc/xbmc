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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <windows.h>
#include <stdio.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "timidity.h"
#include "dlutils.h"

/*ARGSUSED*/
void dl_init(int argc, char **argv)
{
}

void *dl_load_file(char *filename)
{
    void *RETVAL;

    RETVAL = (void*)LoadLibrary(filename);
    return RETVAL;
}

void *dl_find_symbol(void *libhandle, char *symbolname)
{
    void *RETVAL;

    RETVAL = (void*) GetProcAddress((HINSTANCE) libhandle, symbolname);
    return RETVAL;
}

void dl_free(void *libhandle)
{
    FreeLibrary((HINSTANCE)libhandle);
}
