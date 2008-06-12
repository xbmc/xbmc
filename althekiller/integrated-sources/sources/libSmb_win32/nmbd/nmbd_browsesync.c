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

/* This is our local master browser list database. */
extern struct browse_cache_record *lmb_browserlist;

/****************************************************************************
As a domain master browser, do a sync with a local master browser.
**************************************************************************/

static void sync_with_lmb(struct browse_cache_record *browc)
{                     
	struct work_record *work;

	if( !(work = find_workgroup_on_subnet(unicast_subnet, browc->work_group)) ) {
		if( DEBUGLVL( 0 ) ) {
			dbgtext( "sync_with_lmb:\n" );
			dbgtext( "Failed to get a workgroup for a local master browser " );
			dbgtext( "cache entry workgroup " );
			dbgtext( "%s, server %s\n", browc->work_group, browc->lmb_name );
		}
		return;
	}

	/* We should only be doing this if we are a domain master browser for
		the given workgroup. Ensure this is so. */

	if(!AM_DOMAIN_MASTER_BROWSER(work)) {
		if( DEBUGLVL( 0 ) ) {
			dbgtext( "sync_with_lmb:\n" );
			dbgtext( "We are trying to sync with a local master browser " );
			dbgtext( "%s for workgroup %s\n", browc->lmb_name, browc->work_group );
			dbgtext( "and we are not a domain master browser on this workgroup.\n" );
			dbgtext( "Error!\n" );
		}
		return;
	}

	if( DEBUGLVL( 2 ) ) {
		dbgtext( "sync_with_lmb:\n" );
		dbgtext( "Initiating sync with local master browser " );
		dbgtext( "%s<0x20> at IP %s ", browc->lmb_name, inet_ntoa(browc->ip) );
		dbgtext( "for workgroup %s\n", browc->work_group );
	}

	sync_browse_lists(work, browc->lmb_name, 0x20, browc->ip, True, True);

	browc->sync_time += (CHECK_TIME_DMB_TO_LMB_SYNC * 60);
}

/****************************************************************************
Sync or expire any local master browsers.
**************************************************************************/

void dmb_expire_and_sync_browser_lists(time_t t)
{
	static time_t last_run = 0;
	struct browse_cache_record *browc;

	/* Only do this every 20 seconds. */  
	if (t - last_run < 20) 
		return;

	last_run = t;

	expire_lmb_browsers(t);

	for( browc = lmb_browserlist; browc; browc = browc->next ) {
		if (browc->sync_time < t)
			sync_with_lmb(browc);
	}
}

/****************************************************************************
As a local master browser, send an announce packet to the domain master browser.
**************************************************************************/

static void announce_local_master_browser_to_domain_master_browser( struct work_record *work)
{
	pstring outbuf;
	unstring myname;
	unstring dmb_name;
	char *p;

	if(ismyip(work->dmb_addr)) {
		if( DEBUGLVL( 2 ) ) {
			dbgtext( "announce_local_master_browser_to_domain_master_browser:\n" );
			dbgtext( "We are both a domain and a local master browser for " );
			dbgtext( "workgroup %s.  ", work->work_group );
			dbgtext( "Do not announce to ourselves.\n" );
		}
		return;
	}

	memset(outbuf,'\0',sizeof(outbuf));
	p = outbuf;
	SCVAL(p,0,ANN_MasterAnnouncement);
	p++;

	unstrcpy(myname, global_myname());
	strupper_m(myname);
	myname[15]='\0';
	/* The call below does CH_UNIX -> CH_DOS conversion. JRA */
	push_pstring_base(p, myname, outbuf);

	p = skip_string(p,1);

	if( DEBUGLVL( 4 ) ) {
		dbgtext( "announce_local_master_browser_to_domain_master_browser:\n" );
		dbgtext( "Sending local master announce to " );
		dbgtext( "%s for workgroup %s.\n", nmb_namestr(&work->dmb_name),
					work->work_group );
	}

	/* Target name for send_mailslot must be in UNIX charset. */
	pull_ascii_nstring(dmb_name, sizeof(dmb_name), work->dmb_name.name);
	send_mailslot(True, BROWSE_MAILSLOT, outbuf,PTR_DIFF(p,outbuf),
		global_myname(), 0x0, dmb_name, 0x0, 
		work->dmb_addr, FIRST_SUBNET->myip, DGRAM_PORT);
}

/****************************************************************************
As a local master browser, do a sync with a domain master browser.
**************************************************************************/

