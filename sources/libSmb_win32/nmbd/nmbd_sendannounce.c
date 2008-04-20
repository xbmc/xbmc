/* 
   Unix SMB/CIFS implementation.
   NBT netbios routines and daemon - version 2
   Copyright (C) Andrew Tridgell 1994-1998
   Copyright (C) Luke Kenneth Casson Leighton 1994-1998
   Copyright (C) Jeremy Allison 1994-1998

   SMB Version handling
   Copyright (C) John H Terpstra 1995-1998
   
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

extern int  updatecount;
extern BOOL found_lm_clients;

/****************************************************************************
 Send a browser reset packet.
**************************************************************************/

void send_browser_reset(int reset_type, const char *to_name, int to_type, struct in_addr to_ip)
{
	char outbuf[PSTRING_LEN];
	char *p;

	DEBUG(3,("send_browser_reset: sending reset request type %d to %s<%02x> IP %s.\n",
		reset_type, to_name, to_type, inet_ntoa(to_ip) ));

	memset(outbuf,'\0',sizeof(outbuf));
	p = outbuf;
	SCVAL(p,0,ANN_ResetBrowserState);
	p++;
	SCVAL(p,0,reset_type);
	p++;

	send_mailslot(True, BROWSE_MAILSLOT, outbuf,PTR_DIFF(p,outbuf),
		global_myname(), 0x0, to_name, to_type, to_ip, 
		FIRST_SUBNET->myip, DGRAM_PORT);
}

/****************************************************************************
  Broadcast a packet to the local net requesting that all servers in this
  workgroup announce themselves to us.
  **************************************************************************/

void broadcast_announce_request(struct subnet_record *subrec, struct work_record *work)
{
	char outbuf[PSTRING_LEN];
	char *p;

	work->needannounce = True;

	DEBUG(3,("broadcast_announce_request: sending announce request for workgroup %s \
to subnet %s\n", work->work_group, subrec->subnet_name));

	memset(outbuf,'\0',sizeof(outbuf));
	p = outbuf;
	SCVAL(p,0,ANN_AnnouncementRequest);
	p++;

	SCVAL(p,0,work->token); /* (local) Unique workgroup token id. */
	p++;
	p +=  push_string(NULL, p+1, global_myname(), 15, STR_ASCII|STR_UPPER|STR_TERMINATE);
  
	send_mailslot(False, BROWSE_MAILSLOT, outbuf,PTR_DIFF(p,outbuf),
		global_myname(), 0x0, work->work_group,0x1e, subrec->bcast_ip, 
		subrec->myip, DGRAM_PORT);
}

/****************************************************************************
  Broadcast an announcement.
  **************************************************************************/

static void send_announcement(struct subnet_record *subrec, int announce_type,
                              const char *from_name, const char *to_name, int to_type, struct in_addr to_ip,
                              time_t announce_interval,
                              const char *server_name, int server_type, const char *server_comment)
{
	char outbuf[PSTRING_LEN];
	unstring upper_server_name;
	char *p;

	memset(outbuf,'\0',sizeof(outbuf));
	p = outbuf+1;

	SCVAL(outbuf,0,announce_type);

	/* Announcement parameters. */
	SCVAL(p,0,updatecount);
	SIVAL(p,1,announce_interval*1000); /* Milliseconds - despite the spec. */

	safe_strcpy(upper_server_name, server_name, sizeof(upper_server_name)-1);
	strupper_m(upper_server_name);
	push_string(NULL, p+5, upper_server_name, 16, STR_ASCII|STR_TERMINATE);

	SCVAL(p,21,lp_major_announce_version()); /* Major version. */
	SCVAL(p,22,lp_minor_announce_version()); /* Minor version. */

	SIVAL(p,23,server_type & ~SV_TYPE_LOCAL_LIST_ONLY);
	/* Browse version: got from NT/AS 4.00  - Value defined in smb.h (JHT). */
	SSVAL(p,27,BROWSER_ELECTION_VERSION);
	SSVAL(p,29,BROWSER_CONSTANT); /* Browse signature. */

	p += 31 + push_string(NULL, p+31, server_comment, sizeof(outbuf) - (p + 31 - outbuf), STR_ASCII|STR_TERMINATE);

	send_mailslot(False,BROWSE_MAILSLOT, outbuf, PTR_DIFF(p,outbuf),
			from_name, 0x0, to_name, to_type, to_ip, subrec->myip,
			DGRAM_PORT);
}

