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

static void become_domain_master_browser_bcast(const char *);

/****************************************************************************
  Fail to become a Domain Master Browser on a subnet.
  ****************************************************************************/

static void become_domain_master_fail(struct subnet_record *subrec,
                                      struct response_record *rrec,
                                      struct nmb_name *fail_name)
{
	unstring failname;
	struct work_record *work;
	struct server_record *servrec;

	pull_ascii_nstring(failname, sizeof(failname), fail_name->name);
	work = find_workgroup_on_subnet(subrec, failname);
	if(!work) {
		DEBUG(0,("become_domain_master_fail: Error - cannot find \
workgroup %s on subnet %s\n", failname, subrec->subnet_name));
		return;
	}

	/* Set the state back to DOMAIN_NONE. */
	work->dom_state = DOMAIN_NONE;

	if((servrec = find_server_in_workgroup( work, global_myname())) == NULL) {
		DEBUG(0,("become_domain_master_fail: Error - cannot find server %s \
in workgroup %s on subnet %s\n",
			global_myname(), work->work_group, subrec->subnet_name));
		return;
	}

	/* Update our server status. */
	servrec->serv.type &= ~SV_TYPE_DOMAIN_MASTER;

	/* Tell the namelist writer to write out a change. */
	subrec->work_changed = True;

	DEBUG(0,("become_domain_master_fail: Failed to become a domain master browser for \
workgroup %s on subnet %s. Couldn't register name %s.\n",
		work->work_group, subrec->subnet_name, nmb_namestr(fail_name)));
}

/****************************************************************************
  Become a Domain Master Browser on a subnet.
  ****************************************************************************/

static void become_domain_master_stage2(struct subnet_record *subrec, 
                                        struct userdata_struct *userdata,
                                        struct nmb_name *registered_name,
                                        uint16 nb_flags,
                                        int ttl, struct in_addr registered_ip)
{
	unstring regname;
	struct work_record *work;
	struct server_record *servrec;

	pull_ascii_nstring(regname, sizeof(regname), registered_name->name);
	work = find_workgroup_on_subnet( subrec, regname);

	if(!work) {
		DEBUG(0,("become_domain_master_stage2: Error - cannot find \
workgroup %s on subnet %s\n", regname, subrec->subnet_name));
		return;
	}

	if((servrec = find_server_in_workgroup( work, global_myname())) == NULL) {
		DEBUG(0,("become_domain_master_stage2: Error - cannot find server %s \
in workgroup %s on subnet %s\n", 
		global_myname(), regname, subrec->subnet_name));
		work->dom_state = DOMAIN_NONE;
		return;
	}

	/* Set the state in the workgroup structure. */
	work->dom_state = DOMAIN_MST; /* Become domain master. */

	/* Update our server status. */
	servrec->serv.type |= (SV_TYPE_NT|SV_TYPE_DOMAIN_MASTER);

	/* Tell the namelist writer to write out a change. */
	subrec->work_changed = True;

	if( DEBUGLVL( 0 ) ) {
		dbgtext( "*****\n\nSamba server %s ", global_myname() );
		dbgtext( "is now a domain master browser for " );
		dbgtext( "workgroup %s ", work->work_group );
		dbgtext( "on subnet %s\n\n*****\n", subrec->subnet_name );
	}

	if( subrec == unicast_subnet ) {
		struct nmb_name nmbname;
		struct in_addr my_first_ip;
		struct in_addr *nip;

		/* Put our name and first IP address into the 
		   workgroup struct as domain master browser. This
		   will stop us syncing with ourself if we are also
		   a local master browser. */

		make_nmb_name(&nmbname, global_myname(), 0x20);

		work->dmb_name = nmbname;
		/* Pick the first interface ip address as the domain master browser ip. */
		nip = iface_n_ip(0);

		if (!nip) {
			DEBUG(0,("become_domain_master_stage2: Error. iface_n_ip returned NULL\n"));
			return;
		}

		my_first_ip = *nip;

		putip((char *)&work->dmb_addr, &my_first_ip);

		/* We successfully registered by unicast with the
		   WINS server.  We now expect to become the domain
		   master on the local subnets. If this fails, it's
		   probably a 1.9.16p2 to 1.9.16p11 server's fault.

		   This is a configuration issue that should be addressed
		   by the network administrator - you shouldn't have
		   several machines configured as a domain master browser
		   for the same WINS scope (except if they are 1.9.17 or
		   greater, and you know what you're doing.

		   see docs/DOMAIN.txt.

		*/
		become_domain_master_browser_bcast(work->work_group);
	} else {
		/*
		 * Now we are a domain master on a broadcast subnet, we need to add
		 * the WORKGROUP<1b> name to the unicast subnet so that we can answer
		 * unicast requests sent to this name. This bug wasn't found for a while
		 * as it is strange to have a DMB without using WINS. JRA.
		 */
		insert_permanent_name_into_unicast(subrec, registered_name, nb_flags);
	}
}