static void sync_with_dmb(struct work_record *work)
{
	unstring dmb_name;

	if( DEBUGLVL( 2 ) ) {
		dbgtext( "sync_with_dmb:\n" );
		dbgtext( "Initiating sync with domain master browser " );
		dbgtext( "%s ", nmb_namestr(&work->dmb_name) );
		dbgtext( "at IP %s ", inet_ntoa(work->dmb_addr) );
		dbgtext( "for workgroup %s\n", work->work_group );
	}

	pull_ascii_nstring(dmb_name, sizeof(dmb_name), work->dmb_name.name);
	sync_browse_lists(work, dmb_name, work->dmb_name.name_type, 
		work->dmb_addr, False, True);
}

/****************************************************************************
  Function called when a node status query to a domain master browser IP succeeds.
****************************************************************************/

static void domain_master_node_status_success(struct subnet_record *subrec,
                                              struct userdata_struct *userdata,
                                              struct res_rec *answers,
                                              struct in_addr from_ip)
{
	struct work_record *work = find_workgroup_on_subnet( subrec, userdata->data);

	if( work == NULL ) {
		if( DEBUGLVL( 0 ) ) {
			dbgtext( "domain_master_node_status_success:\n" );
			dbgtext( "Unable to find workgroup " );
			dbgtext( "%s on subnet %s.\n", userdata->data, subrec->subnet_name );
		}
		return;
	}

	if( DEBUGLVL( 3 ) ) {
		dbgtext( "domain_master_node_status_success:\n" );
		dbgtext( "Success in node status for workgroup " );
		dbgtext( "%s from ip %s\n", work->work_group, inet_ntoa(from_ip) );
	}

  /* Go through the list of names found at answers->rdata and look for
     the first SERVER<0x20> name. */

	if(answers->rdata != NULL) {
		char *p = answers->rdata;
		int numnames = CVAL(p, 0);

		p += 1;

		while (numnames--) {
			unstring qname;
			uint16 nb_flags;
			int name_type;

			pull_ascii_nstring(qname, sizeof(qname), p);
			name_type = CVAL(p,15);
			nb_flags = get_nb_flags(&p[16]);
			trim_char(qname,'\0',' ');

			p += 18;

			if(!(nb_flags & NB_GROUP) && (name_type == 0x20)) {
				struct nmb_name nmbname;

				make_nmb_name(&nmbname, qname, name_type);

				/* Copy the dmb name and IP address
					into the workgroup struct. */

				work->dmb_name = nmbname;
				putip((char *)&work->dmb_addr, &from_ip);

				/* Do the local master browser announcement to the domain
					master browser name and IP. */
				announce_local_master_browser_to_domain_master_browser( work );

				/* Now synchronise lists with the domain master browser. */
				sync_with_dmb(work);
				break;
			}
		}
	} else if( DEBUGLVL( 0 ) ) {
		dbgtext( "domain_master_node_status_success:\n" );
		dbgtext( "Failed to find a SERVER<0x20> name in reply from IP " );
		dbgtext( "%s.\n", inet_ntoa(from_ip) );
	}
}

/****************************************************************************
  Function called when a node status query to a domain master browser IP fails.
****************************************************************************/

static void domain_master_node_status_fail(struct subnet_record *subrec,
                       struct response_record *rrec)
{
	struct userdata_struct *userdata = rrec->userdata;

	if( DEBUGLVL( 0 ) ) {
		dbgtext( "domain_master_node_status_fail:\n" );
		dbgtext( "Doing a node status request to the domain master browser\n" );
		dbgtext( "for workgroup %s ", userdata ? userdata->data : "NULL" );
		dbgtext( "at IP %s failed.\n", inet_ntoa(rrec->packet->ip) );
		dbgtext( "Cannot sync browser lists.\n" );
	}
}

/****************************************************************************
  Function called when a query for a WORKGROUP<1b> name succeeds.
****************************************************************************/

static void find_domain_master_name_query_success(struct subnet_record *subrec,
                        struct userdata_struct *userdata_in,
                        struct nmb_name *q_name, struct in_addr answer_ip, struct res_rec *rrec)
{
	/* 
	 * Unfortunately, finding the IP address of the Domain Master Browser,
	 * as we have here, is not enough. We need to now do a sync to the
	 * SERVERNAME<0x20> NetBIOS name, as only recent NT servers will
	 * respond to the SMBSERVER name. To get this name from IP
	 * address we do a Node status request, and look for the first
	 * NAME<0x20> in the response, and take that as the server name.
	 * We also keep a cache of the Domain Master Browser name for this
	 * workgroup in the Workgroup struct, so that if the same IP addess
	 * is returned every time, we don't need to do the node status
	 * request.
	 */

