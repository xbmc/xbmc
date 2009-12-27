/*
  $Id: overlap.h,v 1.1 2004/12/18 17:29:32 rocky Exp $

  Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
  Copyright (C) 1998 Monty xiphmont@mit.edu
  
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

#ifndef _OVERLAP_H_
#define _OVERLAP_H_

extern void offset_add_value(cdrom_paranoia_t *p,offsets *o,long value,
			     void(*callback)(long int, paranoia_cb_mode_t));
extern void offset_clear_settings(offsets *o);
extern void offset_adjust_settings(cdrom_paranoia_t *p, 
				   void(*callback)(long, paranoia_cb_mode_t));
extern void i_paranoia_trim(cdrom_paranoia_t *p,long beginword,long endword);
extern void paranoia_resetall(cdrom_paranoia_t *p);
extern void paranoia_resetcache(cdrom_paranoia_t *p);

#endif /*_OVERLAP_H_*/
