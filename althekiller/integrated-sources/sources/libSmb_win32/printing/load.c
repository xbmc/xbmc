/* 
   Unix SMB/CIFS implementation.
   load printer lists
   Copyright (C) Andrew Tridgell 1992-2000
   
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


/***************************************************************************
auto-load some homes and printer services
***************************************************************************/
static void add_auto_printers(void)
{
	const char *p;
	int pnum = lp_servicenumber(PRINTERS_NAME);
	char *str;

	if (pnum < 0)
		return;

	if ((str = SMB_STRDUP(lp_auto_services())) == NULL)
		return;

	for (p = strtok(str, LIST_SEP); p; p = strtok(NULL, LIST_SEP)) {
		if (lp_servicenumber(p) >= 0)
			continue;
		
		if (pcap_printername_ok(p))
			lp_add_printer(p, pnum);
	}

	SAFE_FREE(str);
}

/***************************************************************************
load automatic printer services
***************************************************************************/
void load_printers(void)
{
	if (!pcap_cache_loaded())
		pcap_cache_reload();

	add_auto_printers();

	/* load all printcap printers */
	if (lp_load_printers() && lp_servicenumber(PRINTERS_NAME) >= 0)
		pcap_printer_fn(lp_add_one_printer);
}
