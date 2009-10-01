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
/* dl_dlopen.c */

#include <stdio.h>
#include "timidity.h"

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>	/* the dynamic linker include file for Sunos/Solaris */
#else
#include <nlist.h>
#include <link.h>
#endif

#ifndef RTLD_LAZY
# define RTLD_LAZY 1	/* Solaris 1 */
#endif

#ifdef __NetBSD__
# define dlerror() strerror(errno)
#endif


#include "dlutils.h"

/*ARGSUSED*/
void dl_init(int argc, char **argv)
{
    /* Do nothing */
}

void *dl_load_file(char *filename)
{
    int mode = RTLD_LAZY;
    void *RETVAL;

    RETVAL = dlopen(filename, mode) ;
    if (RETVAL == NULL)
	fprintf(stderr, "%s\n", dlerror());
    return RETVAL;
}

void *dl_find_symbol(void *libhandle, char *symbolname)
{
    void *RETVAL;

#ifdef DLSYM_NEEDS_UNDERSCORE
    char buff[BUFSIZ];
    sprintf(buff, "_%s", symbolname);
    symbolname = buff;
#endif

    RETVAL = dlsym(libhandle, symbolname);
    if (RETVAL == NULL)
	fprintf(stderr, "%s\n", dlerror());
    return RETVAL;
}

void dl_free(void *libhandle)
{
    dlclose(libhandle);
}
