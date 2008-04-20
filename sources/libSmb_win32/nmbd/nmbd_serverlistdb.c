/* 
   Unix SMB/CIFS implementation.
   NBT netbios routines and daemon - version 2
   Copyright (C) Andrew Tridgell 1994-1998
   Copyright (C) Luke Kenneth Casson Leighton 1994-1998
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
   
*/

#include "includes.h"

int updatecount = 0;

/*******************************************************************
  Remove all the servers in a work group.
  ******************************************************************/

void remove_all_servers(struct work_record *work)
{
	struct server_record *servrec;
	struct server_record *nexts;

	for (servrec = work->serverlist; servrec; servrec = nexts) {
		DEBUG(7,("remove_all_servers: Removing server %s\n",servrec->serv.name));
		nexts = servrec->next;

		if (servrec->prev)
			servrec->prev->next = servrec->next;
		if (servrec->next)
			servrec->next->prev = servrec->prev;

		if (work->serverlist == servrec)
			work->serverlist = servrec->next;

		ZERO_STRUCTP(servrec);
		SAFE_FREE(servrec);
	}

	work->subnet->work_changed = True;
}

/***************************************************************************
  Add a server into the a workgroup serverlist.
  **************************************************************************/

static void add_server_to_workgroup(struct work_record *work,
                             struct server_record *servrec)
{
	struct server_record *servrec2;

	if (!work->serverlist) {
		work->serverlist = servrec;
		servrec->prev = NULL;
		servrec->next = NULL;
		return;
	}

	for (servrec2 = work->serverlist; servrec2->next; servrec2 = servrec2->next)
		;

	servrec2->next = servrec;
	servrec->next = NULL;
	servrec->prev = servrec2;
	work->subnet->work_changed = True;
}

/****************************************************************************
  Find a server in a server list.
  **************************************************************************/

struct server_record *find_server_in_workgroup(struct work_record *work, const char *name)
{
	struct server_record *ret;
  
	for (ret = work->serverlist; ret; ret = ret->next) {
		if (strequal(ret->serv.name,name))
			return ret;
	}
	return NULL;
}


/****************************************************************************
  Remove a server entry from this workgroup.
  ****************************************************************************/

void remove_server_from_workgroup(struct work_record *work, struct server_record *servrec)
{
	if (servrec->prev)
		servrec->prev->next = servrec->next;
	if (servrec->next)
		servrec->next->prev = servrec->prev;

	if (work->serverlist == servrec) 
		work->serverlist = servrec->next; 

	ZERO_STRUCTP(servrec);
	SAFE_FREE(servrec);
	work->subnet->work_changed = True;
}

/****************************************************************************
  Create a server entry on this workgroup.
  ****************************************************************************/

struct server_record *create_server_on_workgroup(struct work_record *work,
                                                 const char *name,int servertype, 
                                                 int ttl, const char *comment)
{
	struct server_record *servrec;
  
	if (name[0] == '*') {
		DEBUG(7,("create_server_on_workgroup: not adding name starting with '*' (%s)\n",
			name));
		return (NULL);
	}
  
	if((servrec = find_server_in_workgroup(work, name)) != NULL) {
		DEBUG(0,("create_server_on_workgroup: Server %s already exists on \
workgroup %s. This is a bug.\n", name, work->work_group));
		return NULL;
	}
  
	if((servrec = SMB_MALLOC_P(struct server_record)) == NULL) {
		DEBUG(0,("create_server_entry_on_workgroup: malloc fail !\n"));
		return NULL;
	}

	memset((char *)servrec,'\0',sizeof(*servrec));
 
	servrec->subnet = work->subnet;
 
	fstrcpy(servrec->serv.name,name);
	fstrcpy(servrec->serv.comment,comment);
	strupper_m(servrec->serv.name);
	servrec->serv.type  = servertype;

	update_server_ttl(servrec, ttl);
  
	add_server_to_workgroup(work, servrec);
      
	DEBUG(3,("create_server_on_workgroup: Created server entry %s of type %x (%s) on \
workgroup %s.\n", name,servertype,comment, work->work_group));
 
	work->subnet->work_changed = True;
 
	return(servrec);
}

/*******************************************************************
 Update the ttl field of a server record.
*******************************************************************/

