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

/* forward declarations */
static void wins_next_registration(struct response_record *rrec);


/****************************************************************************
 Deal with a response packet when registering one of our names.
****************************************************************************/

static void register_name_response(struct subnet_record *subrec,
                       struct response_record *rrec, struct packet_struct *p)
{
	/* 
	 * If we are registering broadcast, then getting a response is an
	 * error - we do not have the name. If we are registering unicast,
	 * then we expect to get a response.
	 */

	struct nmb_packet *nmb = &p->packet.nmb;
	BOOL bcast = nmb->header.nm_flags.bcast;
	BOOL success = True;
	struct nmb_name *question_name = &rrec->packet->packet.nmb.question.question_name;
	struct nmb_name *answer_name = &nmb->answers->rr_name;
	struct nmb_packet *sent_nmb = &rrec->packet->packet.nmb;
	int ttl = 0;
	uint16 nb_flags = 0;
	struct in_addr register_ip;
	fstring reg_name;
	
	putip(&register_ip,&sent_nmb->additional->rdata[2]);
	fstrcpy(reg_name, inet_ntoa(register_ip));
	
	if (subrec == unicast_subnet) {
		/* we know that this wins server is definately alive - for the moment! */
		wins_srv_alive(rrec->packet->ip, register_ip);
	}

	/* Sanity check. Ensure that the answer name in the incoming packet is the
	   same as the requested name in the outgoing packet. */

	if(!question_name || !answer_name) {
		DEBUG(0,("register_name_response: malformed response (%s is NULL).\n",
			 question_name ? "question_name" : "answer_name" ));
		return;
	}

	if(!nmb_name_equal(question_name, answer_name)) {
		DEBUG(0,("register_name_response: Answer name %s differs from question name %s.\n", 
			 nmb_namestr(answer_name), nmb_namestr(question_name)));
		return;
	}

	if(bcast) {
		/*
		 * Special hack to cope with old Samba nmbd's.
		 * Earlier versions of Samba (up to 1.9.16p11) respond 
		 * to a broadcast name registration of WORKGROUP<1b> when 
		 * they should not. Hence, until these versions are gone, 
		 * we should treat such errors as success for this particular
		 * case only. jallison@whistle.com.
		 */
		
#if 1 /* OLD_SAMBA_SERVER_HACK */
		unstring ans_name;
		pull_ascii_nstring(ans_name, sizeof(ans_name), answer_name->name);
		if((nmb->header.rcode == ACT_ERR) && strequal(lp_workgroup(), ans_name) &&
		   (answer_name->name_type == 0x1b)) {
			/* Pretend we did not get this. */
			rrec->num_msgs--;

			DEBUG(5,("register_name_response: Ignoring broadcast response to registration of name %s due to old Samba server bug.\n", 
				 nmb_namestr(answer_name)));
			return;
		}
#endif /* OLD_SAMBA_SERVER_HACK */

		/* Someone else has the name. Log the problem. */
		DEBUG(1,("register_name_response: Failed to register name %s IP %s on subnet %s via broadcast. Error code was %d. Reject came from IP %s\n", 
			 nmb_namestr(answer_name), 
			 reg_name,
			 subrec->subnet_name, nmb->header.rcode, inet_ntoa(p->ip)));
		success = False;
	} else {
		/* Unicast - check to see if the response allows us to have the name. */
		if (nmb->header.opcode == NMB_WACK_OPCODE) {
			/* WINS server is telling us to wait. Pretend we didn't get
			   the response but don't send out any more register requests. */

			DEBUG(5,("register_name_response: WACK from WINS server %s in registering name %s IP %s\n", 
				 inet_ntoa(p->ip), nmb_namestr(answer_name), reg_name));

			rrec->repeat_count = 0;
			/* How long we should wait for. */
			rrec->repeat_time = p->timestamp + nmb->answers->ttl;
			rrec->num_msgs--;
			return;
		} else if (nmb->header.rcode != 0) {
			/* Error code - we didn't get the name. */
			success = False;

			DEBUG(0,("register_name_response: %sserver at IP %s rejected our name registration of %s IP %s with error code %d.\n", 
				 subrec==unicast_subnet?"WINS ":"",
				 inet_ntoa(p->ip), 
				 nmb_namestr(answer_name), 
				 reg_name,
				 nmb->header.rcode));
		} else {
			success = True;
			/* Get the data we need to pass to the success function. */
			nb_flags = get_nb_flags(nmb->answers->rdata);
			ttl = nmb->answers->ttl;

			/* send off a registration for the next IP, if any */
			wins_next_registration(rrec);
		}
	} 

