/* 
   Unix SMB/CIFS implementation.
   Samba utility functions
   Copyright (C) Martin Pool     2003
   Copyright (C) Andrew Bartlett 2003
   
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

#ifdef DEVELOPER
const char *global_clobber_region_function;
unsigned int global_clobber_region_line;
#endif

/**
 * In developer builds, clobber a region of memory.
 *
 * If we think a string buffer is longer than it really is, this ought
 * to make the failure obvious, by segfaulting (if in the heap) or by
 * killing the return address (on the stack), or by trapping under a
 * memory debugger.
 *
 * This is meant to catch possible string overflows, even if the
 * actual string copied is not big enough to cause an overflow.
 *
 * In addition, under Valgrind the buffer is marked as uninitialized.
 **/
void clobber_region(const char *fn, unsigned int line, char *dest, size_t len)
{
#ifdef DEVELOPER
	global_clobber_region_function = fn;
	global_clobber_region_line = line;

	/* F1 is odd and 0xf1f1f1f1 shouldn't be a valid pointer */
	memset(dest, 0xF1, len);
#ifdef VALGRIND
	/* Even though we just wrote to this, from the application's
	 * point of view it is not initialized.
	 *
	 * (This is not redundant with the clobbering above.  The
	 * marking might not actually take effect if we're not running
	 * under valgrind.) */
	VALGRIND_MAKE_WRITABLE(dest, len);
#endif /* VALGRIND */
#endif /* DEVELOPER */
}
