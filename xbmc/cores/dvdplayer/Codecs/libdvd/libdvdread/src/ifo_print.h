/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id: ifo_print.h 1135 2008-09-06 21:55:51Z rathann $
 *
 */

#ifndef IFO_PRINT_H_INCLUDED
#define IFO_PRINT_H_INCLUDED

#include <inttypes.h>
#include "ifo_types.h"

void ifo_print(dvd_reader_t *dvd, int title);
void dvdread_print_time(dvd_time_t *dtime);

#endif /* IFO_PRINT_H_INCLUDED */

/*
 * $Log$
 * Revision 1.1  2004/01/11 21:43:13  mroi
 * big build system changes
 *  * cleaned up all Makefiles and added a Makefile.common
 *  * added relchk script
 *  * moved libdvdread files to a dvdread subdir
 *  * moved DVD VM to a vm subdir
 *  * removed unused code in read_cache.c
 *
 * Revision 1.4  2004/01/01 15:13:13  jcdutton
 * Put ifo_print.c and .h back in.
 *
 * Revision 1.2  2003/04/28 15:17:17  jcdutton
 * Update ifodump to work with new libdvdnav cvs, instead of needing libdvdread.
 *
 * Revision 1.1.1.1  2002/08/28 09:48:35  jcdutton
 * Initial import into CVS.
 *
 *
 *
 */