void update_server_ttl(struct server_record *servrec, int ttl)
{
	if(ttl > lp_max_ttl())
		ttl = lp_max_ttl();

	if(is_myname(servrec->serv.name))
		servrec->death_time = PERMANENT_TTL;
	else
		servrec->death_time = (ttl != PERMANENT_TTL) ? time(NULL)+(ttl*3) : PERMANENT_TTL;

	servrec->subnet->work_changed = True;
}

/*******************************************************************
  Expire old servers in the serverlist. A time of -1 indicates 
  everybody dies except those with a death_time of PERMANENT_TTL (which is 0).
  This should only be called from expire_workgroups_and_servers().
  ******************************************************************/

void expire_servers(struct work_record *work, time_t t)
{
	struct server_record *servrec;
	struct server_record *nexts;
  
	for (servrec = work->serverlist; servrec; servrec = nexts) {
		nexts = servrec->next;

		if ((servrec->death_time != PERMANENT_TTL) && ((t == -1) || (servrec->death_time < t))) {
			DEBUG(3,("expire_old_servers: Removing timed out server %s\n",servrec->serv.name));
			remove_server_from_workgroup(work, servrec);
			work->subnet->work_changed = True;
		}
	}
}

/*******************************************************************
 Decide if we should write out a server record for this server.
 We return zero if we should not. Check if we've already written
 out this server record from an earlier subnet.
******************************************************************/

static uint32 write_this_server_name( struct subnet_record *subrec,
                                      struct work_record *work,
                                      struct server_record *servrec)
{
	struct subnet_record *ssub;
	struct work_record *iwork;

	/* Go through all the subnets we have already seen. */
	for (ssub = FIRST_SUBNET; ssub && (ssub != subrec); ssub = NEXT_SUBNET_INCLUDING_UNICAST(ssub)) {
		for(iwork = ssub->workgrouplist; iwork; iwork = iwork->next) {
			if(find_server_in_workgroup( iwork, servrec->serv.name) != NULL) {
				/*
				 * We have already written out this server record, don't
				 * do it again. This gives precedence to servers we have seen
				 * on the broadcast subnets over servers that may have been
				 * added via a sync on the unicast_subet.
				 *
				 * The correct way to do this is to have a serverlist file
				 * per subnet - this means changes to smbd as well. I may
				 * add this at a later date (JRA).
				 */

				return 0;
			}
		}
	}

	return servrec->serv.type;
}

/*******************************************************************
 Decide if we should write out a workgroup record for this workgroup.
 We return zero if we should not. Don't write out lp_workgroup() (we've
 already done it) and also don't write out a second workgroup record
 on the unicast subnet that we've already written out on one of the
 broadcast subnets.
******************************************************************/

static uint32 write_this_workgroup_name( struct subnet_record *subrec, 
                                         struct work_record *work)
{
	struct subnet_record *ssub;

	if(strequal(lp_workgroup(), work->work_group))
		return 0;

	/* This is a workgroup we have seen on a broadcast subnet. All
		these have the same type. */

	if(subrec != unicast_subnet)
		return (SV_TYPE_DOMAIN_ENUM|SV_TYPE_NT|SV_TYPE_LOCAL_LIST_ONLY);

	for(ssub = FIRST_SUBNET; ssub;  ssub = NEXT_SUBNET_EXCLUDING_UNICAST(ssub)) {
		/* This is the unicast subnet so check if we've already written out
			this subnet when we passed over the broadcast subnets. */

		if(find_workgroup_on_subnet( ssub, work->work_group) != NULL)
			return 0;
	}

	/* All workgroups on the unicast subnet (except our own, which we
		have already written out) cannot be local. */

	return (SV_TYPE_DOMAIN_ENUM|SV_TYPE_NT);
}

/*******************************************************************
  Write out the browse.dat file.
  ******************************************************************/

void write_browse_list_entry(XFILE *fp, const char *name, uint32 rec_type,
		const char *local_master_browser_name, const char *description)
{
	fstring tmp;

	slprintf(tmp,sizeof(tmp)-1, "\"%s\"", name);
	x_fprintf(fp, "%-25s ", tmp);
	x_fprintf(fp, "%08x ", rec_type);
	slprintf(tmp, sizeof(tmp)-1, "\"%s\" ", local_master_browser_name);
	x_fprintf(fp, "%-30s", tmp);
	x_fprintf(fp, "\"%s\"\n", description);
}

