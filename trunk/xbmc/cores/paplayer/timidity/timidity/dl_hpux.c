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
#ifdef __hp9000s300
#define magic hpux_magic
#define MAGIC HPUX_MAGIC
#endif

#include <stdlib.h>
#include <dl.h>
#ifdef __hp9000s300
#undef magic
#undef MAGIC
#endif

#include "timidity.h"
#include "dlutils.h"

/*ARGSUSED*/
void dl_init(int argc, char **argv)
{
}

void *dl_load_file(char *filename)
{
    shl_t obj = NULL;
    int	bind_type;

    bind_type = BIND_DEFERRED;
    obj = shl_load(filename, bind_type | BIND_NOSTART, 0L);
    if(obj == NULL)
    {
	perror(filename);
	return NULL;
    }
    return (void *)obj;
}

void *dl_find_symbol(void *libhandle, char *symbolname)
{
    shl_t obj = (shl_t) libhandle;
    void *symaddr = NULL;
    int status;
#ifdef __hp9000s300
    char buff[BUFSIZ];
    sprintf(buff, "_%s", symbolname);
    symbolname = buff;
#endif

    errno = 0;
    status = shl_findsym(&obj, symbolname, TYPE_PROCEDURE, &symaddr);

    if (status == -1 && errno == 0) {	/* try TYPE_DATA instead */
	status = shl_findsym(&obj, symbolname, TYPE_DATA, &symaddr);
    }

    if(status == -1)
    {
 	fprintf(stderr, "%s\n", errno ? strerror(errno) : "Symbol not found.");
	return NULL;
    }
    return symaddr;
}

void dl_free(void *libhandle)
{
}
