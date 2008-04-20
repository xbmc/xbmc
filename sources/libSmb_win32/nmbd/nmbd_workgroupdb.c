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

extern uint16 samba_nb_type;

int workgroup_count = 0; /* unique index key: one for each workgroup */

/****************************************************************************
  Add a workgroup into the list.
**************************************************************************/

static void add_workgroup(struct subnet_record *subrec, struct work_record *work)
{
	work->subnet = subrec;
	DLIST_ADD(subrec->workgrouplist, work);
	subrec->work_changed = True;
}

/****************************************************************************
 Copy name to unstring. Used by create_workgroup() and find_workgroup_on_subnet().
**************************************************************************/

static void name_to_unstring(unstring unname, const char *name)
{
        nstring nname;

	errno = 0;
	push_ascii_nstring(nname, name);
	if (errno == E2BIG) {
		unstring tname;
		pull_ascii_nstring(tname, sizeof(tname), nname);
		unstrcpy(unname, tname);
		DEBUG(0,("name_to_nstring: workgroup name %s is too long. Truncating to %s\n",
			name, tname));
	} else {
		unstrcpy(unname, name);
	}
}
		
/****************************************************************************
  Create an empty workgroup.
**************************************************************************/

static struct work_record *create_workgroup(const char *name, int ttl)
{
	struct work_record *work;
	struct subnet_record *subrec;
	int t = -1;
  
	if((work = SMB_MALLOC_P(struct work_record)) == NULL) {
		DEBUG(0,("create_workgroup: malloc fail !\n"));
		return NULL;
	}
	memset((char *)work, '\0', sizeof(*work));

	name_to_unstring(work->work_group, name);

	work->serverlist = NULL;
  
	work->RunningElection = False;
	work->ElectionCount = 0;
	work->announce_interval = 0;
	work->needelection = False;
	work->needannounce = True;
	work->lastannounce_time = time(NULL);
	work->mst_state = lp_local_master() ? MST_POTENTIAL : MST_NONE;
	work->dom_state = DOMAIN_NONE;
	work->log_state = LOGON_NONE;
  
	work->death_time = (ttl != PERMANENT_TTL) ? time(NULL)+(ttl*3) : PERMANENT_TTL;

	/* Make sure all token representations of workgroups are unique. */
  
	for (subrec = FIRST_SUBNET; subrec && (t == -1); subrec = NEXT_SUBNET_INCLUDING_UNICAST(subrec)) {
		struct work_record *w;
		for (w = subrec->workgrouplist; w && t == -1; w = w->next) {
			if (strequal(w->work_group, work->work_group))
				t = w->token;
		}
	}
  
	if (t == -1)
		work->token = ++workgroup_count;
	else
		work->token = t;
  
	/* No known local master browser as yet. */
	*work->local_master_browser_name = '\0';

	/* No known domain master browser as yet. */
	*work->dmb_name.name = '\0';
	zero_ip(&work->dmb_addr);

	/* WfWg  uses 01040b01 */
	/* Win95 uses 01041501 */
	/* NTAS  uses ???????? */
	work->ElectionCriterion  = (MAINTAIN_LIST)|(BROWSER_ELECTION_VERSION<<8); 
	work->ElectionCriterion |= (lp_os_level() << 24);
	if (lp_domain_master())
		work->ElectionCriterion |= 0x80;
  
	return work;
}

/*******************************************************************
  Remove a workgroup.
******************************************************************/

static struct work_record *remove_workgroup_from_subnet(struct subnet_record *subrec, 
                                     struct work_record *work)
{
	struct work_record *ret_work = NULL;
  
	DEBUG(3,("remove_workgroup: Removing workgroup %s\n", work->work_group));
  
	ret_work = work->next;

	remove_all_servers(work);
  
	if (!work->serverlist) {
		if (work->prev)
			work->prev->next = work->next;
		if (work->next)
			work->next->prev = work->prev;
  
		if (subrec->workgrouplist == work)
			subrec->workgrouplist = work->next; 
  
		ZERO_STRUCTP(work);
		SAFE_FREE(work);
	}
  
	subrec->work_changed = True;

	return ret_work;
}

/****************************************************************************
  Find a workgroup in the workgroup list of a subnet.
**************************************************************************/

struct work_record *find_workgroup_on_subnet(struct subnet_record *subrec, 
                                             const char *name)
{
	struct work_record *ret;
 	unstring un_name;
 
	DEBUG(4, ("find_workgroup_on_subnet: workgroup search for %s on subnet %s: ",
		name, subrec->subnet_name));
  
	name_to_unstring(un_name, name);

	for (ret = subrec->workgrouplist; ret; ret = ret->next) {
		if (strequal(ret->work_group,un_name)) {
			DEBUGADD(4, ("found.\n"));
			return(ret);
		}
	}
	DEBUGADD(4, ("not found.\n"));
	return NULL;
}

/****************************************************************************
  Create a workgroup in the workgroup list of the subnet.
**************************************************************************/

