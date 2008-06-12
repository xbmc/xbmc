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

extern BOOL found_lm_clients;

#if 0

/* XXXX note: This function is currently unsuitable for use, as it
   does not properly check that a server is in a fit state to become
   a backup browser before asking it to be one.
   The code is left here to be worked on at a later date.
*/

/****************************************************************************
Tell a server to become a backup browser
**************************************************************************/

void tell_become_backup(void)
{
  struct subnet_record *subrec;
  for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec))
  {
    struct work_record *work;
    for (work = subrec->workgrouplist; work; work = work->next)
    {
      struct server_record *servrec;
      int num_servers = 0;
      int num_backups = 0;
	  
      for (servrec = work->serverlist; servrec; servrec = servrec->next)
      {
        num_servers++;
	      
        if (is_myname(servrec->serv.name))
          continue;
	      
        if (servrec->serv.type & SV_TYPE_BACKUP_BROWSER) 
        {
          num_backups++;
          continue;
        }
	      
        if (servrec->serv.type & SV_TYPE_MASTER_BROWSER)
          continue;
	      
        if (!(servrec->serv.type & SV_TYPE_POTENTIAL_BROWSER))
          continue;
	      
        DEBUG(3,("num servers: %d num backups: %d\n", 
              num_servers, num_backups));
	      
        /* make first server a backup server. thereafter make every
           tenth server a backup server */
        if (num_backups != 0 && (num_servers+9) / num_backups > 10)
          continue;
	      
        DEBUG(2,("sending become backup to %s %s for %s\n",
             servrec->serv.name, inet_ntoa(subrec->bcast_ip),
             work->work_group));
	      
        /* type 11 request from MYNAME(20) to WG(1e) for SERVER */
        do_announce_request(servrec->serv.name, work->work_group,
              ANN_BecomeBackup, 0x20, 0x1e, subrec->bcast_ip);
      }
    }
  }
}
#endif

/*******************************************************************
  Process an incoming host announcement packet.
*******************************************************************/

void process_host_announce(struct subnet_record *subrec, struct packet_struct *p, char *buf)
{
	struct dgram_packet *dgram = &p->packet.dgram;
	int ttl = IVAL(buf,1)/1000;
	unstring announce_name;
	uint32 servertype = IVAL(buf,23);
	fstring comment;
	struct work_record *work;
	struct server_record *servrec;
	unstring work_name;
	unstring source_name;

	START_PROFILE(host_announce);

	pull_ascii_fstring(comment, buf+31);
  
	pull_ascii_nstring(announce_name, sizeof(announce_name), buf+5);
	pull_ascii_nstring(source_name, sizeof(source_name), dgram->source_name.name);

	DEBUG(3,("process_host_announce: from %s<%02x> IP %s to \
%s for server %s.\n", source_name, source_name[15], inet_ntoa(p->ip),
			nmb_namestr(&dgram->dest_name),announce_name));

	DEBUG(5,("process_host_announce: ttl=%d server type=%08x comment=%s\n",
		ttl, servertype,comment));

	/* Filter servertype to remove impossible bits. */
	servertype &= ~(SV_TYPE_LOCAL_LIST_ONLY|SV_TYPE_DOMAIN_ENUM);

	/* A host announcement must be sent to the name WORKGROUP<1d>. */
	if(dgram->dest_name.name_type != 0x1d) {
		DEBUG(2,("process_host_announce: incorrect name type for destination from IP %s \
(was %02x) should be 0x1d. Allowing packet anyway.\n",
			inet_ntoa(p->ip), dgram->dest_name.name_type));
		/* Change it so it was. */
		dgram->dest_name.name_type = 0x1d;
	}

	/* For a host announce the workgroup name is the destination name. */
	pull_ascii_nstring(work_name, sizeof(work_name), dgram->dest_name.name);

	/*
	 * Syntax servers version 5.1 send HostAnnounce packets to
	 * *THE WRONG NAME*. They send to LOCAL_MASTER_BROWSER_NAME<00>
	 * instead of WORKGROUP<1d> name. So to fix this we check if
	 * the workgroup name is our own name, and if so change it
	 * to be our primary workgroup name.
	 */

	if(strequal(work_name, global_myname()))
		unstrcpy(work_name,lp_workgroup());

	/*
	 * We are being very agressive here in adding a workgroup
	 * name on the basis of a host announcing itself as being
	 * in that workgroup. Maybe we should wait for the workgroup
	 * announce instead ? JRA.
	 */

	work = find_workgroup_on_subnet(subrec, work_name);

	if(servertype != 0) {
		if (work ==NULL ) {
			/* We have no record of this workgroup. Add it. */
			if((work = create_workgroup_on_subnet(subrec, work_name, ttl))==NULL)
				goto done;
		}
  
		if((servrec = find_server_in_workgroup( work, announce_name))==NULL) {
			/* If this server is not already in the workgroup, add it. */
			create_server_on_workgroup(work, announce_name, 
				servertype|SV_TYPE_LOCAL_LIST_ONLY, 
				ttl, comment);
		} else {
			/* Update the record. */
			servrec->serv.type = servertype|SV_TYPE_LOCAL_LIST_ONLY;
			update_server_ttl( servrec, ttl);
			fstrcpy(servrec->serv.comment,comment);
		}
	} else {
		/*
		 * This server is announcing it is going down. Remove it from the 
		 * workgroup.
		 */
		if(!is_myname(announce_name) && (work != NULL) &&
				((servrec = find_server_in_workgroup( work, announce_name))!=NULL)) {
			remove_server_from_workgroup( work, servrec);
		}
	}

	subrec->work_changed = True;
done:

	END_PROFILE(host_announce);
}