void write_browse_list(time_t t, BOOL force_write)
{   
	struct subnet_record *subrec;
	struct work_record *work;
	struct server_record *servrec;
	pstring fname,fnamenew;
	uint32 stype;
	int i;
	XFILE *fp;
	BOOL list_changed = force_write;
	static time_t lasttime = 0;
    
	/* Always dump if we're being told to by a signal. */
	if(force_write == False) {
		if (!lasttime)
			lasttime = t;
		if (t - lasttime < 5)
			return;
	}

	lasttime = t;

	dump_workgroups(force_write);
 
	for (subrec = FIRST_SUBNET; subrec ; subrec = NEXT_SUBNET_INCLUDING_UNICAST(subrec)) {
		if(subrec->work_changed) {
			list_changed = True;
			break;
		}
	}

	if(!list_changed)
		return;

	updatecount++;
    
	pstrcpy(fname,lp_lockdir());
	trim_char(fname,'\0' ,'/');
	pstrcat(fname,"/");
	pstrcat(fname,SERVER_LIST);
	pstrcpy(fnamenew,fname);
	pstrcat(fnamenew,".");
 
	fp = x_fopen(fnamenew,O_WRONLY|O_CREAT|O_TRUNC, 0644);
 
	if (!fp) {
		DEBUG(0,("write_browse_list: Can't open file %s. Error was %s\n",
			fnamenew,strerror(errno)));
		return;
	} 
  
	/*
	 * Write out a record for our workgroup. Use the record from the first
	 * subnet.
	 */

	if((work = find_workgroup_on_subnet(FIRST_SUBNET, lp_workgroup())) == NULL) { 
		DEBUG(0,("write_browse_list: Fatal error - cannot find my workgroup %s\n",
			lp_workgroup()));
		x_fclose(fp);
		return;
	}

	write_browse_list_entry(fp, work->work_group,
		SV_TYPE_DOMAIN_ENUM|SV_TYPE_NT|SV_TYPE_LOCAL_LIST_ONLY,
		work->local_master_browser_name, work->work_group);

	/* 
	 * We need to do something special for our own names.
	 * This is due to the fact that we may be a local master browser on
	 * one of our broadcast subnets, and a domain master on the unicast
	 * subnet. We iterate over the subnets and only write out the name
	 * once.
	 */

	for (i=0; my_netbios_names(i); i++) {
		stype = 0;
		for (subrec = FIRST_SUBNET; subrec ; subrec = NEXT_SUBNET_INCLUDING_UNICAST(subrec)) {
			if((work = find_workgroup_on_subnet( subrec, lp_workgroup() )) == NULL)
				continue;
			if((servrec = find_server_in_workgroup( work, my_netbios_names(i))) == NULL)
				continue;

			stype |= servrec->serv.type;
		}

		/* Output server details, plus what workgroup they're in. */
		write_browse_list_entry(fp, my_netbios_names(i), stype,
			string_truncate(lp_serverstring(), MAX_SERVER_STRING_LENGTH), lp_workgroup());
	}
      
	for (subrec = FIRST_SUBNET; subrec ; subrec = NEXT_SUBNET_INCLUDING_UNICAST(subrec)) { 
		subrec->work_changed = False;

		for (work = subrec->workgrouplist; work ; work = work->next) {
			/* Write out a workgroup record for a workgroup. */
			uint32 wg_type = write_this_workgroup_name( subrec, work);

			if(wg_type) {
				write_browse_list_entry(fp, work->work_group, wg_type,
						work->local_master_browser_name,
						work->work_group);
			}

			/* Now write out any server records a workgroup may have. */

			for (servrec = work->serverlist; servrec ; servrec = servrec->next) {
				uint32 serv_type;

				/* We have already written our names here. */
				if(is_myname(servrec->serv.name))
					continue; 

				serv_type = write_this_server_name(subrec, work, servrec);
				if(serv_type) {
					/* Output server details, plus what workgroup they're in. */
					write_browse_list_entry(fp, servrec->serv.name, serv_type,
						servrec->serv.comment, work->work_group);
				}
			}
		}
	} 
  
	x_fclose(fp);
	unlink(fname);
	chmod(fnamenew,0644);
	rename(fnamenew,fname);
	DEBUG(3,("write_browse_list: Wrote browse list into file %s\n",fname));
}