struct work_record *create_workgroup_on_subnet(struct subnet_record *subrec,
                                               const char *name, int ttl)
{
	struct work_record *work = NULL;

	DEBUG(4,("create_workgroup_on_subnet: creating group %s on subnet %s\n",
		name, subrec->subnet_name));
  
	if ((work = create_workgroup(name, ttl))) {
		add_workgroup(subrec, work);
		subrec->work_changed = True;
		return(work);
	}

	return NULL;
}

/****************************************************************************
  Update a workgroup ttl.
**************************************************************************/

void update_workgroup_ttl(struct work_record *work, int ttl)
{
	if(work->death_time != PERMANENT_TTL)
		work->death_time = time(NULL)+(ttl*3);
	work->subnet->work_changed = True;
}

/****************************************************************************
 Fail function called if we cannot register the WORKGROUP<0> and
 WORKGROUP<1e> names on the net.
**************************************************************************/
     
static void fail_register(struct subnet_record *subrec, struct response_record *rrec,
                          struct nmb_name *nmbname)
{  
	DEBUG(0,("fail_register: Failed to register name %s on subnet %s.\n",
		nmb_namestr(nmbname), subrec->subnet_name));
}  

/****************************************************************************
 If the workgroup is our primary workgroup, add the required names to it.
**************************************************************************/

void initiate_myworkgroup_startup(struct subnet_record *subrec, struct work_record *work)
{
	int i;

	if(!strequal(lp_workgroup(), work->work_group))
		return;

	/* If this is a broadcast subnet then start elections on it if we are so configured. */

	if ((subrec != unicast_subnet) && (subrec != remote_broadcast_subnet) &&
			(subrec != wins_server_subnet) && lp_preferred_master() && lp_local_master()) {
		DEBUG(3, ("initiate_myworkgroup_startup: preferred master startup for \
workgroup %s on subnet %s\n", work->work_group, subrec->subnet_name));
		work->needelection = True;
		work->ElectionCriterion |= (1<<3);
	}
  
	/* Register the WORKGROUP<0> and WORKGROUP<1e> names on the network. */

	register_name(subrec,lp_workgroup(),0x0,samba_nb_type|NB_GROUP, NULL, fail_register,NULL);
	register_name(subrec,lp_workgroup(),0x1e,samba_nb_type|NB_GROUP, NULL, fail_register,NULL);

	for( i = 0; my_netbios_names(i); i++) {
		const char *name = my_netbios_names(i);
		int stype = lp_default_server_announce() | (lp_local_master() ?  SV_TYPE_POTENTIAL_BROWSER : 0 );
   
		if(!strequal(global_myname(), name))
			stype &= ~(SV_TYPE_MASTER_BROWSER|SV_TYPE_POTENTIAL_BROWSER|SV_TYPE_DOMAIN_MASTER|SV_TYPE_DOMAIN_MEMBER);
   
		create_server_on_workgroup(work,name,stype|SV_TYPE_LOCAL_LIST_ONLY, PERMANENT_TTL, 
				string_truncate(lp_serverstring(), MAX_SERVER_STRING_LENGTH));
		DEBUG(3,("initiate_myworkgroup_startup: Added server name entry %s \
on subnet %s\n", name, subrec->subnet_name));
	}
} 

/****************************************************************************
  Dump a copy of the workgroup database into the log file.
  **************************************************************************/

void dump_workgroups(BOOL force_write)
{
	struct subnet_record *subrec;
	int debuglevel = force_write ? 0 : 4;
 
	for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_INCLUDING_UNICAST(subrec)) {
		if (subrec->workgrouplist) {
			struct work_record *work;

			if( DEBUGLVL( debuglevel ) ) {
				dbgtext( "dump_workgroups()\n " );
				dbgtext( "dump workgroup on subnet %15s: ", subrec->subnet_name );
				dbgtext( "netmask=%15s:\n", inet_ntoa(subrec->mask_ip) );
			}

			for (work = subrec->workgrouplist; work; work = work->next) {
				DEBUGADD( debuglevel, ( "\t%s(%d) current master browser = %s\n", work->work_group,
					work->token, *work->local_master_browser_name ? work->local_master_browser_name : "UNKNOWN" ) );
				if (work->serverlist) {
					struct server_record *servrec;		  
					for (servrec = work->serverlist; servrec; servrec = servrec->next) {
						DEBUGADD( debuglevel, ( "\t\t%s %8x (%s)\n",
							servrec->serv.name,
							servrec->serv.type,
							servrec->serv.comment ) );
					}
				}
			}
		}
	}
}

/****************************************************************************
  Expire any dead servers on all workgroups. If the workgroup has expired
  remove it.
  **************************************************************************/

void expire_workgroups_and_servers(time_t t)
{
	struct subnet_record *subrec;
   
	for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_INCLUDING_UNICAST(subrec)) {
		struct work_record *work;
		struct work_record *nextwork;

		for (work = subrec->workgrouplist; work; work = nextwork) {
			nextwork = work->next;
			expire_servers(work, t);

			if ((work->serverlist == NULL) && (work->death_time != PERMANENT_TTL) && 
					((t == (time_t)-1) || (work->death_time < t))) {
				DEBUG(3,("expire_workgroups_and_servers: Removing timed out workgroup %s\n",
						work->work_group));
				remove_workgroup_from_subnet(subrec, work);
			}
		}
	}
}