/*******************************************************************
  Process an incoming WORKGROUP announcement packet.
*******************************************************************/

void process_workgroup_announce(struct subnet_record *subrec, struct packet_struct *p, char *buf)
{
	struct dgram_packet *dgram = &p->packet.dgram;
	int ttl = IVAL(buf,1)/1000;
	unstring workgroup_announce_name;
	unstring master_name;
	uint32 servertype = IVAL(buf,23);
	struct work_record *work;
	unstring source_name;
	unstring dest_name;

	START_PROFILE(workgroup_announce);

	pull_ascii_nstring(workgroup_announce_name,sizeof(workgroup_announce_name),buf+5);
	pull_ascii_nstring(master_name,sizeof(master_name),buf+31);
	pull_ascii_nstring(source_name,sizeof(source_name),dgram->source_name.name);
	pull_ascii_nstring(dest_name,sizeof(dest_name),dgram->dest_name.name);

	DEBUG(3,("process_workgroup_announce: from %s<%02x> IP %s to \
%s for workgroup %s.\n", source_name, source_name[15], inet_ntoa(p->ip),
			nmb_namestr(&dgram->dest_name),workgroup_announce_name));

	DEBUG(5,("process_workgroup_announce: ttl=%d server type=%08x master browser=%s\n",
		ttl, servertype, master_name));

	/* Workgroup announcements must only go to the MSBROWSE name. */
	if (!strequal(dest_name, MSBROWSE) || (dgram->dest_name.name_type != 0x1)) {
		DEBUG(0,("process_workgroup_announce: from IP %s should be to __MSBROWSE__<0x01> not %s\n",
			inet_ntoa(p->ip), nmb_namestr(&dgram->dest_name)));
		goto done;
	}

	if ((work = find_workgroup_on_subnet(subrec, workgroup_announce_name))==NULL) {
		/* We have no record of this workgroup. Add it. */
		if((work = create_workgroup_on_subnet(subrec, workgroup_announce_name, ttl))==NULL)
			goto done;
	} else {
		/* Update the workgroup death_time. */
		update_workgroup_ttl(work, ttl);
	}

	if(*work->local_master_browser_name == '\0') {
		/* Set the master browser name. */
		set_workgroup_local_master_browser_name( work, master_name );
	}

	subrec->work_changed = True;

done:

	END_PROFILE(workgroup_announce);
}

/*******************************************************************
  Process an incoming local master browser announcement packet.
*******************************************************************/

