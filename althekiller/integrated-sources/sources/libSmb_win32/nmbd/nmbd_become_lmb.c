/* 
   Unix SMB/CIFS implementation.
   NBT netbios routines and daemon - version 2
   Copyright (C) Andrew Tridgell 1994-1998
   Copyright (C) Luke Kenneth Casson Leighton 1994-1998
   Copyright (C) Jeremy Allison 1994-2003
   
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

extern uint16 samba_nb_type; /* Samba's NetBIOS name type. */

/*******************************************************************
 Utility function to add a name to the unicast subnet, or add in
 our IP address if it already exists.
******************************************************************/

void insert_permanent_name_into_unicast( struct subnet_record *subrec, 
                                                struct nmb_name *nmbname, uint16 nb_type )
{
	unstring name;
	struct name_record *namerec;

	if((namerec = find_name_on_subnet(unicast_subnet, nmbname, FIND_SELF_NAME)) == NULL) {
		pull_ascii_nstring(name, sizeof(name), nmbname->name);
		/* The name needs to be created on the unicast subnet. */
		(void)add_name_to_subnet( unicast_subnet, name,
				nmbname->name_type, nb_type,
				PERMANENT_TTL, PERMANENT_NAME, 1, &subrec->myip);
	} else {
		/* The name already exists on the unicast subnet. Add our local
		IP for the given broadcast subnet to the name. */
		add_ip_to_name_record( namerec, subrec->myip);
	}
}

/*******************************************************************
 Utility function to remove a name from the unicast subnet.
******************************************************************/

static void remove_permanent_name_from_unicast( struct subnet_record *subrec,
                                                struct nmb_name *nmbname )
{
	struct name_record *namerec;

	if((namerec = find_name_on_subnet(unicast_subnet, nmbname, FIND_SELF_NAME)) != NULL) {
		/* Remove this broadcast subnet IP address from the name. */
		remove_ip_from_name_record( namerec, subrec->myip);
		if(namerec->data.num_ips == 0)
			remove_name_from_namelist( unicast_subnet, namerec);
	}
}

/*******************************************************************
 Utility function always called to set our workgroup and server
 state back to potential browser, or none.
******************************************************************/

static void reset_workgroup_state( struct subnet_record *subrec, const char *workgroup_name,
                                   BOOL force_new_election )
{
	struct work_record *work;
	struct server_record *servrec;
	struct nmb_name nmbname;

	if((work = find_workgroup_on_subnet( subrec, workgroup_name)) == NULL) {
		DEBUG(0,("reset_workgroup_state: Error - cannot find workgroup %s on \
subnet %s.\n", workgroup_name, subrec->subnet_name ));
		return;
	}

	if((servrec = find_server_in_workgroup( work, global_myname())) == NULL) {
		DEBUG(0,("reset_workgroup_state: Error - cannot find server %s \
in workgroup %s on subnet %s\n",
			global_myname(), work->work_group, subrec->subnet_name));
		work->mst_state = lp_local_master() ? MST_POTENTIAL : MST_NONE;
		return;
	}

	/* Update our server status - remove any master flag and replace
		it with the potential browser flag. */
	servrec->serv.type &= ~SV_TYPE_MASTER_BROWSER;
	servrec->serv.type |= (lp_local_master() ? SV_TYPE_POTENTIAL_BROWSER : 0);

	/* Tell the namelist writer to write out a change. */
	subrec->work_changed = True;

	/* Reset our election flags. */
	work->ElectionCriterion &= ~0x4;

	work->mst_state = lp_local_master() ? MST_POTENTIAL : MST_NONE;

	/* Forget who the local master browser was for
		this workgroup. */

	set_workgroup_local_master_browser_name( work, "");

	/*
	 * Ensure the IP address of this subnet is not registered as one
	 * of the IP addresses of the WORKGROUP<1d> name on the unicast
	 * subnet. This undoes what we did below when we became a local
	 * master browser.
	 */

	make_nmb_name(&nmbname, work->work_group, 0x1d);

	remove_permanent_name_from_unicast( subrec, &nmbname);

	if(force_new_election)
		work->needelection = True;
}

/*******************************************************************
  Unbecome the local master browser name release success function.
******************************************************************/

