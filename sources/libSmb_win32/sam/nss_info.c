/* 
   Unix SMB/CIFS implementation.
   nss info helpers
   Copyright (C) Guenther Deschner 2006

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.*/

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_IDMAP

static enum wb_posix_mapping wb_posix_map_type(const char *map_str)
{
	if (strequal(map_str, "template")) 
		return WB_POSIX_MAP_TEMPLATE;
	else if (strequal(map_str, "sfu"))
		return WB_POSIX_MAP_SFU;
	else if (strequal(map_str, "rfc2307"))
		return WB_POSIX_MAP_RFC2307;
	else if (strequal(map_str, "unixinfo"))
		return WB_POSIX_MAP_UNIXINFO;
	
	return WB_POSIX_MAP_UNKNOWN;
}

/* winbind nss info = rfc2307 SO36:sfu FHAIN:rfc2307 PANKOW:template
 *
 * syntax is:
 *	1st param: default setting
 *	following ":" separated list elements:
 *		DOMAIN:setting
 *	setting can be one of "sfu", "rfc2307", "template", "unixinfo"
 */

enum wb_posix_mapping get_nss_info(const char *domain_name)
{
	const char **list = lp_winbind_nss_info();
	enum wb_posix_mapping map_templ = WB_POSIX_MAP_TEMPLATE;
	int i;

	DEBUG(11,("get_nss_info for %s\n", domain_name));

	if (!lp_winbind_nss_info() || !*lp_winbind_nss_info()) {
		return WB_POSIX_MAP_TEMPLATE;
	}

	if ((map_templ = wb_posix_map_type(list[0])) == WB_POSIX_MAP_UNKNOWN) {
		DEBUG(0,("get_nss_info: invalid setting: %s\n", list[0]));
		return WB_POSIX_MAP_TEMPLATE;
	}

	DEBUG(11,("get_nss_info: using \"%s\" by default\n", list[0]));

	for (i=0; list[i]; i++) {

		const char *p = list[i];
		fstring tok;

		if (!next_token(&p, tok, ":", sizeof(tok))) {
			DEBUG(0,("get_nss_info: no \":\" delimitier found\n"));
			continue;
		}

		if (strequal(tok, domain_name)) {
		
			enum wb_posix_mapping type;
			
			if ((type = wb_posix_map_type(p)) == WB_POSIX_MAP_UNKNOWN) {
				DEBUG(0,("get_nss_info: invalid setting: %s\n", p));
				/* return WB_POSIX_MAP_TEMPLATE; */
				continue;
			}

			DEBUG(11,("get_nss_info: using \"%s\" for domain: %s\n", p, tok));
			
			return type;
		}
	}

	return map_templ;
}

const char *wb_posix_map_str(enum wb_posix_mapping mtype)
{
	switch (mtype) {
		case WB_POSIX_MAP_TEMPLATE:
			return "template";
		case WB_POSIX_MAP_SFU:
			return "sfu";
		case WB_POSIX_MAP_RFC2307:
			return "rfc2307";
		case WB_POSIX_MAP_UNIXINFO:
			return "unixinfo";
		default:
			break;
	}
	return NULL;
}

enum wb_posix_mapping wb_posix_map_type(const char *map_str)
{
	if (strequal(map_str, "template")) 
		return WB_POSIX_MAP_TEMPLATE;
	else if (strequal(map_str, "sfu"))
		return WB_POSIX_MAP_SFU;
	else if (strequal(map_str, "rfc2307"))
		return WB_POSIX_MAP_RFC2307;
	else if (strequal(map_str, "unixinfo"))
		return WB_POSIX_MAP_UNIXINFO;
	
	return -1;
}