void process_local_master_announce(struct subnet_record *subrec, struct packet_struct *p, char *buf)
{
	struct dgram_packet *dgram = &p->packet.dgram;
	int ttl = IVAL(buf,1)/1000;
	unstring server_name;
	uint32 servertype = IVAL(buf,23);
	fstring comment;
	unstring work_name;
	struct work_record *work;
	struct server_record *servrec;
	unstring source_name;

	START_PROFILE(local_master_announce);

	pull_ascii_nstring(server_name,sizeof(server_name),buf+5);
	pull_ascii_fstring(comment, buf+31);
	pull_ascii_nstring(source_name, sizeof(source_name), dgram->source_name.name);
	pull_ascii_nstring(work_name, sizeof(work_name), dgram->dest_name.name);

	DEBUG(3,("process_local_master_announce: from %s<%02x> IP %s to \
%s for server %s.\n", source_name, source_name[15], inet_ntoa(p->ip),
		nmb_namestr(&dgram->dest_name),server_name));

	DEBUG(5,("process_local_master_announce: ttl=%d server type=%08x comment=%s\n",
		ttl, servertype, comment));

	/* A local master announcement must be sent to the name WORKGROUP<1e>. */
	if(dgram->dest_name.name_type != 0x1e) {
		DEBUG(0,("process_local_master_announce: incorrect name type for destination from IP %s \
(was %02x) should be 0x1e. Ignoring packet.\n",
			inet_ntoa(p->ip), dgram->dest_name.name_type));
		goto done;
	}

	/* Filter servertype to remove impossible bits. */
	servertype &= ~(SV_TYPE_LOCAL_LIST_ONLY|SV_TYPE_DOMAIN_ENUM);

	/* For a local master announce the workgroup name is the destination name. */

	if ((work = find_workgroup_on_subnet(subrec, work_name))==NULL) {
		/* Don't bother adding if it's a local master release announce. */
		if(servertype == 0)
			goto done;

		/* We have no record of this workgroup. Add it. */
		if((work = create_workgroup_on_subnet(subrec, work_name, ttl))==NULL)
			goto done;
	}

	/* If we think we're the local master browser for this workgroup,
		we should never have got this packet. We don't see our own
		packets.
	*/
	if(AM_LOCAL_MASTER_BROWSER(work)) {
		DEBUG(0,("process_local_master_announce: Server %s at IP %s is announcing itself as \
a local master browser for workgroup %s and we think we are master. Forcing election.\n",
			server_name, inet_ntoa(p->ip), work_name));

		/* Samba nmbd versions 1.9.17 to 1.9.17p4 have a bug in that when
		 they have become a local master browser once, they will never
		 stop sending local master announcements. To fix this we send
		 them a reset browser packet, with level 0x2 on the __SAMBA__
		 name that only they should be listening to. */
   
		send_browser_reset( 0x2, "__SAMBA__" , 0x20, p->ip);

		/* We should demote ourself and force an election. */

		unbecome_local_master_browser( subrec, work, True);

		/* The actual election requests are handled in nmbd_election.c */
		goto done;
	}  

	/* Find the server record on this workgroup. If it doesn't exist, add it. */

	if(servertype != 0) {
		if((servrec = find_server_in_workgroup( work, server_name))==NULL) {
			/* If this server is not already in the workgroup, add it. */
			create_server_on_workgroup(work, server_name, 
				servertype|SV_TYPE_LOCAL_LIST_ONLY, 
				ttl, comment);
		} else {
			/* Update the record. */
			servrec->serv.type = servertype|SV_TYPE_LOCAL_LIST_ONLY;
			update_server_ttl(servrec, ttl);
			fstrcpy(servrec->serv.comment,comment);
		}
	
		set_workgroup_local_master_browser_name( work, server_name );
	} else {
		/*
		 * This server is announcing it is going down. Remove it from the
		 * workgroup.
		 */
		if(!is_myname(server_name) && (work != NULL) &&
				((servrec = find_server_in_workgroup( work, server_name))!=NULL)) {
			remove_server_from_workgroup( work, servrec);
		}
	}

	subrec->work_changed = True;
done:

	END_PROFILE(local_master_announce);
}

/*******************************************************************
  Process a domain master announcement frame.
  Domain master browsers receive these from local masters. The Domain
  master should then issue a sync with the local master, asking for
  that machines local server list.
******************************************************************/