static void unbecome_local_master_success(struct subnet_record *subrec,
                             struct userdata_struct *userdata,
                             struct nmb_name *released_name,
                             struct in_addr released_ip)
{ 
	BOOL force_new_election = False;
	unstring relname;

	memcpy((char *)&force_new_election, userdata->data, sizeof(BOOL));

	DEBUG(3,("unbecome_local_master_success: released name %s.\n",
		nmb_namestr(released_name)));

	/* Now reset the workgroup and server state. */
	pull_ascii_nstring(relname, sizeof(relname), released_name->name);
	reset_workgroup_state( subrec, relname, force_new_election );

	if( DEBUGLVL( 0 ) ) {
		dbgtext( "*****\n\n" );
		dbgtext( "Samba name server %s ", global_myname() );
		dbgtext( "has stopped being a local master browser " );
		dbgtext( "for workgroup %s ", relname );
		dbgtext( "on subnet %s\n\n*****\n", subrec->subnet_name );
	}

}

/*******************************************************************
  Unbecome the local master browser name release fail function.
******************************************************************/

static void unbecome_local_master_fail(struct subnet_record *subrec, struct response_record *rrec,
                       struct nmb_name *fail_name)
{
	struct name_record *namerec;
	struct userdata_struct *userdata = rrec->userdata;
	BOOL force_new_election = False;
	unstring failname;

	memcpy((char *)&force_new_election, userdata->data, sizeof(BOOL));

	DEBUG(0,("unbecome_local_master_fail: failed to release name %s. \
Removing from namelist anyway.\n", nmb_namestr(fail_name)));

	/* Do it anyway. */
	namerec = find_name_on_subnet(subrec, fail_name, FIND_SELF_NAME);
	if(namerec)
		remove_name_from_namelist(subrec, namerec);

	/* Now reset the workgroup and server state. */
	pull_ascii_nstring(failname, sizeof(failname), fail_name->name);
	reset_workgroup_state( subrec, failname, force_new_election );

	if( DEBUGLVL( 0 ) ) {
		dbgtext( "*****\n\n" );
		dbgtext( "Samba name server %s ", global_myname() );
		dbgtext( "has stopped being a local master browser " );
		dbgtext( "for workgroup %s ", failname );
		dbgtext( "on subnet %s\n\n*****\n", subrec->subnet_name );
	}
}

/*******************************************************************
 Utility function to remove the WORKGROUP<1d> name.
******************************************************************/

static void release_1d_name( struct subnet_record *subrec, const char *workgroup_name,
                             BOOL force_new_election)
{
	struct nmb_name nmbname;
	struct name_record *namerec;

	make_nmb_name(&nmbname, workgroup_name, 0x1d);
	if((namerec = find_name_on_subnet( subrec, &nmbname, FIND_SELF_NAME))!=NULL) {
		struct userdata_struct *userdata;
		size_t size = sizeof(struct userdata_struct) + sizeof(BOOL);

		if((userdata = (struct userdata_struct *)SMB_MALLOC(size)) == NULL) {
			DEBUG(0,("release_1d_name: malloc fail.\n"));
			return;
		}

		userdata->copy_fn = NULL;
		userdata->free_fn = NULL;
		userdata->userdata_len = sizeof(BOOL);
		memcpy((char *)userdata->data, &force_new_election, sizeof(BOOL));

		release_name(subrec, namerec,
			unbecome_local_master_success,
			unbecome_local_master_fail,
			userdata);

		zero_free(userdata, size);
	}
}

/*******************************************************************
 Unbecome the local master browser MSBROWSE name release success function.
******************************************************************/

static void release_msbrowse_name_success(struct subnet_record *subrec,
                      struct userdata_struct *userdata,
                      struct nmb_name *released_name,
                      struct in_addr released_ip)
{
	DEBUG(4,("release_msbrowse_name_success: Released name %s on subnet %s\n.",
		nmb_namestr(released_name), subrec->subnet_name ));

	/* Remove the permanent MSBROWSE name added into the unicast subnet. */
	remove_permanent_name_from_unicast( subrec, released_name);
}

/*******************************************************************
 Unbecome the local master browser MSBROWSE name release fail function.
******************************************************************/

