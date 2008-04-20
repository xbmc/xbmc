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

extern struct in_addr allones_ip;

extern uint16 samba_nb_type; /* Samba's NetBIOS type. */

/****************************************************************************
  Fail to become a Logon server on a subnet.
****************************************************************************/

static void become_logon_server_fail(struct subnet_record *subrec,
                                      struct response_record *rrec,
                                      struct nmb_name *fail_name)
{
	unstring failname;
	struct work_record *work;
	struct server_record *servrec;

	pull_ascii_nstring(failname, sizeof(failname), fail_name->name);
	work = find_workgroup_on_subnet(subrec, failname);
	if(!work) {
		DEBUG(0,("become_logon_server_fail: Error - cannot find \
workgroup %s on subnet %s\n", failname, subrec->subnet_name));
		return;
	}

	if((servrec = find_server_in_workgroup( work, global_myname())) == NULL) {
		DEBUG(0,("become_logon_server_fail: Error - cannot find server %s \
in workgroup %s on subnet %s\n",
			global_myname(), failname, subrec->subnet_name));
		work->log_state = LOGON_NONE;
		return;
	}

	/* Set the state back to LOGON_NONE. */
	work->log_state = LOGON_NONE;

	servrec->serv.type &= ~SV_TYPE_DOMAIN_CTRL;

	DEBUG(0,("become_logon_server_fail: Failed to become a domain master for \
workgroup %s on subnet %s. Couldn't register name %s.\n",
		work->work_group, subrec->subnet_name, nmb_namestr(fail_name)));

}

/****************************************************************************
  Become a Logon server on a subnet.
  ****************************************************************************/

static void become_logon_server_success(struct subnet_record *subrec,
                                        struct userdata_struct *userdata,
                                        struct nmb_name *registered_name,
                                        uint16 nb_flags,
                                        int ttl, struct in_addr registered_ip)
{
	unstring reg_name;
	struct work_record *work;
	struct server_record *servrec;

	pull_ascii_nstring(reg_name, sizeof(reg_name), registered_name->name);
	work = find_workgroup_on_subnet( subrec, reg_name);
	if(!work) {
		DEBUG(0,("become_logon_server_success: Error - cannot find \
workgroup %s on subnet %s\n", reg_name, subrec->subnet_name));
		return;
	}

	if((servrec = find_server_in_workgroup( work, global_myname())) == NULL) {
		DEBUG(0,("become_logon_server_success: Error - cannot find server %s \
in workgroup %s on subnet %s\n",
			global_myname(), reg_name, subrec->subnet_name));
		work->log_state = LOGON_NONE;
		return;
	}

	/* Set the state in the workgroup structure. */
	work->log_state = LOGON_SRV; /* Become domain master. */

	/* Update our server status. */
	servrec->serv.type |= (SV_TYPE_NT|SV_TYPE_DOMAIN_MEMBER);
	/* To allow Win95 policies to load we need to set type domain
		controller.
	*/
	servrec->serv.type |= SV_TYPE_DOMAIN_CTRL;

	/* Tell the namelist writer to write out a change. */
	subrec->work_changed = True;

	/*
	 * Add the WORKGROUP<1C> name to the UNICAST subnet with the IP address
	 * for this subnet so we will respond to queries on this name.
	 */

	{
		struct nmb_name nmbname;
		make_nmb_name(&nmbname,lp_workgroup(),0x1c);
		insert_permanent_name_into_unicast(subrec, &nmbname, 0x1c);
	}

	DEBUG(0,("become_logon_server_success: Samba is now a logon server \
for workgroup %s on subnet %s\n", work->work_group, subrec->subnet_name));
}

/*******************************************************************
  Become a logon server by attempting to register the WORKGROUP<1c>
  group name.
******************************************************************/

static void become_logon_server(struct subnet_record *subrec,
                                struct work_record *work)
{
	DEBUG(2,("become_logon_server: Atempting to become logon server for workgroup %s \
on subnet %s\n", work->work_group,subrec->subnet_name));

	DEBUG(3,("become_logon_server: go to first stage: register %s<1c> name\n",
		work->work_group));
	work->log_state = LOGON_WAIT;

	register_name(subrec, work->work_group,0x1c,samba_nb_type|NB_GROUP,
			become_logon_server_success,
			become_logon_server_fail, NULL);
}

/*****************************************************************************
  Add the internet group <1c> logon names by unicast and broadcast.
  ****************************************************************************/

void add_logon_names(void)
{
	struct subnet_record *subrec;

	for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_INCLUDING_UNICAST(subrec)) {
		struct work_record *work = find_workgroup_on_subnet(subrec, lp_workgroup());

		if (work && (work->log_state == LOGON_NONE)) {
			struct nmb_name nmbname;
			make_nmb_name(&nmbname,lp_workgroup(),0x1c);

			if (find_name_on_subnet(subrec, &nmbname, FIND_SELF_NAME) == NULL) {
				if( DEBUGLVL( 0 ) ) {
					dbgtext( "add_domain_logon_names:\n" );
					dbgtext( "Attempting to become logon server " );
					dbgtext( "for workgroup %s ", lp_workgroup() );
					dbgtext( "on subnet %s\n", subrec->subnet_name );
				}
				become_logon_server(subrec, work);
			}
		}
	}
}