void process_master_browser_announce(struct subnet_record *subrec, 
                                     struct packet_struct *p,char *buf)
{
	unstring local_master_name;
	struct work_record *work;
	struct browse_cache_record *browrec;

	START_PROFILE(master_browser_announce);

	pull_ascii_nstring(local_master_name,sizeof(local_master_name),buf);
  
	DEBUG(3,("process_master_browser_announce: Local master announce from %s IP %s.\n",
		local_master_name, inet_ntoa(p->ip)));
  
	if (!lp_domain_master()) {
		DEBUG(0,("process_master_browser_announce: Not configured as domain \
master - ignoring master announce.\n"));
		goto done;
	}
  
	if((work = find_workgroup_on_subnet(subrec, lp_workgroup())) == NULL) {
		DEBUG(0,("process_master_browser_announce: Cannot find workgroup %s on subnet %s\n",
			lp_workgroup(), subrec->subnet_name));
		goto done;
	}

	if(!AM_DOMAIN_MASTER_BROWSER(work)) {
		DEBUG(0,("process_master_browser_announce: Local master announce made to us from \
%s IP %s and we are not a domain master browser.\n", local_master_name, inet_ntoa(p->ip)));
		goto done;
	}

	/* Add this host as a local master browser entry on the browse lists.
		This causes a sync request to be made to it at a later date.
	*/

	if((browrec = find_browser_in_lmb_cache( local_master_name )) == NULL) {
		/* Add it. */
		create_browser_in_lmb_cache( work->work_group, local_master_name, p->ip);
	} else {
		update_browser_death_time(browrec);
	}

done:

	END_PROFILE(master_browser_announce);
}

/*******************************************************************
  Process an incoming LanMan host announcement packet.
*******************************************************************/

void process_lm_host_announce(struct subnet_record *subrec, struct packet_struct *p, char *buf)
{
	struct dgram_packet *dgram = &p->packet.dgram;
	uint32 servertype = IVAL(buf,1);
	int osmajor=CVAL(buf,5);           /* major version of node software */
	int osminor=CVAL(buf,6);           /* minor version of node software */
	int ttl = SVAL(buf,7);
	unstring announce_name;
	struct work_record *work;
	struct server_record *servrec;
	unstring work_name;
	unstring source_name;
	fstring comment;
	char *s = buf+9;

	START_PROFILE(lm_host_announce);
	s = skip_string(s,1);
	pull_ascii(comment, s, sizeof(fstring), 43, STR_TERMINATE);

	pull_ascii_nstring(announce_name,sizeof(announce_name),buf+9);
	pull_ascii_nstring(source_name,sizeof(source_name),dgram->source_name.name);
	/* For a LanMan host announce the workgroup name is the destination name. */
	pull_ascii_nstring(work_name,sizeof(work_name),dgram->dest_name.name);

	DEBUG(3,("process_lm_host_announce: LM Announcement from %s IP %s to \
%s for server %s.\n", nmb_namestr(&dgram->source_name), inet_ntoa(p->ip),
		nmb_namestr(&dgram->dest_name),announce_name));

	DEBUG(5,("process_lm_host_announce: os=(%d,%d) ttl=%d server type=%08x comment=%s\n",
		osmajor, osminor, ttl, servertype,comment));

	if ((osmajor < 36) || (osmajor > 38) || (osminor !=0)) {
		DEBUG(5,("process_lm_host_announce: LM Announcement packet does not \
originate from OS/2 Warp client. Ignoring packet.\n"));
		/* Could have been from a Windows machine (with its LM Announce enabled),
			or a Samba server. Then don't disrupt the current browse list. */
		goto done;
	}

	/* Filter servertype to remove impossible bits. */
	servertype &= ~(SV_TYPE_LOCAL_LIST_ONLY|SV_TYPE_DOMAIN_ENUM);

	/* A LanMan host announcement must be sent to the name WORKGROUP<00>. */
	if(dgram->dest_name.name_type != 0x00) {
		DEBUG(2,("process_lm_host_announce: incorrect name type for destination from IP %s \
(was %02x) should be 0x00. Allowing packet anyway.\n",
			inet_ntoa(p->ip), dgram->dest_name.name_type));
		/* Change it so it was. */
		dgram->dest_name.name_type = 0x00;
	}

	/*
	 * Syntax servers version 5.1 send HostAnnounce packets to
	 * *THE WRONG NAME*. They send to LOCAL_MASTER_BROWSER_NAME<00>
	 * instead of WORKGROUP<1d> name. So to fix this we check if
	 * the workgroup name is our own name, and if so change it
	 * to be our primary workgroup name. This code is probably
	 * not needed in the LanMan announce code, but it won't hurt.
	 */

	if(strequal(work_name, global_myname()))
		unstrcpy(work_name,lp_workgroup());

	/*
	 * We are being very agressive here in adding a workgroup
	 * name on the basis of a host announcing itself as being
	 * in that workgroup. Maybe we should wait for the workgroup
	 * announce instead ? JRA.
	 */

	work = find_workgroup_on_subnet(subrec, work_name);

	if(servertype != 0) {
		if (work == NULL) {
			/* We have no record of this workgroup. Add it. */
			if((work = create_workgroup_on_subnet(subrec, work_name, ttl))==NULL)
				goto done;
		}

		if((servrec = find_server_in_workgroup( work, announce_name))==NULL) {
			/* If this server is not already in the workgroup, add it. */
			create_server_on_workgroup(work, announce_name,
					servertype|SV_TYPE_LOCAL_LIST_ONLY,
					ttl, comment);
		} else {
			/* Update the record. */
			servrec->serv.type = servertype|SV_TYPE_LOCAL_LIST_ONLY;
			update_server_ttl( servrec, ttl);
			fstrcpy(servrec->serv.comment,comment);
		}
	} else {
		/*
		 * This server is announcing it is going down. Remove it from the
		 * workgroup.
		 */
		if(!is_myname(announce_name) && (work != NULL) &&
				((servrec = find_server_in_workgroup( work, announce_name))!=NULL)) {
			remove_server_from_workgroup( work, servrec);
		}
	}

	subrec->work_changed = True;
	found_lm_clients = True;

done:

	END_PROFILE(lm_host_announce);
}