/****************************************************************************
  Broadcast a LanMan announcement.
**************************************************************************/

static void send_lm_announcement(struct subnet_record *subrec, int announce_type,
                              char *from_name, char *to_name, int to_type, struct in_addr to_ip,
                              time_t announce_interval,
                              char *server_name, int server_type, char *server_comment)
{
	char outbuf[PSTRING_LEN];
	char *p=outbuf;

	memset(outbuf,'\0',sizeof(outbuf));

	SSVAL(p,0,announce_type);
	SIVAL(p,2,server_type & ~SV_TYPE_LOCAL_LIST_ONLY);
	SCVAL(p,6,lp_major_announce_version()); /* Major version. */
	SCVAL(p,7,lp_minor_announce_version()); /* Minor version. */
	SSVAL(p,8,announce_interval);            /* In seconds - according to spec. */

	p += 10;
	p += push_string(NULL, p, server_name, 15, STR_ASCII|STR_UPPER|STR_TERMINATE);
	p += push_string(NULL, p, server_comment, sizeof(outbuf)- (p - outbuf), STR_ASCII|STR_UPPER|STR_TERMINATE);

	send_mailslot(False,LANMAN_MAILSLOT, outbuf, PTR_DIFF(p,outbuf),
		from_name, 0x0, to_name, to_type, to_ip, subrec->myip,
		DGRAM_PORT);
}

/****************************************************************************
 We are a local master browser. Announce this to WORKGROUP<1e>.
****************************************************************************/

static void send_local_master_announcement(struct subnet_record *subrec, struct work_record *work,
                                           struct server_record *servrec)
{
	/* Ensure we don't have the prohibited bit set. */
	uint32 type = servrec->serv.type & ~SV_TYPE_LOCAL_LIST_ONLY;

	DEBUG(3,("send_local_master_announcement: type %x for name %s on subnet %s for workgroup %s\n",
		type, global_myname(), subrec->subnet_name, work->work_group));

	send_announcement(subrec, ANN_LocalMasterAnnouncement,
			global_myname(),                 /* From nbt name. */
			work->work_group, 0x1e,          /* To nbt name. */
			subrec->bcast_ip,                /* To ip. */
			work->announce_interval,         /* Time until next announce. */
			global_myname(),                 /* Name to announce. */
			type,                            /* Type field. */
			servrec->serv.comment);
}

/****************************************************************************
 Announce the workgroup WORKGROUP to MSBROWSE<01>.
****************************************************************************/

static void send_workgroup_announcement(struct subnet_record *subrec, struct work_record *work)
{
	DEBUG(3,("send_workgroup_announcement: on subnet %s for workgroup %s\n",
		subrec->subnet_name, work->work_group));

	send_announcement(subrec, ANN_DomainAnnouncement,
			global_myname(),                 /* From nbt name. */
			MSBROWSE, 0x1,                   /* To nbt name. */
			subrec->bcast_ip,                /* To ip. */
			work->announce_interval,         /* Time until next announce. */
			work->work_group,                /* Name to announce. */
			SV_TYPE_DOMAIN_ENUM|SV_TYPE_NT,  /* workgroup announce flags. */
			global_myname());                /* From name as comment. */
}

/****************************************************************************
 Announce the given host to WORKGROUP<1d>.
****************************************************************************/

static void send_host_announcement(struct subnet_record *subrec, struct work_record *work,
                                   struct server_record *servrec)
{
	/* Ensure we don't have the prohibited bits set. */
	uint32 type = servrec->serv.type & ~SV_TYPE_LOCAL_LIST_ONLY;

