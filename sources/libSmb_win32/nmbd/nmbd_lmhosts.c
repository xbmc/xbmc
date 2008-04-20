/*
   Unix SMB/CIFS implementation.
   NBT netbios routines and daemon - version 2
   Copyright (C) Jeremy Allison 1994-1998

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

   Revision History:

   Handle lmhosts file reading.

*/

#include "includes.h"

/****************************************************************************
Load a lmhosts file.
****************************************************************************/

void load_lmhosts_file(char *fname)
{  
	pstring name;
	int name_type;
	struct in_addr ipaddr;
	XFILE *fp = startlmhosts( fname );

	if (!fp) {
		DEBUG(2,("load_lmhosts_file: Can't open lmhosts file %s. Error was %s\n",
			fname, strerror(errno)));
		return;
	}
   
	while (getlmhostsent(fp, name, &name_type, &ipaddr) ) {
		struct subnet_record *subrec = NULL;
		enum name_source source = LMHOSTS_NAME;

		/* We find a relevent subnet to put this entry on, then add it. */
		/* Go through all the broadcast subnets and see if the mask matches. */
		for (subrec = FIRST_SUBNET; subrec ; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
			if(same_net(ipaddr, subrec->bcast_ip, subrec->mask_ip))
				break;
		}
  
		/* If none match add the name to the remote_broadcast_subnet. */
		if(subrec == NULL)
			subrec = remote_broadcast_subnet;

		if(name_type == -1) {
			/* Add the (0) and (0x20) names directly into the namelist for this subnet. */
			(void)add_name_to_subnet(subrec,name,0x00,(uint16)NB_ACTIVE,PERMANENT_TTL,source,1,&ipaddr);
			(void)add_name_to_subnet(subrec,name,0x20,(uint16)NB_ACTIVE,PERMANENT_TTL,source,1,&ipaddr);
		} else {
			/* Add the given name type to the subnet namelist. */
			(void)add_name_to_subnet(subrec,name,name_type,(uint16)NB_ACTIVE,PERMANENT_TTL,source,1,&ipaddr);
		}
	}
   
	endlmhosts(fp);
}

/****************************************************************************
  Find a name read from the lmhosts file. We secretly check the names on
  the remote_broadcast_subnet as if the name was added to a regular broadcast
  subnet it will be found by normal name query processing.
****************************************************************************/

BOOL find_name_in_lmhosts(struct nmb_name *nmbname, struct name_record **namerecp)
{
	struct name_record *namerec;

	*namerecp = NULL;

	if((namerec = find_name_on_subnet(remote_broadcast_subnet, nmbname, FIND_ANY_NAME))==NULL)
		return False;

	if(!NAME_IS_ACTIVE(namerec) || (namerec->data.source != LMHOSTS_NAME))
		return False;

	*namerecp = namerec;
	return True;
}
