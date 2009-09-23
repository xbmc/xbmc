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
/* dl_dld.c */

#include <stdio.h>
#include <dld.h>	/* GNU DLD header file */
#include <unistd.h>
#include "timidity.h"
#include "dlutils.h"

/*ARGSUSED*/
void dl_init(int argc, char **argv)
{
    int dlderr;

#ifdef __linux__
    dlderr = dld_init("/proc/self/exe");
    if (dlderr)
#endif
    {
	dlderr = dld_init(dld_find_executable(argv[0]));
        if (dlderr) {
            char *msg = dld_strerror(dlderr);
            fprintf(stderr, "dld_init(%s) failed: %s\n", argv[0], msg);
        }
    }
}

void *dl_load_file(char *filename)
{
    int dlderr;

    dlderr = dld_link(filename);
    if(dlderr)
    {
	fprintf(stderr, "dld_link(%s): %s\n", filename, dld_strerror(dlderr));
	return NULL;
    }
    return filename;
}

/*ARGSUSED*/
void *dl_find_symbol(void *libhandle, char *symbolname)
{
    void *RETVAL;

    RETVAL = (void *)dld_get_func(symbolname);
    if (RETVAL == NULL)
    {
	fprintf(stderr, "dl_find_symbol: Unable to find '%s' symbol\n",
 		symbolname);
    }
    return RETVAL;
}

void dl_free(void *libhandle)
{
}
