/* 
   Unix SMB/CIFS implementation.
   Username handling
   Copyright (C) Andrew Tridgell 1992-1998
   Copyright (C) Jeremy Allison 1997-2001.
   Copyright (C) Volker Lendecke 2006
   
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

/*******************************************************************
 Map a username from a dos name to a unix name by looking in the username
 map. Note that this modifies the name in place.
 This is the main function that should be called *once* on
 any incoming or new username - in order to canonicalize the name.
 This is being done to de-couple the case conversions from the user mapping
 function. Previously, the map_username was being called
 every time Get_Pwnam was called.
 Returns True if username was changed, false otherwise.
********************************************************************/

BOOL map_username(fstring user)
{
	static BOOL initialised=False;
	static fstring last_from,last_to;
	XFILE *f;
	char *mapfile = lp_username_map();
	char *s;
	pstring buf;
	BOOL mapped_user = False;
	char *cmd = lp_username_map_script();
	
	if (!*user)
		return False;
		
	if (strequal(user,last_to))
		return False;

	if (strequal(user,last_from)) {
		DEBUG(3,("Mapped user %s to %s\n",user,last_to));
		fstrcpy(user,last_to);
		return True;
	}
	
	/* first try the username map script */
	
	if ( *cmd ) {
		char **qlines;
		pstring command;
		int numlines, ret, fd;

		pstr_sprintf( command, "%s \"%s\"", cmd, user );

		DEBUG(10,("Running [%s]\n", command));
		ret = smbrun(command, &fd);
		DEBUGADD(10,("returned [%d]\n", ret));

		if ( ret != 0 ) {
			if (fd != -1)
				close(fd);
			return False;
		}

		numlines = 0;
		qlines = fd_lines_load(fd, &numlines,0);
		DEBUGADD(10,("Lines returned = [%d]\n", numlines));
		close(fd);

		/* should be either no lines or a single line with the mapped username */

		if (numlines && qlines) {
			DEBUG(3,("Mapped user %s to %s\n", user, qlines[0] ));
			fstrcpy( user, qlines[0] );
		}

		file_lines_free(qlines);
		
		return numlines != 0;
	}

	/* ok.  let's try the mapfile */
	
	if (!*mapfile)
		return False;

	if (!initialised) {
		*last_from = *last_to = 0;
		initialised = True;
	}
  
	f = x_fopen(mapfile,O_RDONLY, 0);
	if (!f) {
		DEBUG(0,("can't open username map %s. Error %s\n",mapfile, strerror(errno) ));
		return False;
	}

	DEBUG(4,("Scanning username map %s\n",mapfile));

	while((s=fgets_slash(buf,sizeof(buf),f))!=NULL) {
		char *unixname = s;
		char *dosname = strchr_m(unixname,'=');
		char **dosuserlist;
		BOOL return_if_mapped = False;

		if (!dosname)
			continue;

		*dosname++ = 0;

		while (isspace((int)*unixname))
			unixname++;

		if ('!' == *unixname) {
			return_if_mapped = True;
			unixname++;
			while (*unixname && isspace((int)*unixname))
				unixname++;
		}
    
		if (!*unixname || strchr_m("#;",*unixname))
			continue;

		{
			int l = strlen(unixname);
			while (l && isspace((int)unixname[l-1])) {
				unixname[l-1] = 0;
				l--;
			}
		}

		/* skip lines like 'user = ' */

		dosuserlist = str_list_make(dosname, NULL);
		if (!dosuserlist) {
			DEBUG(0,("Bad username map entry.  Unable to build user list.  Ignoring.\n"));
			continue;
		}

		if (strchr_m(dosname,'*') ||
		    user_in_list(user, (const char **)dosuserlist)) {
			DEBUG(3,("Mapped user %s to %s\n",user,unixname));
			mapped_user = True;
			fstrcpy( last_from,user );
			fstrcpy( user, unixname );
			fstrcpy( last_to,user );
			if ( return_if_mapped ) {
				str_list_free (&dosuserlist);
				x_fclose(f);
				return True;
			}
		}
    
		str_list_free (&dosuserlist);
	}

	x_fclose(f);

	/*
	 * Setup the last_from and last_to as an optimization so 
	 * that we don't scan the file again for the same user.
	 */
	fstrcpy(last_from,user);
	fstrcpy(last_to,user);

	return mapped_user;
}