static void release_msbrowse_name_fail( struct subnet_record *subrec, 
                       struct response_record *rrec,
                       struct nmb_name *fail_name)
{
	struct name_record *namerec;

	DEBUG(4,("release_msbrowse_name_fail: Failed to release name %s on subnet %s\n.",
		nmb_namestr(fail_name), subrec->subnet_name ));

	/* Release the name anyway. */
	namerec = find_name_on_subnet(subrec, fail_name, FIND_SELF_NAME);
	if(namerec)
		remove_name_from_namelist(subrec, namerec);

	/* Remove the permanent MSBROWSE name added into the unicast subnet. */
	remove_permanent_name_from_unicast( subrec, fail_name);
}

/*******************************************************************
  Unbecome the local master browser. If force_new_election is true, restart
  the election process after we've unbecome the local master.
******************************************************************/

void unbecome_local_master_browser(struct subnet_record *subrec, struct work_record *work,
                                   BOOL force_new_election)
{
	struct name_record *namerec;
	struct nmb_name nmbname;

  /* Sanity check. */

	DEBUG(2,("unbecome_local_master_browser: unbecoming local master for workgroup %s \
on subnet %s\n",work->work_group, subrec->subnet_name));
  
	if(find_server_in_workgroup( work, global_myname()) == NULL) {
		DEBUG(0,("unbecome_local_master_browser: Error - cannot find server %s \
in workgroup %s on subnet %s\n",
			global_myname(), work->work_group, subrec->subnet_name));
			work->mst_state = lp_local_master() ? MST_POTENTIAL : MST_NONE;
		return;
	}
  
	/* Set the state to unbecoming. */
	work->mst_state = MST_UNBECOMING_MASTER;

	/*
	 * Release the WORKGROUP<1d> name asap to allow another machine to
	 * claim it.
	 */

	release_1d_name( subrec, work->work_group, force_new_election);

	/* Deregister any browser names we may have. */
	make_nmb_name(&nmbname, MSBROWSE, 0x1);
	if((namerec = find_name_on_subnet( subrec, &nmbname, FIND_SELF_NAME))!=NULL) {
		release_name(subrec, namerec,
			release_msbrowse_name_success,
			release_msbrowse_name_fail,
			NULL);
	}

	/*
	 * Ensure we have sent and processed these release packets
	 * before returning - we don't want to process any election
	 * packets before dealing with the 1d release.
	 */

	retransmit_or_expire_response_records(time(NULL));
}

/****************************************************************************
  Success in registering the WORKGROUP<1d> name.
  We are now *really* a local master browser.
  ****************************************************************************/

static void become_local_master_stage2(struct subnet_record *subrec,
                                        struct userdata_struct *userdata,
                                        struct nmb_name *registered_name,
                                        uint16 nb_flags,
                                        int ttl, struct in_addr registered_ip)
{
	int i = 0;
	struct server_record *sl;
	struct work_record *work;
	struct server_record *servrec;
	unstring regname;

	pull_ascii_nstring(regname, sizeof(regname), registered_name->name);
	work = find_workgroup_on_subnet( subrec, regname);

	if(!work) {
		DEBUG(0,("become_local_master_stage2: Error - cannot find \
workgroup %s on subnet %s\n", regname, subrec->subnet_name));
		return;
	}

	if((servrec = find_server_in_workgroup( work, global_myname())) == NULL) {
		DEBUG(0,("become_local_master_stage2: Error - cannot find server %s \
in workgroup %s on subnet %s\n",
			global_myname(), regname, subrec->subnet_name));
			work->mst_state = lp_local_master() ? MST_POTENTIAL : MST_NONE;
		return;
	}
  
	DEBUG(3,("become_local_master_stage2: registered as master browser for workgroup %s \
on subnet %s\n", work->work_group, subrec->subnet_name));

	work->mst_state = MST_BROWSER; /* registering WORKGROUP(1d) succeeded */

	/* update our server status */
	servrec->serv.type |= SV_TYPE_MASTER_BROWSER;
	servrec->serv.type &= ~SV_TYPE_POTENTIAL_BROWSER;

	/* Tell the namelist writer to write out a change. */
	subrec->work_changed = True;

	/* Add this name to the workgroup as local master browser. */
	set_workgroup_local_master_browser_name( work, global_myname());

	/* Count the number of servers we have on our list. If it's
		less than 10 (just a heuristic) request the servers
		to announce themselves.
	*/
	for( sl = work->serverlist; sl != NULL; sl = sl->next)
		i++;

	if (i < 10) {
		/* Ask all servers on our local net to announce to us. */
		broadcast_announce_request(subrec, work);
	}

	/*
	 * Now we are a local master on a broadcast subnet, we need to add
	 * the WORKGROUP<1d> name to the unicast subnet so that we can answer
	 * unicast requests sent to this name. We can create this name directly on
	 * the unicast subnet as a WINS server always returns true when registering
	 * this name, and discards the registration. We use the number of IP
	 * addresses registered to this name as a reference count, as we
	 * remove this broadcast subnet IP address from it when we stop becoming a local
	 * master browser for this broadcast subnet.
	 */

	insert_permanent_name_into_unicast( subrec, registered_name, nb_flags);

	/* Reset the announce master browser timer so that we try and tell a domain
		master browser as soon as possible that we are a local master browser. */
	reset_announce_timer();

	if( DEBUGLVL( 0 ) ) {
		dbgtext( "*****\n\n" );
		dbgtext( "Samba name server %s ", global_myname() );
		dbgtext( "is now a local master browser " );
		dbgtext( "for workgroup %s ", work->work_group );
		dbgtext( "on subnet %s\n\n*****\n", subrec->subnet_name );
	}
}

