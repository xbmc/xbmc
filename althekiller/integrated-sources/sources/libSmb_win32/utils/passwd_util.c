/* 
   Unix SMB/CIFS implementation.
   passdb editing frontend

   Copyright (C) Jeremy Allison  1998
   Copyright (C) Andrew Tridgell 1998
   Copyright (C) Tim Potter      2000
   Copyright (C) Simo Sorce      2000
   Copyright (C) Martin Pool     2001
   Copyright (C) Gerald Carter   2002
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

/*************************************************************
 Utility function to prompt for passwords from stdin. Each
 password entered must end with a newline.
*************************************************************/
char *stdin_new_passwd( void)
{
	static fstring new_pw;
	size_t len;

	ZERO_ARRAY(new_pw);

	/*
	 * if no error is reported from fgets() and string at least contains
	 * the newline that ends the password, then replace the newline with
	 * a null terminator.
	 */
	if ( fgets(new_pw, sizeof(new_pw), stdin) != NULL) {
		if ((len = strlen(new_pw)) > 0) {
			if(new_pw[len-1] == '\n')
				new_pw[len - 1] = 0;
		}
	}
	return(new_pw);
}

/*************************************************************
 Utility function to get passwords via tty or stdin
 Used if the '-s' (smbpasswd) or '-t' (pdbedit) option is set
 to silently get passwords to enable scripting.
*************************************************************/
char *get_pass( const char *prompt, BOOL stdin_get)
{
	char *p;
	if (stdin_get) {
		p = stdin_new_passwd();
	} else {
		p = getpass( prompt);
	}
	return smb_xstrdup( p);
}