	DEBUG(3,("send_host_announcement: type %x for host %s on subnet %s for workgroup %s\n",
		type, servrec->serv.name, subrec->subnet_name, work->work_group));

	send_announcement(subrec, ANN_HostAnnouncement,
			servrec->serv.name,              /* From nbt name. */
			work->work_group, 0x1d,          /* To nbt name. */
			subrec->bcast_ip,                /* To ip. */
			work->announce_interval,         /* Time until next announce. */
			servrec->serv.name,              /* Name to announce. */
			type,                            /* Type field. */
			servrec->serv.comment);
}

/****************************************************************************
 Announce the given LanMan host
****************************************************************************/

static void send_lm_host_announcement(struct subnet_record *subrec, struct work_record *work,
                                   struct server_record *servrec, int lm_interval)
{
	/* Ensure we don't have the prohibited bits set. */
	uint32 type = servrec->serv.type & ~SV_TYPE_LOCAL_LIST_ONLY;

	DEBUG(3,("send_lm_host_announcement: type %x for host %s on subnet %s for workgroup %s, ttl: %d\n",
		type, servrec->serv.name, subrec->subnet_name, work->work_group, lm_interval));

	send_lm_announcement(subrec, ANN_HostAnnouncement,
			servrec->serv.name,              /* From nbt name. */
			work->work_group, 0x00,          /* To nbt name. */
			subrec->bcast_ip,                /* To ip. */
			lm_interval,                     /* Time until next announce. */
			servrec->serv.name,              /* Name to announce (fstring not netbios name struct). */
			type,                            /* Type field. */
			servrec->serv.comment);
}

/****************************************************************************
  Announce a server record.
  ****************************************************************************/

static void announce_server(struct subnet_record *subrec, struct work_record *work,
                     struct server_record *servrec)
{
	/* Only do domain announcements if we are a master and it's
		our primary name we're being asked to announce. */

	if (AM_LOCAL_MASTER_BROWSER(work) && strequal(global_myname(),servrec->serv.name)) {
		send_local_master_announcement(subrec, work, servrec);
		send_workgroup_announcement(subrec, work);
	} else {
		send_host_announcement(subrec, work, servrec);
	}
}

/****************************************************************************
  Go through all my registered names on all broadcast subnets and announce
  them if the timeout requires it.
  **************************************************************************/

void announce_my_server_names(time_t t)
{
	struct subnet_record *subrec;

	for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
		struct work_record *work = find_workgroup_on_subnet(subrec, lp_workgroup());

		if(work) {
			struct server_record *servrec;

			if (work->needannounce) {
				/* Drop back to a max 3 minute announce. This is to prevent a
					single lost packet from breaking things for too long. */

				work->announce_interval = MIN(work->announce_interval,
							CHECK_TIME_MIN_HOST_ANNCE*60);
				work->lastannounce_time = t - (work->announce_interval+1);
				work->needannounce = False;
			}

			/* Announce every minute at first then progress to every 12 mins */
			if ((t - work->lastannounce_time) < work->announce_interval)
				continue;

			if (work->announce_interval < (CHECK_TIME_MAX_HOST_ANNCE * 60))
				work->announce_interval += 60;

			work->lastannounce_time = t;

			for (servrec = work->serverlist; servrec; servrec = servrec->next) {
				if (is_myname(servrec->serv.name))
					announce_server(subrec, work, servrec);
			}
		} /* if work */
	} /* for subrec */
}

/****************************************************************************
  Go through all my registered names on all broadcast subnets and announce
  them as a LanMan server if the timeout requires it.
**************************************************************************/