	struct work_record *work;
	struct nmb_name nmbname;
	struct userdata_struct *userdata;
	size_t size = sizeof(struct userdata_struct) + sizeof(fstring)+1;
	unstring qname;

	pull_ascii_nstring(qname, sizeof(qname), q_name->name);
	if( !(work = find_workgroup_on_subnet(subrec, qname)) ) {
		if( DEBUGLVL( 0 ) ) {
			dbgtext( "find_domain_master_name_query_success:\n" );
			dbgtext( "Failed to find workgroup %s\n", qname);
		}
	return;
  }

  /* First check if we already have a dmb for this workgroup. */

	if(!is_zero_ip(work->dmb_addr) && ip_equal(work->dmb_addr, answer_ip)) {
		/* Do the local master browser announcement to the domain
			master browser name and IP. */
		announce_local_master_browser_to_domain_master_browser( work );

		/* Now synchronise lists with the domain master browser. */
		sync_with_dmb(work);
		return;
	} else {
		zero_ip(&work->dmb_addr);
	}

	/* Now initiate the node status request. */

	/* We used to use the name "*",0x0 here, but some Windows
	 * servers don't answer that name. However we *know* they
	 * have the name workgroup#1b (as we just looked it up).
	 * So do the node status request on this name instead.
	 * Found at LBL labs. JRA.
	 */

	make_nmb_name(&nmbname,work->work_group,0x1b);

	/* Put the workgroup name into the userdata so we know
	 what workgroup we're talking to when the reply comes
	 back. */

	/* Setup the userdata_struct - this is copied so we can use
	a stack variable for this. */

	if((userdata = (struct userdata_struct *)SMB_MALLOC(size)) == NULL) {
		DEBUG(0, ("find_domain_master_name_query_success: malloc fail.\n"));
		return;
	}

	userdata->copy_fn = NULL;
	userdata->free_fn = NULL;
	userdata->userdata_len = strlen(work->work_group)+1;
	overmalloc_safe_strcpy(userdata->data, work->work_group, size - sizeof(*userdata) - 1);

	node_status( subrec, &nmbname, answer_ip, 
		domain_master_node_status_success,
		domain_master_node_status_fail,
		userdata);

	zero_free(userdata, size);
}

/****************************************************************************
  Function called when a query for a WORKGROUP<1b> name fails.
  ****************************************************************************/

static void find_domain_master_name_query_fail(struct subnet_record *subrec,
                                    struct response_record *rrec,
                                    struct nmb_name *question_name, int fail_code)
{
	if( DEBUGLVL( 0 ) ) {
		dbgtext( "find_domain_master_name_query_fail:\n" );
		dbgtext( "Unable to find the Domain Master Browser name " );
		dbgtext( "%s for the workgroup %s.\n",
			nmb_namestr(question_name), question_name->name );
		dbgtext( "Unable to sync browse lists in this workgroup.\n" );
	}
}

/****************************************************************************
As a local master browser for a workgroup find the domain master browser
name, announce ourselves as local master browser to it and then pull the
full domain browse lists from it onto the given subnet.
**************************************************************************/

void announce_and_sync_with_domain_master_browser( struct subnet_record *subrec,
                                                   struct work_record *work)
{
	/* Only do this if we are using a WINS server. */
	if(we_are_a_wins_client() == False) {
		if( DEBUGLVL( 10 ) ) {
			dbgtext( "announce_and_sync_with_domain_master_browser:\n" );
			dbgtext( "Ignoring, as we are not a WINS client.\n" );
		}
		return;
	}

	/* First, query for the WORKGROUP<1b> name from the WINS server. */
	query_name(unicast_subnet, work->work_group, 0x1b,
             find_domain_master_name_query_success,
             find_domain_master_name_query_fail,
             NULL);
}

/****************************************************************************
  Function called when a node status query to a domain master browser IP succeeds.
  This function is only called on query to a Samba 1.9.18 or above WINS server.

  Note that adding the workgroup name is enough for this workgroup to be
  browsable by clients, as clients query the WINS server or broadcast 
  nets for the WORKGROUP<1b> name when they want to browse a workgroup
  they are not in. We do not need to do a sync with this Domain Master
  Browser in order for our browse clients to see machines in this workgroup.
  JRA.
****************************************************************************/

static void get_domain_master_name_node_status_success(struct subnet_record *subrec,
                                              struct userdata_struct *userdata,
                                              struct res_rec *answers,
                                              struct in_addr from_ip)
{
	struct work_record *work;
	unstring server_name;

	server_name[0] = 0;

