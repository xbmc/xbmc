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

/* Election parameters. */
extern time_t StartupTime;

/****************************************************************************
  Send an election datagram packet.
**************************************************************************/

static void send_election_dgram(struct subnet_record *subrec, const char *workgroup_name,
                                uint32 criterion, int timeup,const char *server_name)
{
	pstring outbuf;
	unstring srv_name;
	char *p;

	DEBUG(2,("send_election_dgram: Sending election packet for workgroup %s on subnet %s\n",
		workgroup_name, subrec->subnet_name ));

	memset(outbuf,'\0',sizeof(outbuf));
	p = outbuf;
	SCVAL(p,0,ANN_Election); /* Election opcode. */
	p++;

	SCVAL(p,0,((criterion == 0 && timeup == 0) ? 0 : ELECTION_VERSION));
	SIVAL(p,1,criterion);
	SIVAL(p,5,timeup*1000); /* ms - Despite what the spec says. */
	p += 13;
	unstrcpy(srv_name, server_name);
	strupper_m(srv_name);
	/* The following call does UNIX -> DOS charset conversion. */
	pstrcpy_base(p, srv_name, outbuf);
	p = skip_string(p,1);
  
	send_mailslot(False, BROWSE_MAILSLOT, outbuf, PTR_DIFF(p,outbuf),
		global_myname(), 0,
		workgroup_name, 0x1e,
		subrec->bcast_ip, subrec->myip, DGRAM_PORT);
}

/*******************************************************************
 We found a current master browser on one of our broadcast interfaces.
******************************************************************/

static void check_for_master_browser_success(struct subnet_record *subrec,
                                 struct userdata_struct *userdata,
                                 struct nmb_name *answer_name,
                                 struct in_addr answer_ip, struct res_rec *rrec)
{
	unstring aname;
	pull_ascii_nstring(aname, sizeof(aname), answer_name->name);
	DEBUG(3,("check_for_master_browser_success: Local master browser for workgroup %s exists at \
IP %s (just checking).\n", aname, inet_ntoa(answer_ip) ));
}

/*******************************************************************
 We failed to find a current master browser on one of our broadcast interfaces.
******************************************************************/

static void check_for_master_browser_fail( struct subnet_record *subrec,
                                           struct response_record *rrec,
                                           struct nmb_name *question_name,
                                           int fail_code)
{
	unstring workgroup_name;
	struct work_record *work;

	pull_ascii_nstring(workgroup_name,sizeof(workgroup_name),question_name->name);

	work = find_workgroup_on_subnet(subrec, workgroup_name);
	if(work == NULL) {
		DEBUG(0,("check_for_master_browser_fail: Unable to find workgroup %s on subnet %s.=\n",
			workgroup_name, subrec->subnet_name ));
		return;
	}

	if (strequal(work->work_group, lp_workgroup())) {

		if (lp_local_master()) {
			/* We have discovered that there is no local master
				browser, and we are configured to initiate
				an election that we will participate in.
			*/
			DEBUG(2,("check_for_master_browser_fail: Forcing election on workgroup %s subnet %s\n",
				work->work_group, subrec->subnet_name ));

			/* Setting this means we will participate when the
				election is run in run_elections(). */
			work->needelection = True;
		} else {
			/* We need to force an election, because we are configured
				not to become the local master, but we still need one,
				having detected that one doesn't exist.
			*/
			send_election_dgram(subrec, work->work_group, 0, 0, "");
		}
	}
}

/*******************************************************************
  Ensure there is a local master browser for a workgroup on our
  broadcast interfaces.
******************************************************************/

