/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2001 Masanao Izumo <mo@goice.co.jp>
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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
/*
  dl_dyld.c
  To use dyld in Mac OS X / Darwin system
  by Urabe Shyouhei<mput@mac.com>
*/
#include <stdio.h>
#include <mach-o/dyld.h>
#include <unistd.h>
#include "timidity.h"
#include "dlutils.h"

void dl_init(int argc,char** argv)
{
// do nothing.
}

void* dl_load_file(char* path)
{
    NSObjectFileImage obj_file; /* file handler */
    
    if (NSCreateObjectFileImageFromFile(path, &obj_file) != NSObjectFileImageSuccess) {
	fprintf(stderr,"dl_load_file:Failed to load %.200s\n", path);
    }
    return NSLinkModule(obj_file,path,TRUE);
}

void* dl_find_symbol(void* libhandle, char* symbol)
{
    
    /* avoid a bug of how to treat '_'. */
    char buf[BUFSIZ];
    sprintf(buf,"_%s",symbol);
    
    if(NSIsSymbolNameDefined(symbol)) {
	fprintf(stderr,"dl_find_symbol:Failed to find %.200s\n",symbol);
    }
    return NSAddressOfSymbol(NSLookupAndBindSymbol(buf));
}

void dl_free(void *libhandle)
{
    /* sorry but I honestly don't know how to free dynamic library.
     * Someone please implement it. */
}