	DEBUG(5,("register_name_response: %s in registering %sname %s IP %s with %s.\n",
		 success ? "success" : "failure", 
		 subrec==unicast_subnet?"WINS ":"",
		 nmb_namestr(answer_name), 
		 reg_name,
		 inet_ntoa(rrec->packet->ip)));

	if(success) {
		/* Enter the registered name into the subnet name database before calling
		   the success function. */
		standard_success_register(subrec, rrec->userdata, answer_name, nb_flags, ttl, register_ip);
		if( rrec->success_fn)
			(*(register_name_success_function)rrec->success_fn)(subrec, rrec->userdata, answer_name, nb_flags, ttl, register_ip);
	} else {
		if( rrec->fail_fn)
			(*(register_name_fail_function)rrec->fail_fn)(subrec, rrec, question_name);
		/* Remove the name. */
		standard_fail_register( subrec, rrec, question_name);
	}

	/* Ensure we don't retry. */
	remove_response_record(subrec, rrec);
}

/****************************************************************************
 Deal with a timeout of a WINS registration request
****************************************************************************/

static void wins_registration_timeout(struct subnet_record *subrec,
				      struct response_record *rrec)
{
	struct userdata_struct *userdata = rrec->userdata;
	struct nmb_packet *sent_nmb = &rrec->packet->packet.nmb;
	struct nmb_name *nmbname = &sent_nmb->question.question_name;
	struct in_addr register_ip;
	fstring src_addr;

	putip(&register_ip,&sent_nmb->additional->rdata[2]);

	fstrcpy(src_addr, inet_ntoa(register_ip));

	DEBUG(2,("wins_registration_timeout: WINS server %s timed out registering IP %s\n", 
		 inet_ntoa(rrec->packet->ip), src_addr));

	/* mark it temporarily dead for this source address */
	wins_srv_died(rrec->packet->ip, register_ip);

	/* if we have some userdata then use that to work out what
	   wins server to try next */
	if (userdata) {
		const char *tag = (const char *)userdata->data;

		/* try the next wins server in our failover list for
		   this tag */
		rrec->packet->ip = wins_srv_ip_tag(tag, register_ip);
	}

	/* if we have run out of wins servers for this tag then they
	   must all have timed out. We treat this as *success*, not
	   failure, and go into our standard name refresh mode. This
	   copes with all the wins servers being down */
	if (wins_srv_is_dead(rrec->packet->ip, register_ip)) {
		uint16 nb_flags = get_nb_flags(sent_nmb->additional->rdata);
		int ttl = sent_nmb->additional->ttl;

		standard_success_register(subrec, userdata, nmbname, nb_flags, ttl, register_ip);
		if(rrec->success_fn) {
			(*(register_name_success_function)rrec->success_fn)(subrec, 
									    rrec->userdata, 
									    nmbname, 
									    nb_flags, 
									    ttl, 
									    register_ip);
		}

		/* send off a registration for the next IP, if any */
		wins_next_registration(rrec);

		/* don't need to send this packet any more */
		remove_response_record(subrec, rrec);
		return;
	}
	
	/* we will be moving to the next WINS server for this group,
	   send it immediately */
	rrec->repeat_count = 2;
	rrec->repeat_time = time(NULL) + 1;
	rrec->in_expiration_processing = False;

	DEBUG(6,("Retrying register of name %s IP %s with WINS server %s\n",
		 nmb_namestr(nmbname), src_addr, inet_ntoa(rrec->packet->ip)));

	/* notice that we don't remove the response record. This keeps
	   us trying to register with each of our failover wins servers */
}

/****************************************************************************
 Deal with a timeout when registering one of our names.
****************************************************************************/

static void register_name_timeout_response(struct subnet_record *subrec,
					   struct response_record *rrec)
{
	/*
	 * If we are registering unicast, then NOT getting a response is an
	 * error - we do not have the name. If we are registering broadcast,
	 * then we don't expect to get a response.
	 */

	struct nmb_packet *sent_nmb = &rrec->packet->packet.nmb;
	BOOL bcast = sent_nmb->header.nm_flags.bcast;
	BOOL success = False;
	struct nmb_name *question_name = &sent_nmb->question.question_name;
	uint16 nb_flags = 0;
	int ttl = 0;
	struct in_addr registered_ip;
	