/****************************************************************************
  Send a backup list response.
*****************************************************************************/

static void send_backup_list_response(struct subnet_record *subrec, 
				      struct work_record *work,
				      struct nmb_name *send_to_name,
				      unsigned char max_number_requested,
				      uint32 token, struct in_addr sendto_ip,
				      int port)
{                     
	char outbuf[1024];
	char *p, *countptr;
	unsigned int count = 0;
	unstring send_to_namestr;
#if 0
  struct server_record *servrec;
#endif
	unstring myname;

	memset(outbuf,'\0',sizeof(outbuf));

	DEBUG(3,("send_backup_list_response: sending backup list for workgroup %s to %s IP %s\n",
		work->work_group, nmb_namestr(send_to_name), inet_ntoa(sendto_ip)));
  
	p = outbuf;
  
	SCVAL(p,0,ANN_GetBackupListResp); /* Backup list response opcode. */
	p++;

	countptr = p;
	p++;

	SIVAL(p,0,token); /* The sender's unique info. */
	p += 4;
  
	/* We always return at least one name - our own. */
	count = 1;
	unstrcpy(myname, global_myname());
	strupper_m(myname);
	myname[15]='\0';
	push_pstring_base(p, myname, outbuf);

	p = skip_string(p,1);

	/* Look for backup browsers in this workgroup. */

#if 0
  /* we don't currently send become_backup requests so we should never
     send any other servers names out as backups for our
     workgroup. That's why this is commented out (tridge) */

  /*
   * NB. Note that the struct work_record here is not neccessarily
   * attached to the subnet *subrec.
   */

  for (servrec = work->serverlist; servrec; servrec = servrec->next)
  { 
    int len = PTR_DIFF(p, outbuf);
    if((sizeof(outbuf) - len) < 16)
      break;

    if(count >= (unsigned int)max_number_requested)
      break;

    if(strnequal(servrec->serv.name, global_myname(),15))
      continue;

    if(!(servrec->serv.type & SV_TYPE_BACKUP_BROWSER))
      continue;

    StrnCpy(p, servrec->serv.name, 15);
    strupper_m(p);
    count++;

    DEBUG(5,("send_backup_list_response: Adding server %s number %d\n",
              p, count));

    p = skip_string(p,1);
  }
#endif

	SCVAL(countptr, 0, count);

	pull_ascii_nstring(send_to_namestr, sizeof(send_to_namestr), send_to_name->name);

	DEBUG(4,("send_backup_list_response: sending response to %s<00> IP %s with %d servers.\n",
		send_to_namestr, inet_ntoa(sendto_ip), count));

	send_mailslot(True, BROWSE_MAILSLOT,
		outbuf,PTR_DIFF(p,outbuf),
		global_myname(), 0, 
		send_to_namestr,0,
		sendto_ip, subrec->myip, port);
}

