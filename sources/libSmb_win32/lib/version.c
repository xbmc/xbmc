/* 
   Unix SMB/CIFS implementation.
   Samba Version functions
   
   Copyright (C) Stefan Metzmacher	2003
   
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

const char *samba_version_string(void)
{
#ifndef SAMBA_VERSION_VENDOR_SUFFIX
	return SAMBA_VERSION_OFFICIAL_STRING;
#else
	static fstring samba_version;
	fstring tmp_version;
	static BOOL init_samba_version;
	size_t remaining;

	if (init_samba_version)
		return samba_version;

	snprintf(samba_version,sizeof(samba_version),"%s-%s",
		SAMBA_VERSION_OFFICIAL_STRING,
		SAMBA_VERSION_VENDOR_SUFFIX);

#ifdef SAMBA_VENDOR_PATCH
	remaining = sizeof(samba_version)-strlen(samba_version);
	snprintf( tmp_version, sizeof(tmp_version),  "-%d", SAMBA_VENDOR_PATCH );
	strlcat( samba_version, tmp_version, remaining-1 );
#endif

	init_samba_version = True;
	return samba_version;
#endif
}