	if( DEBUGLVL( 3 ) ) {
		dbgtext( "get_domain_master_name_node_status_success:\n" );
		dbgtext( "Success in node status from ip %s\n", inet_ntoa(from_ip) );
	}

	/* 
	 * Go through the list of names found at answers->rdata and look for
	 * the first WORKGROUP<0x1b> name.
	 */

	if(answers->rdata != NULL) {
		char *p = answers->rdata;
		int numnames = CVAL(p, 0);

		p += 1;

		while (numnames--) {
			unstring qname;
			uint16 nb_flags;
			int name_type;

			pull_ascii_nstring(qname, sizeof(qname), p);
			name_type = CVAL(p,15);
			nb_flags = get_nb_flags(&p[16]);
			trim_char(qname,'\0',' ');

			p += 18;

			if(!(nb_flags & NB_GROUP) && (name_type == 0x00) && 
					server_name[0] == 0) {
				/* this is almost certainly the server netbios name */
				unstrcpy(server_name, qname);
				continue;
			}

			if(!(nb_flags & NB_GROUP) && (name_type == 0x1b)) {
				if( DEBUGLVL( 5 ) ) {
					dbgtext( "get_domain_master_name_node_status_success:\n" );
					dbgtext( "%s(%s) ", server_name, inet_ntoa(from_ip) );
					dbgtext( "is a domain master browser for workgroup " );
					dbgtext( "%s. Adding this name.\n", qname );
				}

				/* 
				 * If we don't already know about this workgroup, add it
				 * to the workgroup list on the unicast_subnet.
				 */

				if((work = find_workgroup_on_subnet( subrec, qname)) == NULL) {
					struct nmb_name nmbname;
					/* 
					 * Add it - with an hour in the cache.
					 */
					if(!(work= create_workgroup_on_subnet(subrec, qname, 60*60)))
						return;

					/* remember who the master is */
					unstrcpy(work->local_master_browser_name, server_name);
					make_nmb_name(&nmbname, server_name, 0x20);
					work->dmb_name = nmbname;
					work->dmb_addr = from_ip;
				}
				break;
			}
		}
	} else if( DEBUGLVL( 0 ) ) {
		dbgtext( "get_domain_master_name_node_status_success:\n" );
		dbgtext( "Failed to find a WORKGROUP<0x1b> name in reply from IP " );
		dbgtext( "%s.\n", inet_ntoa(from_ip) );
	}
}

/****************************************************************************
  Function called when a node status query to a domain master browser IP fails.
****************************************************************************/

static void get_domain_master_name_node_status_fail(struct subnet_record *subrec,
                       struct response_record *rrec)
{
	if( DEBUGLVL( 0 ) ) {
		dbgtext( "get_domain_master_name_node_status_fail:\n" );
		dbgtext( "Doing a node status request to the domain master browser " );
		dbgtext( "at IP %s failed.\n", inet_ntoa(rrec->packet->ip) );
		dbgtext( "Cannot get workgroup name.\n" );
	}
}

/****************************************************************************
  Function called when a query for *<1b> name succeeds.
****************************************************************************/

static void find_all_domain_master_names_query_success(struct subnet_record *subrec,
                        struct userdata_struct *userdata_in,
                        struct nmb_name *q_name, struct in_addr answer_ip, struct res_rec *rrec)
{
	/* 
	 * We now have a list of all the domain master browsers for all workgroups
	 * that have registered with the WINS server. Now do a node status request
	 * to each one and look for the first 1b name in the reply. This will be
	 * the workgroup name that we will add to the unicast subnet as a 'non-local'
	 * workgroup.
	 */

	struct nmb_name nmbname;
	struct in_addr send_ip;
	int i;

	if( DEBUGLVL( 5 ) ) {
		dbgtext( "find_all_domain_master_names_query_succes:\n" );
		dbgtext( "Got answer from WINS server of %d ", (rrec->rdlength / 6) );
		dbgtext( "IP addresses for Domain Master Browsers.\n" );
	}

	for(i = 0; i < rrec->rdlength / 6; i++) {
		/* Initiate the node status requests. */
		make_nmb_name(&nmbname, "*", 0);

		putip((char *)&send_ip, (char *)&rrec->rdata[(i*6) + 2]);

		/* 
		 * Don't send node status requests to ourself.
		 */

		if(ismyip( send_ip )) {
			if( DEBUGLVL( 5 ) ) {
				dbgtext( "find_all_domain_master_names_query_succes:\n" );
				dbgtext( "Not sending node status to our own IP " );
				dbgtext( "%s.\n", inet_ntoa(send_ip) );
			}
			continue;
		}

		if( DEBUGLVL( 5 ) ) {
			dbgtext( "find_all_domain_master_names_query_success:\n" );
			dbgtext( "Sending node status request to IP %s.\n", inet_ntoa(send_ip) );
		}

		node_status( subrec, &nmbname, send_ip, 
				get_domain_master_name_node_status_success,
				get_domain_master_name_node_status_fail,
				NULL);
	}
}