/*******************************************************************
  Process a send backup list request packet.

  A client sends a backup list request to ask for a list of servers on
  the net that maintain server lists for a domain. A server is then
  chosen from this list to send NetServerEnum commands to to list
  available servers.

********************************************************************/

void process_get_backup_list_request(struct subnet_record *subrec,
                                     struct packet_struct *p,char *buf)
{
	struct dgram_packet *dgram = &p->packet.dgram;
	struct work_record *work;
	unsigned char max_number_requested = CVAL(buf,0);
	uint32 token = IVAL(buf,1); /* Sender's key index for the workgroup. */
	int name_type = dgram->dest_name.name_type;
	unstring workgroup_name;
	struct subnet_record *search_subrec = subrec;

	START_PROFILE(get_backup_list);
	pull_ascii_nstring(workgroup_name, sizeof(workgroup_name), dgram->dest_name.name);

	DEBUG(3,("process_get_backup_list_request: request from %s IP %s to %s.\n",
		nmb_namestr(&dgram->source_name), inet_ntoa(p->ip),
		nmb_namestr(&dgram->dest_name)));
  
	/* We have to be a master browser, or a domain master browser
		for the requested workgroup. That means it must be our
		workgroup. */

	if(strequal(workgroup_name, lp_workgroup()) == False) {
		DEBUG(7,("process_get_backup_list_request: Ignoring announce request for workgroup %s.\n",
			workgroup_name));
		goto done;
	}

	if((work = find_workgroup_on_subnet(search_subrec, workgroup_name)) == NULL) {
		DEBUG(0,("process_get_backup_list_request: Cannot find workgroup %s on \
subnet %s.\n", workgroup_name, search_subrec->subnet_name));
		goto done;
	}

	/* 
	 * If the packet was sent to WORKGROUP<1b> instead
	 * of WORKGROUP<1d> then it was unicast to us a domain master
	 * browser. Change search subrec to unicast.
	 */

	if(name_type == 0x1b) {
		/* We must be a domain master browser in order to
			process this packet. */

		if(!AM_DOMAIN_MASTER_BROWSER(work)) {
			DEBUG(0,("process_get_backup_list_request: domain list requested for workgroup %s \
and I am not a domain master browser.\n", workgroup_name));
			goto done;
		}

		search_subrec = unicast_subnet;
	} else if (name_type == 0x1d) {
		/* We must be a local master browser in order to process this packet. */

		if(!AM_LOCAL_MASTER_BROWSER(work)) {
			DEBUG(0,("process_get_backup_list_request: domain list requested for workgroup %s \
and I am not a local master browser.\n", workgroup_name));
			goto done;
		}
	} else {
		DEBUG(0,("process_get_backup_list_request: Invalid name type %x - should be 0x1b or 0x1d.\n",
			name_type));
		goto done;
	}

	send_backup_list_response(subrec, work, &dgram->source_name,
			max_number_requested, token, p->ip, p->port);

done:

	END_PROFILE(get_backup_list);
}

/*******************************************************************
  Process a reset browser state packet.

  Diagnostic packet:
  0x1 - Stop being a master browser and become a backup browser.
  0x2 - Discard browse lists, stop being a master browser, try again.
  0x4 - Stop being a master browser forever.
         
******************************************************************/

void process_reset_browser(struct subnet_record *subrec,
                                  struct packet_struct *p,char *buf)
{
	struct dgram_packet *dgram = &p->packet.dgram;
	int state = CVAL(buf,0);
	struct subnet_record *sr;

	START_PROFILE(reset_browser);