void announce_my_lm_server_names(time_t t)
{
	struct subnet_record *subrec;
	static time_t last_lm_announce_time=0;
	int announce_interval = lp_lm_interval();
	int lm_announce = lp_lm_announce();

	if ((announce_interval <= 0) || (lm_announce <= 0)) {
		/* user absolutely does not want LM announcements to be sent. */
		return;
	}

	if ((lm_announce >= 2) && (!found_lm_clients)) {
		/* has been set to 2 (Auto) but no LM clients detected (yet). */
		return;
	}

	/* Otherwise: must have been set to 1 (Yes), or LM clients *have*
		been detected. */

	for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
		struct work_record *work = find_workgroup_on_subnet(subrec, lp_workgroup());

		if(work) {
			struct server_record *servrec;

			if (last_lm_announce_time && ((t - last_lm_announce_time) < announce_interval ))
				continue;

			last_lm_announce_time = t;

			for (servrec = work->serverlist; servrec; servrec = servrec->next) {
				if (is_myname(servrec->serv.name))
					/* skipping equivalent of announce_server() */
					send_lm_host_announcement(subrec, work, servrec, announce_interval);
			}
		} /* if work */
	} /* for subrec */
}

/* Announce timer. Moved into global static so it can be reset
   when a machine becomes a local master browser. */
static time_t announce_timer_last=0;

/****************************************************************************
 Reset the announce_timer so that a local master browser announce will be done
 immediately.
 ****************************************************************************/

void reset_announce_timer(void)
{
	announce_timer_last = time(NULL) - (CHECK_TIME_MST_ANNOUNCE * 60);
}

/****************************************************************************
  Announce myself as a local master browser to a domain master browser.
  **************************************************************************/

void announce_myself_to_domain_master_browser(time_t t)
{
	struct subnet_record *subrec;
	struct work_record *work;

	if(!we_are_a_wins_client()) {
		DEBUG(10,("announce_myself_to_domain_master_browser: no unicast subnet, ignoring.\n"));
		return;
	}

	if (!announce_timer_last)
		announce_timer_last = t;

	if ((t-announce_timer_last) < (CHECK_TIME_MST_ANNOUNCE * 60)) {
		DEBUG(10,("announce_myself_to_domain_master_browser: t (%d) - last(%d) < %d\n",
			(int)t, (int)announce_timer_last, 
			CHECK_TIME_MST_ANNOUNCE * 60 ));
		return;
	}

	announce_timer_last = t;

	/* Look over all our broadcast subnets to see if any of them
		has the state set as local master browser. */

	for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
		for (work = subrec->workgrouplist; work; work = work->next) {
			if (AM_LOCAL_MASTER_BROWSER(work)) {
				DEBUG(4,( "announce_myself_to_domain_master_browser: I am a local master browser for \
workgroup %s on subnet %s\n", work->work_group, subrec->subnet_name));

				/* Look in nmbd_browsersync.c for the rest of this code. */
				announce_and_sync_with_domain_master_browser(subrec, work);
			}
		}
	}
}

/****************************************************************************
Announce all samba's server entries as 'gone'.
This must *only* be called on shutdown.
****************************************************************************/

void announce_my_servers_removed(void)
{
	int announce_interval = lp_lm_interval();
	int lm_announce = lp_lm_announce();
	struct subnet_record *subrec; 

	for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
		struct work_record *work;
		for (work = subrec->workgrouplist; work; work = work->next) {
			struct server_record *servrec;

			work->announce_interval = 0;
			for (servrec = work->serverlist; servrec; servrec = servrec->next) {
				if (!is_myname(servrec->serv.name))
					continue;
				servrec->serv.type = 0;
				if(AM_LOCAL_MASTER_BROWSER(work))
					send_local_master_announcement(subrec, work, servrec);
				send_host_announcement(subrec, work, servrec);

				if ((announce_interval <= 0) || (lm_announce <= 0)) {
					/* user absolutely does not want LM announcements to be sent. */
					continue;
				}

				if ((lm_announce >= 2) && (!found_lm_clients)) {
					/* has been set to 2 (Auto) but no LM clients detected (yet). */
					continue;
				}

				/* 
				 * lm announce was set or we have seen lm announcements, so do
				 * a lm announcement of host removed.
				 */

				send_lm_host_announcement(subrec, work, servrec, 0);
			}
		}
	}
}

/****************************************************************************
  Do all the "remote" announcements. These are used to put ourselves
  on a remote browse list. They are done blind, no checking is done to
  see if there is actually a local master browser at the other end.
  **************************************************************************/

