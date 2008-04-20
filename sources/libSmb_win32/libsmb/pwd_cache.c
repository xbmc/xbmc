/* 
   Unix SMB/CIFS implementation.
   Password cacheing.  obfuscation is planned
   Copyright (C) Luke Kenneth Casson Leighton 1996-1998
   
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

/****************************************************************************
 Initialises a password structure.
****************************************************************************/

static void pwd_init(struct pwd_info *pwd)
{
	memset((char *)pwd->password  , '\0', sizeof(pwd->password  ));

	pwd->null_pwd  = True; /* safest option... */
}

/****************************************************************************
 Stores a cleartext password.
****************************************************************************/

void pwd_set_cleartext(struct pwd_info *pwd, const char *clr)
{
	pwd_init(pwd);
	if (clr) {
		fstrcpy(pwd->password, clr);
		pwd->null_pwd = False;
	} else {
		pwd->null_pwd = True;
	}

	pwd->cleartext = True;
}

/****************************************************************************
 Gets a cleartext password.
****************************************************************************/

void pwd_get_cleartext(struct pwd_info *pwd, fstring clr)
{
	if (pwd->cleartext)
		fstrcpy(clr, pwd->password);
	else
		clr[0] = 0;

}
