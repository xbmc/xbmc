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
 Deal with a response packet when querying a name.
****************************************************************************/

static void query_name_response( struct subnet_record   *subrec,
                                 struct response_record *rrec,
                                 struct packet_struct   *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	BOOL success = False;
	struct nmb_name *question_name = &rrec->packet->packet.nmb.question.question_name;
	struct in_addr answer_ip;

	zero_ip(&answer_ip);

	/* Ensure we don't retry the query but leave the response record cleanup
		to the timeout code. We may get more answer responses in which case
		we should mark the name in conflict.. */
	rrec->repeat_count = 0;

	if(rrec->num_msgs == 1) {
		/* This is the first response. */

		if(nmb->header.opcode == NMB_WACK_OPCODE) {
			/* WINS server is telling us to wait. Pretend we didn't get
				the response but don't send out any more query requests. */

			if( DEBUGLVL( 5 ) ) {
				dbgtext( "query_name_response: " );
				dbgtext( "WACK from WINS server %s ", inet_ntoa(p->ip) );
				dbgtext( "in querying name %s ", nmb_namestr(question_name) );
				dbgtext( "on subnet %s.\n", subrec->subnet_name );
			}
  
			rrec->repeat_count = 0;
			/* How long we should wait for. */
			if (nmb->answers) {
				rrec->repeat_time = p->timestamp + nmb->answers->ttl;
			} else {
				/* No answer - this is probably a corrupt
				   packet.... */
				DEBUG(0,("query_name_response: missing answer record in "
					"NMB_WACK_OPCODE response.\n"));
				rrec->repeat_time = p->timestamp + 10;
			}
			rrec->num_msgs--;
			return;
		} else if(nmb->header.rcode != 0) {

			success = False;

			if( DEBUGLVL( 5 ) ) {
				dbgtext( "query_name_response: On subnet %s ", subrec->subnet_name );
				dbgtext( "- negative response from IP %s ", inet_ntoa(p->ip) );
				dbgtext( "for name %s. ", nmb_namestr(question_name) );
				dbgtext( "Error code was %d.\n", nmb->header.rcode );
			}
		} else {
			if (!nmb->answers) {
				dbgtext( "query_name_response: On subnet %s ", subrec->subnet_name );
				dbgtext( "IP %s ", inet_ntoa(p->ip) );
				dbgtext( "returned a success response with no answer\n" );
				return;
			}

			success = True;

			putip((char *)&answer_ip,&nmb->answers->rdata[2]);
	
			if( DEBUGLVL( 5 ) ) {
				dbgtext( "query_name_response: On subnet %s ", subrec->subnet_name );
				dbgtext( "- positive response from IP %s ", inet_ntoa(p->ip) );
				dbgtext( "for name %s.  ", nmb_namestr(question_name) );
				dbgtext( "IP of that name is %s\n", inet_ntoa(answer_ip) );
			}

			/* Interestingly, we could add these names to our namelists, and
				change nmbd to a model that checked its own name cache first,
				before sending out a query. This is a task for another day, though.
			*/
		}
	} else if( rrec->num_msgs > 1) {

		if( DEBUGLVL( 0 ) ) {
			if (nmb->answers)
				putip( (char *)&answer_ip, &nmb->answers->rdata[2] );
			dbgtext( "query_name_response: " );
			dbgtext( "Multiple (%d) responses ", rrec->num_msgs );
			dbgtext( "received for a query on subnet %s ", subrec->subnet_name );
			dbgtext( "for name %s.\nThis response ", nmb_namestr(question_name) );
			dbgtext( "was from IP %s, reporting ", inet_ntoa(p->ip) );
			dbgtext( "an IP address of %s.\n", inet_ntoa(answer_ip) );
		}

		/* We have already called the success or fail function, so we
			don't call again here. Leave the response record around in
			case we get more responses. */

		return; 
	}
  
	if(success && rrec->success_fn)
		(*(query_name_success_function)rrec->success_fn)(subrec, rrec->userdata, question_name, answer_ip, nmb->answers);
	else if( rrec->fail_fn)
		(*(query_name_fail_function)rrec->fail_fn)(subrec, rrec, question_name, nmb->header.rcode);

}

/****************************************************************************
 Deal with a timeout when querying a name.
****************************************************************************/

static void query_name_timeout_response(struct subnet_record *subrec,
                       struct response_record *rrec)
{
	struct nmb_packet *sent_nmb = &rrec->packet->packet.nmb;
	/* We can only fail here, never succeed. */
	BOOL failed = True;
	struct nmb_name *question_name = &sent_nmb->question.question_name;