/****************************************************************************
  Failed to register the WORKGROUP<1d> name.
  ****************************************************************************/

static void become_local_master_fail2(struct subnet_record *subrec,
                                      struct response_record *rrec,
                                      struct nmb_name *fail_name)
{
	unstring failname;
	struct work_record *work;

	DEBUG(0,("become_local_master_fail2: failed to register name %s on subnet %s. \
Failed to become a local master browser.\n", nmb_namestr(fail_name), subrec->subnet_name));

	pull_ascii_nstring(failname, sizeof(failname), fail_name->name);
	work = find_workgroup_on_subnet( subrec, failname);

	if(!work) {
		DEBUG(0,("become_local_master_fail2: Error - cannot find \
workgroup %s on subnet %s\n", failname, subrec->subnet_name));
		return;
	}

	/* Roll back all the way by calling unbecome_local_master_browser(). */
	unbecome_local_master_browser(subrec, work, False);
}

/****************************************************************************
  Success in registering the MSBROWSE name.
  ****************************************************************************/

static void become_local_master_stage1(struct subnet_record *subrec,
                                        struct userdata_struct *userdata,
                                        struct nmb_name *registered_name,
                                        uint16 nb_flags,
                                        int ttl, struct in_addr registered_ip)
{
	char *work_name = userdata->data;
	struct work_record *work = find_workgroup_on_subnet( subrec, work_name);

	if(!work) {
		DEBUG(0,("become_local_master_stage1: Error - cannot find \
			%s on subnet %s\n", work_name, subrec->subnet_name));
		return;
	}

	DEBUG(3,("become_local_master_stage1: go to stage 2: register the %s<1d> name.\n",
		work->work_group));

	work->mst_state = MST_MSB; /* Registering MSBROWSE was successful. */

	/*
	 * We registered the MSBROWSE name on a broadcast subnet, now need to add
	 * the MSBROWSE name to the unicast subnet so that we can answer
	 * unicast requests sent to this name. We create this name directly on
	 * the unicast subnet.
	 */

	insert_permanent_name_into_unicast( subrec, registered_name, nb_flags);

	/* Attempt to register the WORKGROUP<1d> name. */
	register_name(subrec, work->work_group,0x1d,samba_nb_type,
		become_local_master_stage2,
		become_local_master_fail2,
		NULL);
}

/****************************************************************************
  Failed to register the MSBROWSE name.
  ****************************************************************************/

static void become_local_master_fail1(struct subnet_record *subrec,
                                      struct response_record *rrec,
                                      struct nmb_name *fail_name)
{
	char *work_name = rrec->userdata->data;
	struct work_record *work = find_workgroup_on_subnet(subrec, work_name);

