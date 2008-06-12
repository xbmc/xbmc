/*
   Samba - Unix SMB/CIFS implementation
   Test harness for check_dos_char
   Copyright (C) Martin Pool 2003
   
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


/*
 * Just print out DOS validity or not for every character.
 *
 * DOS validity for a Unicode character set means that it can be
 * represented in DOS codepage, and that the DOS character maps back
 * to the same Unicode character.
 *
 * This depends on which DOS codepage is configured.
 */
 int main(void)
{
	smb_ucs2_t	i;

	for (i = 0; i < 0xffff; i++) {
		printf("%d %d\n", (int) i, (int) check_dos_char(i));
	}
	
	return 0;
}
