/* findsep.h
 * library routines for find silent points in mp3 data
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
#ifndef __FINDSEP_H__
#define __FINDSEP_H__

#include "srtypes.h"

error_code
findsep_silence (const u_char* mpgbuf, 
		 long mpgsize, 
		 long len_to_sw,
		 long searchwindow,
		 long silence_length, 
		 long padding1,
		 long padding2,
		 u_long* pos1, 
		 u_long* pos2
		 );
error_code find_bitrate(unsigned long* bitrate, const u_char* mpgbuf, 
			long mpgsize);

#endif //__FINDSEP_H__
