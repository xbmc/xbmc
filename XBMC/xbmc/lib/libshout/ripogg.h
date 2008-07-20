/* ripogg.h
 *
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __RIP_OGG_H__
#define __RIP_OGG_H__

#include "list.h"

void rip_ogg_init (void);
void rip_ogg_get_current_header (unsigned char** ptr, int* len);
void rip_ogg_process_chunk (LIST* page_list, const char* buf, u_long size,
			    TRACK_INFO* ti);

#endif