void check_master_browser_exists(time_t t)
{
	static time_t lastrun=0;
	struct subnet_record *subrec;
	const char *workgroup_name = lp_workgroup();

	if (!lastrun)
		lastrun = t;

	if (t < (lastrun + (CHECK_TIME_MST_BROWSE * 60)))
		return;

	lastrun = t;

	dump_workgroups(False);

	for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
		struct work_record *work;

		for (work = subrec->workgrouplist; work; work = work->next) {
			if (strequal(work->work_group, workgroup_name) && !AM_LOCAL_MASTER_BROWSER(work)) {
				/* Do a name query for the local master browser on this net. */
				query_name( subrec, work->work_group, 0x1d,
					check_for_master_browser_success,
					check_for_master_browser_fail,
					NULL);
			}
		}
	}
}

/*******************************************************************
  Run an election.
******************************************************************/

void run_elections(time_t t)
{
	static time_t lastime = 0;
  
	struct subnet_record *subrec;
  
	START_PROFILE(run_elections);

	/* Send election packets once every 2 seconds - note */
	if (lastime && (t - lastime < 2)) {
		END_PROFILE(run_elections);
		return;
	}
  
	lastime = t;
  
	for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
		struct work_record *work;

		for (work = subrec->workgrouplist; work; work = work->next) {
			if (work->RunningElection) {
				/*
				 * We can only run an election for a workgroup if we have
				 * registered the WORKGROUP<1e> name, as that's the name
				 * we must listen to.
				 */
				struct nmb_name nmbname;

				make_nmb_name(&nmbname, work->work_group, 0x1e);
				if(find_name_on_subnet( subrec, &nmbname, FIND_SELF_NAME)==NULL) {
					DEBUG(8,("run_elections: Cannot send election packet yet as name %s not \
yet registered on subnet %s\n", nmb_namestr(&nmbname), subrec->subnet_name ));
					continue;
				}

				send_election_dgram(subrec, work->work_group, work->ElectionCriterion,
						t - StartupTime, global_myname());
	      
				if (work->ElectionCount++ >= 4) {
					/* Won election (4 packets were sent out uncontested. */
					DEBUG(2,("run_elections: >>> Won election for workgroup %s on subnet %s <<<\n",
						work->work_group, subrec->subnet_name ));

					work->RunningElection = False;

					become_local_master_browser(subrec, work);
				}
			}
		}
	}
	END_PROFILE(run_elections);
}

/*******************************************************************
  Determine if I win an election.
******************************************************************/

static BOOL win_election(struct work_record *work, int version,
                         uint32 criterion, int timeup, const char *server_name)
{  
	int mytimeup = time(NULL) - StartupTime;
	uint32 mycriterion = work->ElectionCriterion;

	/* If local master is false then never win in election broadcasts. */
	if(!lp_local_master()) {
		DEBUG(3,("win_election: Losing election as local master == False\n"));
		return False;
	}
 
	DEBUG(4,("win_election: election comparison: %x:%x %x:%x %d:%d %s:%s\n",
			version, ELECTION_VERSION,
			criterion, mycriterion,
			timeup, mytimeup,
			server_name, global_myname()));

	if (version > ELECTION_VERSION)
		return(False);
	if (version < ELECTION_VERSION)
		return(True);
  
	if (criterion > mycriterion)
		return(False);
	if (criterion < mycriterion)
		return(True);

	if (timeup > mytimeup)
		return(False);
	if (timeup < mytimeup)
		return(True);

	if (StrCaseCmp(global_myname(), server_name) > 0)
		return(False);
  
	return(True);
}

/*******************************************************************
  Process an incoming election datagram packet.
******************************************************************/

