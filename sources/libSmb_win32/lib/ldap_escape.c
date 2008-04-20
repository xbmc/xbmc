/* 
   Unix SMB/CIFS implementation.
   ldap filter argument escaping

   Copyright (C) 1998, 1999, 2000 Luke Howard <lukeh@padl.com>,
   Copyright (C) 2003 Andrew Bartlett <abartlet@samba.org>

  
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

/**
 * Escape a parameter to an LDAP filter string, so they cannot contain
 * embeded ( ) * or \ chars which may cause it not to parse correctly. 
 *
 * @param s The input string
 *
 * @return A string allocated with malloc(), containing the escaped string, 
 * and to be free()ed by the caller.
 **/

char *escape_ldap_string_alloc(const char *s)
{
	size_t len = strlen(s)+1;
	char *output = SMB_MALLOC(len);
	const char *sub;
	int i = 0;
	char *p = output;

	if (output == NULL) {
		return NULL;
	}
	
	while (*s)
	{
		switch (*s)
		{
		case '*':
			sub = "\\2a";
			break;
		case '(':
			sub = "\\28";
			break;
		case ')':
			sub = "\\29";
			break;
		case '\\':
			sub = "\\5c";
			break;
		default:
			sub = NULL;
			break;
		}
		
		if (sub) {
			len = len + 3;
			output = SMB_REALLOC(output, len);
			if (!output) { 
				return NULL;
			}
			
			p = &output[i];
			strncpy (p, sub, 3);
			p += 3;
			i += 3;

		} else {
			*p = *s;
			p++;
			i++;
		}
		s++;
	}
	
	*p = '\0';
	return output;
}
