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

/****************************************************************************
 Deal with a successful node status response.
****************************************************************************/

static void node_status_response(struct subnet_record *subrec,
                       struct response_record *rrec, struct packet_struct *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	struct nmb_name *question_name = &rrec->packet->packet.nmb.question.question_name;
	struct nmb_name *answer_name = &nmb->answers->rr_name;

	/* Sanity check. Ensure that the answer name in the incoming packet is the
		same as the requested name in the outgoing packet. */

	if(!nmb_name_equal(question_name, answer_name)) {
		DEBUG(0,("node_status_response: Answer name %s differs from question \
name %s.\n", nmb_namestr(answer_name), nmb_namestr(question_name)));
		return;
	}

	DEBUG(5,("node_status_response: response from name %s on subnet %s.\n",
		nmb_namestr(answer_name), subrec->subnet_name));

	/* Just send the whole answer resource record for the success function to parse. */
	if(rrec->success_fn)
		(*(node_status_success_function)rrec->success_fn)(subrec, rrec->userdata, nmb->answers, p->ip);

	/* Ensure we don't retry. */
	remove_response_record(subrec, rrec);
}

/****************************************************************************
 Deal with a timeout when requesting a node status.
****************************************************************************/

static void node_status_timeout_response(struct subnet_record *subrec,
                       struct response_record *rrec)
{
	struct nmb_packet *sent_nmb = &rrec->packet->packet.nmb;
	struct nmb_name *question_name = &sent_nmb->question.question_name;

	DEBUG(5,("node_status_timeout_response: failed to get node status from name %s on subnet %s\n",
		nmb_namestr(question_name), subrec->subnet_name));

	if( rrec->fail_fn)
		(*rrec->fail_fn)(subrec, rrec);

	/* Ensure we don't retry. */
	remove_response_record(subrec, rrec);
}

/****************************************************************************
 Try and do a node status to a name - given the name & IP address.
****************************************************************************/
 
BOOL node_status(struct subnet_record *subrec, struct nmb_name *nmbname,
                 struct in_addr send_ip, node_status_success_function success_fn, 
                 node_status_fail_function fail_fn, struct userdata_struct *userdata)
{
	if(queue_node_status( subrec, node_status_response, node_status_timeout_response,
				success_fn, fail_fn, userdata, nmbname, send_ip)==NULL) {
		DEBUG(0,("node_status: Failed to send packet trying to get node status for \
name %s, IP address %s\n", nmb_namestr(nmbname), inet_ntoa(send_ip)));
		return True;
	} 
	return False;
}