void announce_remote(time_t t)
{
	char *s;
	const char *ptr;
	static time_t last_time = 0;
	pstring s2;
	struct in_addr addr;
	char *comment;
	int stype = lp_default_server_announce();

	if (last_time && (t < (last_time + REMOTE_ANNOUNCE_INTERVAL)))
		return;

	last_time = t;

	s = lp_remote_announce();
	if (!*s)
		return;

	comment = string_truncate(lp_serverstring(), MAX_SERVER_STRING_LENGTH);

	for (ptr=s; next_token(&ptr,s2,NULL,sizeof(s2)); ) {
		/* The entries are of the form a.b.c.d/WORKGROUP with 
				WORKGROUP being optional */
		const char *wgroup;
		char *pwgroup;
		int i;

		pwgroup = strchr_m(s2,'/');
		if (pwgroup)
			*pwgroup++ = 0;
		if (!pwgroup || !*pwgroup)
			wgroup = lp_workgroup();
		else
			wgroup = pwgroup;

		addr = *interpret_addr2(s2);
    
		/* Announce all our names including aliases */
		/* Give the ip address as the address of our first
				broadcast subnet. */

		for(i=0; my_netbios_names(i); i++) {
			const char *name = my_netbios_names(i);

			DEBUG(5,("announce_remote: Doing remote announce for server %s to IP %s.\n",
				name, inet_ntoa(addr) ));

			send_announcement(FIRST_SUBNET, ANN_HostAnnouncement,
						name,                      /* From nbt name. */
						wgroup, 0x1d,              /* To nbt name. */
						addr,                      /* To ip. */
						REMOTE_ANNOUNCE_INTERVAL,  /* Time until next announce. */
						name,                      /* Name to announce. */
						stype,                     /* Type field. */
						comment);
		}
	}
}

/****************************************************************************
  Implement the 'remote browse sync' feature Andrew added.
  These are used to put our browse lists into remote browse lists.
**************************************************************************/

void browse_sync_remote(time_t t)
{  
	char *s;
	const char *ptr;
	static time_t last_time = 0; 
	pstring s2;
	struct in_addr addr;
	struct work_record *work;
	pstring outbuf;
	char *p;
	unstring myname;
 
	if (last_time && (t < (last_time + REMOTE_ANNOUNCE_INTERVAL)))
		return;
   
	last_time = t;

	s = lp_remote_browse_sync();
	if (!*s)
		return;

	/*
	 * We only do this if we are the local master browser
	 * for our workgroup on the firsst subnet.
	 */

	if((work = find_workgroup_on_subnet(FIRST_SUBNET, lp_workgroup())) == NULL) {   
		DEBUG(0,("browse_sync_remote: Cannot find workgroup %s on subnet %s\n",
			lp_workgroup(), FIRST_SUBNET->subnet_name ));
		return;
	}
         
	if(!AM_LOCAL_MASTER_BROWSER(work)) {
		DEBUG(5,("browse_sync_remote: We can only do this if we are a local master browser \
for workgroup %s on subnet %s.\n", lp_workgroup(), FIRST_SUBNET->subnet_name ));
		return;
	} 

	memset(outbuf,'\0',sizeof(outbuf));
	p = outbuf;
	SCVAL(p,0,ANN_MasterAnnouncement);
	p++;

	unstrcpy(myname, global_myname());
	strupper_m(myname);
	myname[15]='\0';
	push_pstring_base(p, myname, outbuf);

	p = skip_string(p,1);

	for (ptr=s; next_token(&ptr,s2,NULL,sizeof(s2)); ) {
		/* The entries are of the form a.b.c.d */
		addr = *interpret_addr2(s2);

		DEBUG(5,("announce_remote: Doing remote browse sync announce for server %s to IP %s.\n",
			global_myname(), inet_ntoa(addr) ));

		send_mailslot(True, BROWSE_MAILSLOT, outbuf,PTR_DIFF(p,outbuf),
			global_myname(), 0x0, "*", 0x0, addr, FIRST_SUBNET->myip, DGRAM_PORT);
	}
}