/****************************************************************************
  Start the name registration process when becoming a Domain Master Browser
  on a subnet.
****************************************************************************/

static void become_domain_master_stage1(struct subnet_record *subrec, const char *wg_name)
{ 
	struct work_record *work;

	DEBUG(2,("become_domain_master_stage1: Becoming domain master browser for \
workgroup %s on subnet %s\n", wg_name, subrec->subnet_name));

	/* First, find the workgroup on the subnet. */
	if((work = find_workgroup_on_subnet( subrec, wg_name )) == NULL) {
		DEBUG(0,("become_domain_master_stage1: Error - unable to find workgroup %s on subnet %s.\n",
			wg_name, subrec->subnet_name));
		return;
	}

	DEBUG(3,("become_domain_master_stage1: go to first stage: register <1b> name\n"));
	work->dom_state = DOMAIN_WAIT;

	/* WORKGROUP<1b> is the domain master browser name. */
	register_name(subrec, work->work_group,0x1b,samba_nb_type,
			become_domain_master_stage2,
			become_domain_master_fail, NULL);
}

/****************************************************************************
  Function called when a query for a WORKGROUP<1b> name succeeds.
  This is normally a fail condition as it means there is already
  a domain master browser for a workgroup and we were trying to
  become one.
****************************************************************************/

static void become_domain_master_query_success(struct subnet_record *subrec,
                        struct userdata_struct *userdata,
                        struct nmb_name *nmbname, struct in_addr ip, 
                        struct res_rec *rrec)
{
	unstring name;
	pull_ascii_nstring(name, sizeof(name), nmbname->name);

	/* If the given ip is not ours, then we can't become a domain
		controler as the name is already registered.
	*/

	/* BUG note. Samba 1.9.16p11 servers seem to return the broadcast
		address or zero ip for this query. Pretend this is ok. */

	if(ismyip(ip) || ip_equal(allones_ip, ip) || is_zero_ip(ip)) {
		if( DEBUGLVL( 3 ) ) {
			dbgtext( "become_domain_master_query_success():\n" );
			dbgtext( "Our address (%s) ", inet_ntoa(ip) );
			dbgtext( "returned in query for name %s ", nmb_namestr(nmbname) );
			dbgtext( "(domain master browser name) " );
			dbgtext( "on subnet %s.\n", subrec->subnet_name );
			dbgtext( "Continuing with domain master code.\n" );
		}

		become_domain_master_stage1(subrec, name);
	} else {
		if( DEBUGLVL( 0 ) ) {
			dbgtext( "become_domain_master_query_success:\n" );
			dbgtext( "There is already a domain master browser at " );
			dbgtext( "IP %s for workgroup %s ", inet_ntoa(ip), name );
			dbgtext( "registered on subnet %s.\n", subrec->subnet_name );
		}
	}
}

/****************************************************************************
  Function called when a query for a WORKGROUP<1b> name fails.
  This is normally a success condition as it then allows us to register
  our own Domain Master Browser name.
  ****************************************************************************/

static void become_domain_master_query_fail(struct subnet_record *subrec,
                                    struct response_record *rrec,
                                    struct nmb_name *question_name, int fail_code)
{
	unstring name;

	/* If the query was unicast, and the error is not NAM_ERR (name didn't exist),
		then this is a failure. Otherwise, not finding the name is what we want. */

	if((subrec == unicast_subnet) && (fail_code != NAM_ERR)) {
		DEBUG(0,("become_domain_master_query_fail: Error %d returned when \
querying WINS server for name %s.\n", 
			fail_code, nmb_namestr(question_name)));
		return;
	}

