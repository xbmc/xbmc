/* 
   Unix SMB/CIFS implementation.
   server specific string routines
   Copyright (C) Andrew Tridgell 2001
   Copyright (C) Andrew Bartlett <abartlet@samba.org> 2003
   
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

#include "includes.h"
extern int max_send;

/* Make sure we can't write a string past the end of the buffer */

size_t srvstr_push_fn(const char *function, unsigned int line, 
		      const char *base_ptr, void *dest, 
		      const char *src, int dest_len, int flags)
{
	size_t buf_used = PTR_DIFF(dest, base_ptr);
	if (dest_len == -1) {
		if (((ptrdiff_t)dest < (ptrdiff_t)base_ptr) || (buf_used > (size_t)max_send)) {
#if 0
			DEBUG(0, ("Pushing string of 'unlimited' length into non-SMB buffer!\n"));
#endif
			return push_string_fn(function, line, base_ptr, dest, src, -1, flags);
		}
		return push_string_fn(function, line, base_ptr, dest, src, max_send - buf_used, flags);
	}
	
	/* 'normal' push into size-specified buffer */
	return push_string_fn(function, line, base_ptr, dest, src, dest_len, flags);
}