	if(rrec->num_msgs != 0) {
		/* We got at least one response, and have called the success/fail
			function already. */

		failed = False; 
	}

	if(failed) {
		if( DEBUGLVL( 5 ) ) {
			dbgtext( "query_name_timeout_response: No response to " );
			dbgtext( "query for name %s ", nmb_namestr(question_name) );
			dbgtext( "on subnet %s.\n", subrec->subnet_name );
		}

		if(rrec->fail_fn)
			(*(query_name_fail_function)rrec->fail_fn)(subrec, rrec, question_name, 0);
	}

	remove_response_record(subrec, rrec);
}

/****************************************************************************
 Lookup a name on our local namelists. We check the lmhosts file first. If the
 name is not there we look for the name on the given subnet.
****************************************************************************/

static BOOL query_local_namelists(struct subnet_record *subrec, struct nmb_name *nmbname,
                                  struct name_record **namerecp) 
{
	struct name_record *namerec;

	*namerecp = NULL;

	if(find_name_in_lmhosts(nmbname, namerecp))
		return True;
  
	if((namerec = find_name_on_subnet(subrec, nmbname, FIND_ANY_NAME))==NULL)
		return False;

	if( NAME_IS_ACTIVE(namerec) && ( (namerec->data.source == SELF_NAME) || (namerec->data.source == LMHOSTS_NAME) ) ) {
		*namerecp = namerec;
		return True;
	} 
	return False;
}

/****************************************************************************
 Try and query for a name.
****************************************************************************/

BOOL query_name(struct subnet_record *subrec, const char *name, int type,
                   query_name_success_function success_fn,
                   query_name_fail_function fail_fn, 
                   struct userdata_struct *userdata)
{
	struct nmb_name nmbname;
	struct name_record *namerec;

	make_nmb_name(&nmbname, name, type);

	/*
	 * We need to check our local namelists first.
	 * It may be an magic name, lmhosts name or just
	 * a name we have registered.
	 */

	if(query_local_namelists(subrec, &nmbname, &namerec) == True) {
		struct res_rec rrec;
		int i;

		memset((char *)&rrec, '\0', sizeof(struct res_rec));

		/* Fake up the needed res_rec just in case it's used. */
		rrec.rr_name = nmbname;
		rrec.rr_type = RR_TYPE_NB;
		rrec.rr_class = RR_CLASS_IN;
		rrec.ttl = PERMANENT_TTL;
		rrec.rdlength = namerec->data.num_ips * 6;
		if(rrec.rdlength > MAX_DGRAM_SIZE) {
			if( DEBUGLVL( 0 ) ) {
				dbgtext( "query_name: nmbd internal error - " );
				dbgtext( "there are %d ip addresses ", namerec->data.num_ips );
				dbgtext( "for name %s.\n", nmb_namestr(&nmbname) );
			}
			return False;
		}

		for( i = 0; i < namerec->data.num_ips; i++) {
			set_nb_flags( &rrec.rdata[i*6], namerec->data.nb_flags );
			putip( &rrec.rdata[(i*6) + 2], (char *)&namerec->data.ip[i]);
		}

		/* Call the success function directly. */
		if(success_fn)
			(*(query_name_success_function)success_fn)(subrec, userdata, &nmbname, namerec->data.ip[0], &rrec);
		return False;
	}

	if(queue_query_name( subrec, query_name_response, query_name_timeout_response, success_fn, fail_fn, userdata, &nmbname) == NULL) {
		if( DEBUGLVL( 0 ) ) {
			dbgtext( "query_name: Failed to send packet " );
			dbgtext( "trying to query name %s\n", nmb_namestr(&nmbname) );
		}
		return True;
	}
	return False;
}

/****************************************************************************
 Try and query for a name from nmbd acting as a WINS server.
****************************************************************************/

BOOL query_name_from_wins_server(struct in_addr ip_to, 
                   const char *name, int type,
                   query_name_success_function success_fn,
                   query_name_fail_function fail_fn, 
                   struct userdata_struct *userdata)
{
	struct nmb_name nmbname;

	make_nmb_name(&nmbname, name, type);

	if(queue_query_name_from_wins_server( ip_to, query_name_response, query_name_timeout_response, success_fn, fail_fn, userdata, &nmbname) == NULL) {
		if( DEBUGLVL( 0 ) ) {
			dbgtext( "query_name_from_wins_server: Failed to send packet " );
			dbgtext( "trying to query name %s\n", nmb_namestr(&nmbname) );
		}
		return True;
	}
	return False;
}