	DEBUG(1,("process_reset_browser: received diagnostic browser reset \
request from %s IP %s state=0x%X\n",
		nmb_namestr(&dgram->source_name), inet_ntoa(p->ip), state));

	/* Stop being a local master browser on all our broadcast subnets. */
	if (state & 0x1) {
		for (sr = FIRST_SUBNET; sr; sr = NEXT_SUBNET_EXCLUDING_UNICAST(sr)) {
			struct work_record *work;
			for (work = sr->workgrouplist; work; work = work->next) {
				if (AM_LOCAL_MASTER_BROWSER(work))
					unbecome_local_master_browser(sr, work, True);
			}
		}
	}
  
	/* Discard our browse lists. */
	if (state & 0x2) {
		/*
		 * Calling expire_workgroups_and_servers with a -1
		 * time causes all servers not marked with a PERMANENT_TTL
		 * on the workgroup lists to be discarded, and all 
		 * workgroups with empty server lists to be discarded.
		 * This means we keep our own server names and workgroup
		 * as these have a PERMANENT_TTL.
		 */

		expire_workgroups_and_servers(-1);
	}
  
	/* Request to stop browsing altogether. */
	if (state & 0x4)
		DEBUG(1,("process_reset_browser: ignoring request to stop being a browser.\n"));

	END_PROFILE(reset_browser);
}

/*******************************************************************
  Process an announcement request packet.
  We don't respond immediately, we just check it's a request for
  our workgroup and then set the flag telling the announce code
  in nmbd_sendannounce.c:announce_my_server_names that an 
  announcement is needed soon.
******************************************************************/

void process_announce_request(struct subnet_record *subrec, struct packet_struct *p, char *buf)
{
	struct dgram_packet *dgram = &p->packet.dgram;
	struct work_record *work;
	unstring workgroup_name;
 
	START_PROFILE(announce_request);

	pull_ascii_nstring(workgroup_name, sizeof(workgroup_name), dgram->dest_name.name);
	DEBUG(3,("process_announce_request: Announce request from %s IP %s to %s.\n",
		nmb_namestr(&dgram->source_name), inet_ntoa(p->ip),
		nmb_namestr(&dgram->dest_name)));
  
	/* We only send announcement requests on our workgroup. */
	if(strequal(workgroup_name, lp_workgroup()) == False) {
		DEBUG(7,("process_announce_request: Ignoring announce request for workgroup %s.\n",
			workgroup_name));
		goto done;
	}

	if((work = find_workgroup_on_subnet(subrec, workgroup_name)) == NULL) {
		DEBUG(0,("process_announce_request: Unable to find workgroup %s on subnet !\n",
			workgroup_name));
		goto done;
	}

	work->needannounce = True;
done:

	END_PROFILE(announce_request);
}

/*******************************************************************
  Process a LanMan announcement request packet.
  We don't respond immediately, we just check it's a request for
  our workgroup and then set the flag telling that we have found
  a LanMan client (DOS or OS/2) and that we will have to start
  sending LanMan announcements (unless specifically disabled
  through the "lm announce" parameter in smb.conf)
******************************************************************/

void process_lm_announce_request(struct subnet_record *subrec, struct packet_struct *p, char *buf)
{
	struct dgram_packet *dgram = &p->packet.dgram;
	unstring workgroup_name;

	START_PROFILE(lm_announce_request);

	pull_ascii_nstring(workgroup_name, sizeof(workgroup_name), dgram->dest_name.name);
	DEBUG(3,("process_lm_announce_request: Announce request from %s IP %s to %s.\n",
		nmb_namestr(&dgram->source_name), inet_ntoa(p->ip),
		nmb_namestr(&dgram->dest_name)));

	/* We only send announcement requests on our workgroup. */
	if(strequal(workgroup_name, lp_workgroup()) == False) {
		DEBUG(7,("process_lm_announce_request: Ignoring announce request for workgroup %s.\n",
			workgroup_name));
		goto done;
	}

	if(find_workgroup_on_subnet(subrec, workgroup_name) == NULL) {
		DEBUG(0,("process_announce_request: Unable to find workgroup %s on subnet !\n",
			workgroup_name));
		goto done;
	}

	found_lm_clients = True;

done:

	END_PROFILE(lm_announce_request);
}