	/* Otherwise - not having the name allows us to register it. */
	pull_ascii_nstring(name, sizeof(name), question_name->name);
	become_domain_master_stage1(subrec, name);
}

/****************************************************************************
  Attempt to become a domain master browser on all broadcast subnets.
  ****************************************************************************/

static void become_domain_master_browser_bcast(const char *workgroup_name)
{
	struct subnet_record *subrec;

	for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) { 
		struct work_record *work = find_workgroup_on_subnet(subrec, workgroup_name);

		if (work && (work->dom_state == DOMAIN_NONE)) {
			struct nmb_name nmbname;
			make_nmb_name(&nmbname,workgroup_name,0x1b);

			/*
			 * Check for our name on the given broadcast subnet first, only initiate
			 * further processing if we cannot find it.
			 */

			if (find_name_on_subnet(subrec, &nmbname, FIND_SELF_NAME) == NULL) {
				if( DEBUGLVL( 0 ) ) {
					dbgtext( "become_domain_master_browser_bcast:\n" );
					dbgtext( "Attempting to become domain master browser on " );
					dbgtext( "workgroup %s on subnet %s\n",
						workgroup_name, subrec->subnet_name );
				}

				/* Send out a query to establish whether there's a 
				   domain controller on the local subnet. If not,
				   we can become a domain controller. 
				*/

				DEBUG(0,("become_domain_master_browser_bcast: querying subnet %s \
for domain master browser on workgroup %s\n", subrec->subnet_name, workgroup_name));

				query_name(subrec, workgroup_name, nmbname.name_type,
					become_domain_master_query_success, 
					become_domain_master_query_fail,
					NULL);
			}
		}
	}
}

/****************************************************************************
  Attempt to become a domain master browser by registering with WINS.
  ****************************************************************************/

static void become_domain_master_browser_wins(const char *workgroup_name)
{
	struct work_record *work;

	work = find_workgroup_on_subnet(unicast_subnet, workgroup_name);

	if (work && (work->dom_state == DOMAIN_NONE)) {
		struct nmb_name nmbname;

		make_nmb_name(&nmbname,workgroup_name,0x1b);

		/*
		 * Check for our name on the unicast subnet first, only initiate
		 * further processing if we cannot find it.
		 */

		if (find_name_on_subnet(unicast_subnet, &nmbname, FIND_SELF_NAME) == NULL) {
			if( DEBUGLVL( 0 ) ) {
				dbgtext( "become_domain_master_browser_wins:\n" );
				dbgtext( "Attempting to become domain master browser " );
				dbgtext( "on workgroup %s, subnet %s.\n",
					workgroup_name, unicast_subnet->subnet_name );
			}

			/* Send out a query to establish whether there's a 
			   domain master broswer registered with WINS. If not,
			   we can become a domain master browser. 
			*/

			DEBUG(0,("become_domain_master_browser_wins: querying WINS server from IP %s \
for domain master browser name %s on workgroup %s\n",
				inet_ntoa(unicast_subnet->myip), nmb_namestr(&nmbname), workgroup_name));

			query_name(unicast_subnet, workgroup_name, nmbname.name_type,
				become_domain_master_query_success,
				become_domain_master_query_fail,
				NULL);
		}
	}
}

/****************************************************************************
  Add the domain logon server and domain master browser names
  if we are set up to do so.
  **************************************************************************/

void add_domain_names(time_t t)
{
	static time_t lastrun = 0;

	if ((lastrun != 0) && (t < lastrun + (CHECK_TIME_ADD_DOM_NAMES * 60)))
		return;

	lastrun = t;

	/* Do the "internet group" - <1c> names. */
	if (lp_domain_logons())
		add_logon_names();

	/* Do the domain master names. */
	if(lp_domain_master()) {
		if(we_are_a_wins_client()) {
			/* We register the WORKGROUP<1b> name with the WINS
				server first, and call add_domain_master_bcast()
				only if this is successful.

				This results in domain logon services being gracefully provided,
				as opposed to the aggressive nature of 1.9.16p2 to 1.9.16p11.
				1.9.16p2 to 1.9.16p11 - due to a bug in namelogon.c,
				cannot provide domain master / domain logon services.
			*/
			become_domain_master_browser_wins(lp_workgroup());
		} else {
			become_domain_master_browser_bcast(lp_workgroup());
		}
	}
}
