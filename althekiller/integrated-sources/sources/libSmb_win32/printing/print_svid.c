/*
 * Copyright (C) 1997-1998 by Norm Jacobs, Colorado Springs, Colorado, USA
 * Copyright (C) 1997-1998 by Sun Microsystem, Inc.
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * This module implements support for gathering and comparing available
 * printer information on a SVID or XPG4 compliant system.  It does this
 * through the use of the SVID/XPG4 command "lpstat(1)".
 *
 * The expectations is that execution of the command "lpstat -v" will
 * generate responses in the form of:
 *
 *	device for serial: /dev/term/b
 *	system for fax: server
 *	system for color: server (as printer chroma)
 */


#include "includes.h"

#if defined(SYSV) || defined(HPUX)
BOOL sysv_cache_reload(void)
{
	char **lines;
	int i;

#if defined(HPUX)
	DEBUG(5, ("reloading hpux printcap cache\n"));
#else
	DEBUG(5, ("reloading sysv printcap cache\n"));
#endif

	if ((lines = file_lines_pload("/usr/bin/lpstat -v", NULL)) == NULL)
	{
#if defined(HPUX)
      
       	       /*
		* if "lpstat -v" is NULL then we check if schedular is running if it is
		* that means no printers are added on the HP-UX system, if schedular is not
		* running we display reload error.
		*/

		char **scheduler;
                scheduler = file_lines_pload("/usr/bin/lpstat -r", NULL);
                if(!strcmp(*scheduler,"scheduler is running")){
                        DEBUG(3,("No Printers found!!!\n"));
			file_lines_free(scheduler);
                        return True;
                }
                else{
                        DEBUG(3,("Scheduler is not running!!!\n"));
			file_lines_free(scheduler);
			return False;
		}
#else
		DEBUG(3,("No Printers found!!!\n"));
		return False;
#endif
	}

	for (i = 0; lines[i]; i++) {
		char *name, *tmp;
		char *buf = lines[i];

		/* eat "system/device for " */
		if (((tmp = strchr_m(buf, ' ')) == NULL) ||
		    ((tmp = strchr_m(++tmp, ' ')) == NULL))
			continue;

		/*
		 * In case we're only at the "for ".
		 */

		if(!strncmp("for ", ++tmp, 4)) {
			tmp=strchr_m(tmp, ' ');
			tmp++;
		}

		/* Eat whitespace. */

		while(*tmp == ' ')
			++tmp;

		/*
		 * On HPUX there is an extra line that can be ignored.
		 * d.thibadeau 2001/08/09
		 */
		if(!strncmp("remote to", tmp, 9))
			continue;

		name = tmp;

		/* truncate the ": ..." */
		if ((tmp = strchr_m(name, ':')) != NULL)
			*tmp = '\0';
		
		/* add it to the cache */
		if (!pcap_cache_add(name, NULL)) {
			file_lines_free(lines);
			return False;
		}
	}

	file_lines_free(lines);
	return True;
}

#else
/* this keeps fussy compilers happy */
 void print_svid_dummy(void);
 void print_svid_dummy(void) {}
#endif