void process_election(struct subnet_record *subrec, struct packet_struct *p, char *buf)
{
	struct dgram_packet *dgram = &p->packet.dgram;
	int version = CVAL(buf,0);
	uint32 criterion = IVAL(buf,1);
	int timeup = IVAL(buf,5)/1000;
	unstring server_name;
	struct work_record *work;
	unstring workgroup_name;

	START_PROFILE(election);

	pull_ascii_nstring(server_name, sizeof(server_name), buf+13);
	pull_ascii_nstring(workgroup_name, sizeof(workgroup_name), dgram->dest_name.name);

	server_name[15] = 0;  

	DEBUG(3,("process_election: Election request from %s at IP %s on subnet %s for workgroup %s.\n",
		server_name,inet_ntoa(p->ip), subrec->subnet_name, workgroup_name ));

	DEBUG(5,("process_election: vers=%d criterion=%08x timeup=%d\n", version,criterion,timeup));

	if(( work = find_workgroup_on_subnet(subrec, workgroup_name)) == NULL) {
		DEBUG(0,("process_election: Cannot find workgroup %s on subnet %s.\n",
			workgroup_name, subrec->subnet_name ));
		goto done;
	}

	if (!strequal(work->work_group, lp_workgroup())) {
		DEBUG(3,("process_election: ignoring election request for workgroup %s on subnet %s as this \
is not my workgroup.\n", work->work_group, subrec->subnet_name ));
		goto done;
	}

	if (win_election(work, version,criterion,timeup,server_name)) {
		/* We take precedence over the requesting server. */
		if (!work->RunningElection) {
			/* We weren't running an election - start running one. */

			work->needelection = True;
			work->ElectionCount=0;
		}

		/* Note that if we were running an election for this workgroup on this
			subnet already, we just ignore the server we take precedence over. */
	} else {
		/* We lost. Stop participating. */
		work->needelection = False;

		if (work->RunningElection || AM_LOCAL_MASTER_BROWSER(work)) {
			work->RunningElection = False;
			DEBUG(3,("process_election: >>> Lost election for workgroup %s on subnet %s <<<\n",
				work->work_group, subrec->subnet_name ));
			if (AM_LOCAL_MASTER_BROWSER(work))
				unbecome_local_master_browser(subrec, work, False);
		}
	}
done:

	END_PROFILE(election);
}

/****************************************************************************
  This function looks over all the workgroups known on all the broadcast
  subnets and decides if a browser election is to be run on that workgroup.
  It returns True if any election packets need to be sent (this will then
  be done by run_elections().
***************************************************************************/

BOOL check_elections(void)
{
	struct subnet_record *subrec;
	BOOL run_any_election = False;

	for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
		struct work_record *work;
		for (work = subrec->workgrouplist; work; work = work->next) {
			run_any_election |= work->RunningElection;

			/* 
			 * Start an election if we have any chance of winning.
			 * Note this is a change to the previous code, that would
			 * only run an election if nmbd was in the potential browser
			 * state. We need to run elections in any state if we're told
			 * to. JRA.
			 */

			if (work->needelection && !work->RunningElection && lp_local_master()) {
				/*
				 * We can only run an election for a workgroup if we have
				 * registered the WORKGROUP<1e> name, as that's the name
				 * we must listen to.
				 */
				struct nmb_name nmbname;

				make_nmb_name(&nmbname, work->work_group, 0x1e);
				if(find_name_on_subnet( subrec, &nmbname, FIND_SELF_NAME)==NULL) {
					DEBUG(8,("check_elections: Cannot send election packet yet as name %s not \
yet registered on subnet %s\n", nmb_namestr(&nmbname), subrec->subnet_name ));
					continue;
				}

				DEBUG(3,("check_elections: >>> Starting election for workgroup %s on subnet %s <<<\n",
					work->work_group, subrec->subnet_name ));

				work->ElectionCount = 0;
				work->RunningElection = True;
				work->needelection = False;
			}
		}
	}
	return run_any_election;
}

/****************************************************************************
 Process a internal Samba message forcing an election.
***************************************************************************/

void nmbd_message_election(int msg_type, struct process_id src,
			   void *buf, size_t len)
{
	struct subnet_record *subrec;

	for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
		struct work_record *work;
		for (work = subrec->workgrouplist; work; work = work->next) {
			if (strequal(work->work_group, lp_workgroup())) {
				work->needelection = True;
				work->ElectionCount=0;
				work->mst_state = lp_local_master() ? MST_POTENTIAL : MST_NONE;
			}
		}
	}
}
