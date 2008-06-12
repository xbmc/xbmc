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

/****************************************************************************
 Deal with a response packet when releasing one of our names.
****************************************************************************/

static void release_name_response(struct subnet_record *subrec,
				  struct response_record *rrec, struct packet_struct *p)
{
	/* 
	 * If we are releasing broadcast, then getting a response is an
	 * error. If we are releasing unicast, then we expect to get a response.
	 */
	struct nmb_packet *nmb = &p->packet.nmb;
	BOOL bcast = nmb->header.nm_flags.bcast;
	BOOL success = True;
	struct nmb_name *question_name = &rrec->packet->packet.nmb.question.question_name;
	struct nmb_name *answer_name = &nmb->answers->rr_name;
	struct in_addr released_ip;

	/* Sanity check. Ensure that the answer name in the incoming packet is the
	   same as the requested name in the outgoing packet. */
	if (!nmb_name_equal(question_name, answer_name)) {
		DEBUG(0,("release_name_response: Answer name %s differs from question name %s.\n", 
			 nmb_namestr(answer_name), nmb_namestr(question_name)));
		return;
	}

	if (bcast) {
		/* Someone sent a response to a bcast release? ignore it. */
		return;
	}

	/* Unicast - check to see if the response allows us to release the name. */
	if (nmb->header.rcode != 0) {
		/* Error code - we were told not to release the name ! What now ! */
		success = False;

		DEBUG(0,("release_name_response: WINS server at IP %s rejected our \
name release of name %s with error code %d.\n", 
			 inet_ntoa(p->ip), 
			 nmb_namestr(answer_name), nmb->header.rcode));
	} else if (nmb->header.opcode == NMB_WACK_OPCODE) {
		/* WINS server is telling us to wait. Pretend we didn't get
		   the response but don't send out any more release requests. */

		DEBUG(5,("release_name_response: WACK from WINS server %s in releasing \
name %s on subnet %s.\n", 
			 inet_ntoa(p->ip), nmb_namestr(answer_name), subrec->subnet_name));

		rrec->repeat_count = 0;
		/* How long we should wait for. */
		rrec->repeat_time = p->timestamp + nmb->answers->ttl;
		rrec->num_msgs--;
		return;
	}

	DEBUG(5,("release_name_response: %s in releasing name %s on subnet %s.\n",
		 success ? "success" : "failure", nmb_namestr(answer_name), subrec->subnet_name));
	if (success) {
		putip((char*)&released_ip ,&nmb->answers->rdata[2]);

		if(rrec->success_fn)
			(*(release_name_success_function)rrec->success_fn)(subrec, rrec->userdata, answer_name, released_ip);
		standard_success_release( subrec, rrec->userdata, answer_name, released_ip);
	} else {
		/* We have no standard_fail_release - maybe we should add one ? */
		if (rrec->fail_fn) {
			(*(release_name_fail_function)rrec->fail_fn)(subrec, rrec, answer_name);
		}
	}

	remove_response_record(subrec, rrec);
}

/****************************************************************************
 Deal with a timeout when releasing one of our names.
****************************************************************************/

static void release_name_timeout_response(struct subnet_record *subrec,
					  struct response_record *rrec)
{
	/* a release is *always* considered to be successful when it
	   times out. This doesn't cause problems as if a WINS server
	   doesn't respond and someone else wants the name then the
	   normal WACK/name query from the WINS server will cope */
	struct nmb_packet *sent_nmb = &rrec->packet->packet.nmb;
	BOOL bcast = sent_nmb->header.nm_flags.bcast;
	struct nmb_name *question_name = &sent_nmb->question.question_name;
	struct in_addr released_ip;

	/* Get the ip address we were trying to release. */
	putip((char*)&released_ip ,&sent_nmb->additional->rdata[2]);

	if (!bcast) {
		/* mark the WINS server temporarily dead */
		wins_srv_died(rrec->packet->ip, released_ip);
	}

	DEBUG(5,("release_name_timeout_response: success in releasing name %s on subnet %s.\n",
		 nmb_namestr(question_name), subrec->subnet_name));

	if (rrec->success_fn) {
		(*(release_name_success_function)rrec->success_fn)(subrec, rrec->userdata, question_name, released_ip);
	}

	standard_success_release( subrec, rrec->userdata, question_name, released_ip);
	remove_response_record(subrec, rrec);
}


/*
  when releasing a name with WINS we need to send the release to each of
  the WINS groups
*/
static void wins_release_name(struct name_record *namerec,
			      release_name_success_function success_fn,
			      release_name_fail_function fail_fn,
			      struct userdata_struct *userdata)			      
{
	int t, i;
	char **wins_tags;

	/* get the list of wins tags - we try to release for each of them */
	wins_tags = wins_srv_tags();

	for (t=0;wins_tags && wins_tags[t]; t++) {
		for (i = 0; i < namerec->data.num_ips; i++) {
			struct in_addr wins_ip = wins_srv_ip_tag(wins_tags[t], namerec->data.ip[i]);

			BOOL last_one = ((i==namerec->data.num_ips - 1) && !wins_tags[t+1]);
			if (queue_release_name(unicast_subnet,
					       release_name_response,
					       release_name_timeout_response,
					       last_one?success_fn : NULL,
					       last_one? fail_fn : NULL,
					       last_one? userdata : NULL,
					       &namerec->name,
					       namerec->data.nb_flags,
					       namerec->data.ip[i],
					       wins_ip) == NULL) {
				DEBUG(0,("release_name: Failed to send packet trying to release name %s IP %s\n",
					 nmb_namestr(&namerec->name), inet_ntoa(namerec->data.ip[i]) ));
			}
		}
	}

	wins_srv_tags_free(wins_tags);
}


/****************************************************************************
 Try and release one of our names.
****************************************************************************/

void release_name(struct subnet_record *subrec, struct name_record *namerec,
		  release_name_success_function success_fn,
		  release_name_fail_function fail_fn,
		  struct userdata_struct *userdata)
{
	int i;

	/* Ensure it's a SELF name, and in the ACTIVE state. */
	if ((namerec->data.source != SELF_NAME) || !NAME_IS_ACTIVE(namerec)) {
		DEBUG(0,("release_name: Cannot release name %s from subnet %s. Source was %d \n",
			 nmb_namestr(&namerec->name), subrec->subnet_name, namerec->data.source)); 
		return;
	}

	/* Set the name into the deregistering state. */
	namerec->data.nb_flags |= NB_DEREG;

	/* wins releases are a bit different */
	if (subrec == unicast_subnet) {
		wins_release_name(namerec, success_fn, fail_fn, userdata);
		return;
	}

	/*  
	 * Go through and release the name for all known ip addresses.
	 * Only call the success/fail function on the last one (it should
	 * only be done once).
	 */
	for (i = 0; i < namerec->data.num_ips; i++) {
		if (queue_release_name(subrec,
				       release_name_response,
				       release_name_timeout_response,
				       (i == (namerec->data.num_ips - 1)) ? success_fn : NULL,
				       (i == (namerec->data.num_ips - 1)) ? fail_fn : NULL,
				       (i == (namerec->data.num_ips - 1)) ? userdata : NULL,
				       &namerec->name,
				       namerec->data.nb_flags,
				       namerec->data.ip[i],
				       subrec->bcast_ip) == NULL) {
			DEBUG(0,("release_name: Failed to send packet trying to release name %s IP %s\n",
				 nmb_namestr(&namerec->name), inet_ntoa(namerec->data.ip[i]) ));
		}
	}
}