	if (bcast) {
		if(rrec->num_msgs == 0) {
			/* Not receiving a message is success for broadcast registration. */
			success = True; 

			/* Pull the success values from the original request packet. */
			nb_flags = get_nb_flags(sent_nmb->additional->rdata);
			ttl = sent_nmb->additional->ttl;
			putip(&registered_ip,&sent_nmb->additional->rdata[2]);
		}
	} else {
		/* wins timeouts are special */
		wins_registration_timeout(subrec, rrec);
		return;
	}

	DEBUG(5,("register_name_timeout_response: %s in registering name %s on subnet %s.\n",
		 success ? "success" : "failure", nmb_namestr(question_name), subrec->subnet_name));
	if(success) {
		/* Enter the registered name into the subnet name database before calling
		   the success function. */
		standard_success_register(subrec, rrec->userdata, question_name, nb_flags, ttl, registered_ip);
		if( rrec->success_fn)
			(*(register_name_success_function)rrec->success_fn)(subrec, rrec->userdata, question_name, nb_flags, ttl, registered_ip);
	} else {
		if( rrec->fail_fn)
			(*(register_name_fail_function)rrec->fail_fn)(subrec, rrec, question_name);
		/* Remove the name. */
		standard_fail_register( subrec, rrec, question_name);
	}

	/* Ensure we don't retry. */
	remove_response_record(subrec, rrec);
}

/****************************************************************************
 Initiate one multi-homed name registration packet.
****************************************************************************/

static void multihomed_register_one(struct nmb_name *nmbname,
				    uint16 nb_flags,
				    register_name_success_function success_fn,
				    register_name_fail_function fail_fn,
				    struct in_addr ip,
				    const char *tag)
{
	struct userdata_struct *userdata;
	struct in_addr wins_ip = wins_srv_ip_tag(tag, ip);
	fstring ip_str;

	userdata = (struct userdata_struct *)SMB_MALLOC(sizeof(*userdata) + strlen(tag) + 1);
	if (!userdata) {
		DEBUG(0,("Failed to allocate userdata structure!\n"));
		return;
	}
	ZERO_STRUCTP(userdata);
	userdata->userdata_len = strlen(tag) + 1;
	strlcpy(userdata->data, tag, userdata->userdata_len);	

	fstrcpy(ip_str, inet_ntoa(ip));

	DEBUG(6,("Registering name %s IP %s with WINS server %s using tag '%s'\n",
		 nmb_namestr(nmbname), ip_str, inet_ntoa(wins_ip), tag));

	if (queue_register_multihomed_name(unicast_subnet,
					   register_name_response,
					   register_name_timeout_response,
					   success_fn,
					   fail_fn,
					   userdata,
					   nmbname,
					   nb_flags,
					   ip,
					   wins_ip) == NULL) {
		DEBUG(0,("multihomed_register_one: Failed to send packet trying to register name %s IP %s\n", 
			 nmb_namestr(nmbname), inet_ntoa(ip)));		
	}

	free(userdata);
}

/****************************************************************************
 We have finished the registration of one IP and need to see if we have
 any more IPs left to register with this group of wins server for this name.
****************************************************************************/

static void wins_next_registration(struct response_record *rrec)
{
	struct nmb_packet *sent_nmb = &rrec->packet->packet.nmb;
	struct nmb_name *nmbname = &sent_nmb->question.question_name;
	uint16 nb_flags = get_nb_flags(sent_nmb->additional->rdata);
	struct userdata_struct *userdata = rrec->userdata;
	const char *tag;
	struct in_addr last_ip;
	struct subnet_record *subrec;

	putip(&last_ip,&sent_nmb->additional->rdata[2]);

	if (!userdata) {
		/* it wasn't multi-homed */
		return;
	}

	tag = (const char *)userdata->data;

	for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
		if (ip_equal(last_ip, subrec->myip)) {
			subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec);
			break;
		}
	}

	if (!subrec) {
		/* no more to do! */
		return;
	}

	switch (sent_nmb->header.opcode) {
	case NMB_NAME_MULTIHOMED_REG_OPCODE:
		multihomed_register_one(nmbname, nb_flags, NULL, NULL, subrec->myip, tag);
		break;
	case NMB_NAME_REFRESH_OPCODE_8:
		queue_wins_refresh(nmbname, 
				   register_name_response,
				   register_name_timeout_response,
				   nb_flags, subrec->myip, tag);
		break;
	}
}

/****************************************************************************
 Try and register one of our names on the unicast subnet - multihomed.
****************************************************************************/