	if(!work) {
		DEBUG(0,("become_local_master_fail1: Error - cannot find \
workgroup %s on subnet %s\n", work_name, subrec->subnet_name));
		return;
	}

	if(find_server_in_workgroup(work, global_myname()) == NULL) {
		DEBUG(0,("become_local_master_fail1: Error - cannot find server %s \
in workgroup %s on subnet %s\n",
			global_myname(), work->work_group, subrec->subnet_name));
		return;
	}

	reset_workgroup_state( subrec, work->work_group, False );

	DEBUG(0,("become_local_master_fail1: Failed to become a local master browser for \
workgroup %s on subnet %s. Couldn't register name %s.\n",
		work->work_group, subrec->subnet_name, nmb_namestr(fail_name)));
}

/******************************************************************
  Become the local master browser on a subnet.
  This gets called if we win an election on this subnet.

  Stage 1: mst_state was MST_POTENTIAL - go to MST_BACK register ^1^2__MSBROWSE__^2^1.
  Stage 2: mst_state was MST_BACKUP  - go to MST_MSB  and register WORKGROUP<1d>.
  Stage 3: mst_state was MST_MSB  - go to MST_BROWSER.
******************************************************************/

void become_local_master_browser(struct subnet_record *subrec, struct work_record *work)
{
	struct userdata_struct *userdata;
	size_t size = sizeof(struct userdata_struct) + sizeof(fstring) + 1;

	/* Sanity check. */
	if (!lp_local_master()) { 
		DEBUG(0,("become_local_master_browser: Samba not configured as a local master browser.\n"));
		return;
	}

	if(!AM_POTENTIAL_MASTER_BROWSER(work)) {
		DEBUG(2,("become_local_master_browser: Awaiting potential browser state. Current state is %d\n",
			work->mst_state ));
		return;
	}

	if(find_server_in_workgroup( work, global_myname()) == NULL) {
		DEBUG(0,("become_local_master_browser: Error - cannot find server %s \
in workgroup %s on subnet %s\n",
			global_myname(), work->work_group, subrec->subnet_name));
		return;
	}

	DEBUG(2,("become_local_master_browser: Starting to become a master browser for workgroup \
%s on subnet %s\n", work->work_group, subrec->subnet_name));
  
	DEBUG(3,("become_local_master_browser: first stage - attempt to register ^1^2__MSBROWSE__^2^1\n"));
	work->mst_state = MST_BACKUP; /* an election win was successful */

	work->ElectionCriterion |= 0x5;

	/* Tell the namelist writer to write out a change. */
	subrec->work_changed = True;

	/* Setup the userdata_struct. */
	if((userdata = (struct userdata_struct *)SMB_MALLOC(size)) == NULL) {
		DEBUG(0,("become_local_master_browser: malloc fail.\n"));
		return;
	}

	userdata->copy_fn = NULL;
	userdata->free_fn = NULL;
	userdata->userdata_len = strlen(work->work_group)+1;
	overmalloc_safe_strcpy(userdata->data, work->work_group, size - sizeof(*userdata) - 1);

	/* Register the special browser group name. */
	register_name(subrec, MSBROWSE, 0x01, samba_nb_type|NB_GROUP,
		become_local_master_stage1,
		become_local_master_fail1,
		userdata);

	zero_free(userdata, size);
}

/***************************************************************
 Utility function to set the local master browser name. Does
 some sanity checking as old versions of Samba seem to sometimes
 say that the master browser name for a workgroup is the same
 as the workgroup name.
****************************************************************/

void set_workgroup_local_master_browser_name( struct work_record *work, const char *newname)
{
	DEBUG(5,("set_workgroup_local_master_browser_name: setting local master name to '%s' \
for workgroup %s.\n", newname, work->work_group ));

#if 0
  /*
   * Apparently some sites use the workgroup name as the local
   * master browser name. Arrrrggghhhhh ! (JRA).
   */
  if(strequal( work->work_group, newname))
  {
    DEBUG(5, ("set_workgroup_local_master_browser_name: Refusing to set \
local_master_browser_name for workgroup %s to workgroup name.\n",
         work->work_group ));
    return;
  }
#endif

	unstrcpy(work->local_master_browser_name, newname);
}