/****************************************************************************
  Function called when a query for *<1b> name fails.
  ****************************************************************************/
static void find_all_domain_master_names_query_fail(struct subnet_record *subrec,
                                    struct response_record *rrec,
                                    struct nmb_name *question_name, int fail_code)
{
	if( DEBUGLVL( 10 ) ) {
		dbgtext( "find_domain_master_name_query_fail:\n" );
		dbgtext( "WINS server did not reply to a query for name " );
		dbgtext( "%s.\nThis means it ", nmb_namestr(question_name) );
		dbgtext( "is probably not a Samba 1.9.18 or above WINS server.\n" );
	}
}

/****************************************************************************
 If we are a domain master browser on the unicast subnet, do a query to the
 WINS server for the *<1b> name. This will only work to a Samba WINS server,
 so ignore it if we fail. If we succeed, contact each of the IP addresses in
 turn and do a node status request to them. If this succeeds then look for a
 <1b> name in the reply - this is the workgroup name. Add this to the unicast
 subnet. This is expensive, so we only do this every 15 minutes.
**************************************************************************/

void collect_all_workgroup_names_from_wins_server(time_t t)
{
	static time_t lastrun = 0;
	struct work_record *work;

	/* Only do this if we are using a WINS server. */
	if(we_are_a_wins_client() == False)
		return;

	/* Check to see if we are a domain master browser on the unicast subnet. */
	if((work = find_workgroup_on_subnet( unicast_subnet, lp_workgroup())) == NULL) {
		if( DEBUGLVL( 0 ) ) {
			dbgtext( "collect_all_workgroup_names_from_wins_server:\n" );
			dbgtext( "Cannot find my workgroup %s ", lp_workgroup() );
			dbgtext( "on subnet %s.\n", unicast_subnet->subnet_name );
		}
		return;
	}

	if(!AM_DOMAIN_MASTER_BROWSER(work))
		return;

	if ((lastrun != 0) && (t < lastrun + (15 * 60)))
		return;
     
	lastrun = t;

	/* First, query for the *<1b> name from the WINS server. */
	query_name(unicast_subnet, "*", 0x1b,
		find_all_domain_master_names_query_success,
		find_all_domain_master_names_query_fail,
		NULL);
} 


/****************************************************************************
 If we are a domain master browser on the unicast subnet, do a regular sync
 with all other DMBs that we know of on that subnet.

To prevent exponential network traffic with large numbers of workgroups
we use a randomised system where sync probability is inversely proportional
to the number of known workgroups
**************************************************************************/

void sync_all_dmbs(time_t t)
{
	static time_t lastrun = 0;
	struct work_record *work;
	int count=0;

	/* Only do this if we are using a WINS server. */
	if(we_are_a_wins_client() == False)
		return;

	/* Check to see if we are a domain master browser on the
           unicast subnet. */
	work = find_workgroup_on_subnet(unicast_subnet, lp_workgroup());
	if (!work)
		return;

	if (!AM_DOMAIN_MASTER_BROWSER(work))
		return;

	if ((lastrun != 0) && (t < lastrun + (5 * 60)))
		return;
     
	/* count how many syncs we might need to do */
	for (work=unicast_subnet->workgrouplist; work; work = work->next) {
		if (strcmp(lp_workgroup(), work->work_group)) {
			count++;
		}
	}

	/* sync with a probability of 1/count */
	for (work=unicast_subnet->workgrouplist; work; work = work->next) {
		if (strcmp(lp_workgroup(), work->work_group)) {
			unstring dmb_name;

			if (((unsigned)sys_random()) % count != 0)
				continue;

			lastrun = t;

			if (!work->dmb_name.name[0]) {
				/* we don't know the DMB - assume it is
				   the same as the unicast local master */
				make_nmb_name(&work->dmb_name, 
					      work->local_master_browser_name,
					      0x20);
			}

			pull_ascii_nstring(dmb_name, sizeof(dmb_name), work->dmb_name.name);

			DEBUG(3,("Initiating DMB<->DMB sync with %s(%s)\n",
				 dmb_name, inet_ntoa(work->dmb_addr)));

			sync_browse_lists(work, 
					  dmb_name,
					  work->dmb_name.name_type, 
					  work->dmb_addr, False, False);
		}
	}
}