static void multihomed_register_name(struct nmb_name *nmbname, uint16 nb_flags,
				     register_name_success_function success_fn,
				     register_name_fail_function fail_fn)
{
	/*
	  If we are adding a group name, we just send multiple
	  register name packets to the WINS server (this is an
	  internet group name.

	  If we are adding a unique name, We need first to add 
	  our names to the unicast subnet namelist. This is 
	  because when a WINS server receives a multihomed 
	  registration request, the first thing it does is to 
	  send a name query to the registering machine, to see 
	  if it has put the name in it's local namelist.
	  We need the name there so the query response code in
	  nmbd_incomingrequests.c will find it.

	  We are adding this name prematurely (we don't really
	  have it yet), but as this is on the unicast subnet
	  only we will get away with this (only the WINS server
	  will ever query names from us on this subnet).
	*/
	int num_ips=0;
	int i, t;
	struct subnet_record *subrec;
	char **wins_tags;
	struct in_addr *ip_list;
	unstring name;

	for(subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec) )
		num_ips++;
	
	if((ip_list = SMB_MALLOC_ARRAY(struct in_addr, num_ips))==NULL) {
		DEBUG(0,("multihomed_register_name: malloc fail !\n"));
		return;
	}

	for (subrec = FIRST_SUBNET, i = 0; 
	     subrec;
	     subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec), i++ ) {
		ip_list[i] = subrec->myip;
	}

	pull_ascii_nstring(name, sizeof(name), nmbname->name);
	add_name_to_subnet(unicast_subnet, name, nmbname->name_type,
			   nb_flags, lp_max_ttl(), SELF_NAME,
			   num_ips, ip_list);

	/* get the list of wins tags - we try to register for each of them */
	wins_tags = wins_srv_tags();

	/* Now try and register the name for each wins tag.  Note that
	   at this point we only register our first IP with each wins
	   group. We will register the rest from
	   wins_next_registration() when we get the reply for this
	   one. That follows the way W2K does things (tridge)
	*/
	for (t=0; wins_tags && wins_tags[t]; t++) {
		multihomed_register_one(nmbname, nb_flags,
					success_fn, fail_fn,
					ip_list[0],
					wins_tags[t]);
	}

	wins_srv_tags_free(wins_tags);
	
	SAFE_FREE(ip_list);
}

/****************************************************************************
 Try and register one of our names.
****************************************************************************/

void register_name(struct subnet_record *subrec,
                   const char *name, int type, uint16 nb_flags,
                   register_name_success_function success_fn,
                   register_name_fail_function fail_fn,
                   struct userdata_struct *userdata)
{
	struct nmb_name nmbname;
	nstring nname;

	errno = 0;
	push_ascii_nstring(nname, name);
        if (errno == E2BIG) {
		unstring tname;
		pull_ascii_nstring(tname, sizeof(tname), nname);
		DEBUG(0,("register_name: NetBIOS name %s is too long. Truncating to %s\n",
			name, tname));
		make_nmb_name(&nmbname, tname, type);
	} else {
		make_nmb_name(&nmbname, name, type);
	}

	/* Always set the NB_ACTIVE flag on the name we are
	   registering. Doesn't make sense without it.
	*/
	
	nb_flags |= NB_ACTIVE;
	
	if (subrec == unicast_subnet) {
		/* we now always do multi-homed registration if we are
		   registering to a WINS server. This copes much
		   better with complex WINS setups */
		multihomed_register_name(&nmbname, nb_flags,
					 success_fn, fail_fn);
		return;
	}
	
	if (queue_register_name(subrec,
				register_name_response,
				register_name_timeout_response,
				success_fn,
				fail_fn,
				userdata,
				&nmbname,
				nb_flags) == NULL) {
		DEBUG(0,("register_name: Failed to send packet trying to register name %s\n",
			 nmb_namestr(&nmbname)));
	}
}

/****************************************************************************
 Try and refresh one of our names. This is *only* called for WINS refresh
****************************************************************************/

void wins_refresh_name(struct name_record *namerec)
{
	int t;
	char **wins_tags;

	/* get the list of wins tags - we try to refresh for each of them */
	wins_tags = wins_srv_tags();

	for (t=0; wins_tags && wins_tags[t]; t++) {
		queue_wins_refresh(&namerec->name, 
				   register_name_response,
				   register_name_timeout_response,
				   namerec->data.nb_flags,
				   namerec->data.ip[0], wins_tags[t]);
	}

	wins_srv_tags_free(wins_tags);
}
