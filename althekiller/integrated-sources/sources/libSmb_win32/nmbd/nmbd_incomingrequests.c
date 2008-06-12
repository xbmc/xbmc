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
   
   This file contains all the code to process NetBIOS requests coming
   in on port 137. It does not deal with the code needed to service
   WINS server requests, but only broadcast and unicast requests.

*/

#include "includes.h"

/****************************************************************************
Send a name release response.
**************************************************************************/

static void send_name_release_response(int rcode, struct packet_struct *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	char rdata[6];

	memcpy(&rdata[0], &nmb->additional->rdata[0], 6);
  
	reply_netbios_packet(p,                       /* Packet to reply to. */
			rcode,                        /* Result code. */
			NMB_REL,                      /* nmbd type code. */
			NMB_NAME_RELEASE_OPCODE,      /* opcode. */
			0,                            /* ttl. */
			rdata,                        /* data to send. */
			6);                           /* data length. */
}

/****************************************************************************
Process a name release packet on a broadcast subnet.
Ignore it if it's not one of our names.
****************************************************************************/

void process_name_release_request(struct subnet_record *subrec, 
                                  struct packet_struct *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	struct in_addr owner_ip;
	struct nmb_name *question = &nmb->question.question_name;
	unstring qname;
	BOOL bcast = nmb->header.nm_flags.bcast;
	uint16 nb_flags = get_nb_flags(nmb->additional->rdata);
	BOOL group = (nb_flags & NB_GROUP) ? True : False;
	struct name_record *namerec;
	int rcode = 0;
  
	putip((char *)&owner_ip,&nmb->additional->rdata[2]);  
  
	if(!bcast) {
		/* We should only get broadcast name release packets here.
		   Anyone trying to release unicast should be going to a WINS
		   server. If the code gets here, then either we are not a wins
		   server and they sent it anyway, or we are a WINS server and
		   the request was malformed. Either way, log an error here.
		   and send an error reply back.
		*/
		DEBUG(0,("process_name_release_request: unicast name release request \
received for name %s from IP %s on subnet %s. Error - should be sent to WINS server\n",
			nmb_namestr(question), inet_ntoa(owner_ip), subrec->subnet_name));      

		send_name_release_response(FMT_ERR, p);
		return;
	}

	DEBUG(3,("process_name_release_request: Name release on name %s, \
subnet %s from owner IP %s\n",
		nmb_namestr(&nmb->question.question_name),
		subrec->subnet_name, inet_ntoa(owner_ip)));
  
	/* If someone is releasing a broadcast group name, just ignore it. */
	if( group && !ismyip(owner_ip) )
		return;

	/*
	 * Code to work around a bug in FTP OnNet software NBT implementation.
	 * They do a broadcast name release for WORKGROUP<0> and WORKGROUP<1e>
	 * names and *don't set the group bit* !!!!!
	 */

	pull_ascii_nstring(qname, sizeof(qname), question->name);
	if( !group && !ismyip(owner_ip) && strequal(qname, lp_workgroup()) && 
			((question->name_type == 0x0) || (question->name_type == 0x1e))) {
		DEBUG(6,("process_name_release_request: FTP OnNet bug workaround. Ignoring \
group release name %s from IP %s on subnet %s with no group bit set.\n",
			nmb_namestr(question), inet_ntoa(owner_ip), subrec->subnet_name ));
		return;
	}

	namerec = find_name_on_subnet(subrec, question, FIND_ANY_NAME);

	/* We only care about someone trying to release one of our names. */
	if( namerec && ( (namerec->data.source == SELF_NAME)
			|| (namerec->data.source == PERMANENT_NAME) ) ) {
		rcode = ACT_ERR;
		DEBUG(0, ("process_name_release_request: Attempt to release name %s from IP %s \
on subnet %s being rejected as it is one of our names.\n", 
		nmb_namestr(&nmb->question.question_name), inet_ntoa(owner_ip), subrec->subnet_name));
	}

	if(rcode == 0)
		return;

	/* Send a NAME RELEASE RESPONSE (pos/neg) see rfc1002.txt 4.2.10-11 */
	send_name_release_response(rcode, p);
}

/****************************************************************************
Send a name registration response.
**************************************************************************/

static void send_name_registration_response(int rcode, int ttl, struct packet_struct *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	char rdata[6];

	memcpy(&rdata[0], &nmb->additional->rdata[0], 6);
  
	reply_netbios_packet(p,                                /* Packet to reply to. */
				rcode,                         /* Result code. */
				NMB_REG,                       /* nmbd type code. */
				NMB_NAME_REG_OPCODE,           /* opcode. */
				ttl,                           /* ttl. */
				rdata,                         /* data to send. */
				6);                            /* data length. */
}

/****************************************************************************
Process a name refresh request on a broadcast subnet.
**************************************************************************/
     
void process_name_refresh_request(struct subnet_record *subrec,
                                  struct packet_struct *p)
{    
	struct nmb_packet *nmb = &p->packet.nmb;
	struct nmb_name *question = &nmb->question.question_name;
	BOOL bcast = nmb->header.nm_flags.bcast;
	struct in_addr from_ip;
  
	putip((char *)&from_ip,&nmb->additional->rdata[2]);

	if(!bcast) { 
		/* We should only get broadcast name refresh packets here.
		   Anyone trying to refresh unicast should be going to a WINS
		   server. If the code gets here, then either we are not a wins
		   server and they sent it anyway, or we are a WINS server and
		   the request was malformed. Either way, log an error here.
		   and send an error reply back.
		*/
		DEBUG(0,("process_name_refresh_request: unicast name registration request \
received for name %s from IP %s on subnet %s.\n",
			nmb_namestr(question), inet_ntoa(from_ip), subrec->subnet_name));
		DEBUG(0,("Error - should be sent to WINS server\n"));
    
		send_name_registration_response(FMT_ERR, 0, p);
		return;
	} 

	/* Just log a message. We really don't care about broadcast name refreshes. */
     
	DEBUG(3,("process_name_refresh_request: Name refresh for name %s \
IP %s on subnet %s\n", nmb_namestr(question), inet_ntoa(from_ip), subrec->subnet_name));
}
    
/****************************************************************************
Process a name registration request on a broadcast subnet.
**************************************************************************/

void process_name_registration_request(struct subnet_record *subrec, 
                                       struct packet_struct *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	struct nmb_name *question = &nmb->question.question_name;
	BOOL bcast = nmb->header.nm_flags.bcast;
	uint16 nb_flags = get_nb_flags(nmb->additional->rdata);
	BOOL group = (nb_flags & NB_GROUP) ? True : False;
	struct name_record *namerec = NULL;
	int ttl = nmb->additional->ttl;
	struct in_addr from_ip;
  
	putip((char *)&from_ip,&nmb->additional->rdata[2]);
  
	if(!bcast) {
		/* We should only get broadcast name registration packets here.
		   Anyone trying to register unicast should be going to a WINS
		   server. If the code gets here, then either we are not a wins
		   server and they sent it anyway, or we are a WINS server and
		   the request was malformed. Either way, log an error here.
		   and send an error reply back.
		*/
		DEBUG(0,("process_name_registration_request: unicast name registration request \
received for name %s from IP %s on subnet %s. Error - should be sent to WINS server\n",
			nmb_namestr(question), inet_ntoa(from_ip), subrec->subnet_name));      

		send_name_registration_response(FMT_ERR, 0, p);
		return;
	}

	DEBUG(3,("process_name_registration_request: Name registration for name %s \
IP %s on subnet %s\n", nmb_namestr(question), inet_ntoa(from_ip), subrec->subnet_name));
  
	/* See if the name already exists. */
	namerec = find_name_on_subnet(subrec, question, FIND_ANY_NAME);
 
	/* 
	 * If the name being registered exists and is a WINS_PROXY_NAME 
	 * then delete the WINS proxy name entry so we don't reply erroneously
	 * later to queries.
	 */

	if((namerec != NULL) && (namerec->data.source == WINS_PROXY_NAME)) {
		remove_name_from_namelist( subrec, namerec );
		namerec = NULL;
	}

	if (!group) {
		/* Unique name. */

		if( (namerec != NULL)
				&& ( (namerec->data.source == SELF_NAME)
				|| (namerec->data.source == PERMANENT_NAME)
				|| NAME_GROUP(namerec) ) ) {
			/* No-one can register one of Samba's names, nor can they
				register a name that's a group name as a unique name */

			send_name_registration_response(ACT_ERR, 0, p);
			return;
		} else if(namerec != NULL) {
			/* Update the namelist record with the new information. */
			namerec->data.ip[0] = from_ip;
			update_name_ttl(namerec, ttl);

			DEBUG(3,("process_name_registration_request: Updated name record %s \
with IP %s on subnet %s\n",nmb_namestr(&namerec->name),inet_ntoa(from_ip), subrec->subnet_name));
			return;
		}
	} else {
		/* Group name. */

		if( (namerec != NULL)
				&& !NAME_GROUP(namerec)
				&& ( (namerec->data.source == SELF_NAME)
				|| (namerec->data.source == PERMANENT_NAME) ) ) {
			/* Disallow group names when we have a unique name. */
			send_name_registration_response(ACT_ERR, 0, p);  
			return;  
		}  
	}
}

/****************************************************************************
This is used to sort names for a name status into a sensible order.
We put our own names first, then in alphabetical order.
**************************************************************************/

static int status_compare(char *n1,char *n2)
{
	unstring name1, name2;
	int l1,l2,l3;

	memset(name1, '\0', sizeof(name1));
	memset(name2, '\0', sizeof(name2));
	pull_ascii_nstring(name1, sizeof(name1), n1);
	pull_ascii_nstring(name2, sizeof(name2), n2);
	n1 = name1;
	n2 = name2;

	/* It's a bit tricky because the names are space padded */
	for (l1=0;l1<15 && n1[l1] && n1[l1] != ' ';l1++)
		;
	for (l2=0;l2<15 && n2[l2] && n2[l2] != ' ';l2++)
		;
	l3 = strlen(global_myname());

	if ((l1==l3) && strncmp(n1,global_myname(),l3) == 0 && 
			(l2!=l3 || strncmp(n2,global_myname(),l3) != 0))
		return -1;

	if ((l2==l3) && strncmp(n2,global_myname(),l3) == 0 && 
			(l1!=l3 || strncmp(n1,global_myname(),l3) != 0))
		return 1;

	return memcmp(n1,n2,sizeof(name1));
}

/****************************************************************************
  Process a node status query
  ****************************************************************************/

void process_node_status_request(struct subnet_record *subrec, struct packet_struct *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	unstring qname;
	int ques_type = nmb->question.question_name.name_type;
	char rdata[MAX_DGRAM_SIZE];
	char *countptr, *buf, *bufend, *buf0;
	int names_added,i;
	struct name_record *namerec;

	pull_ascii_nstring(qname, sizeof(qname), nmb->question.question_name.name);

	DEBUG(3,("process_node_status_request: status request for name %s from IP %s on \
subnet %s.\n", nmb_namestr(&nmb->question.question_name), inet_ntoa(p->ip), subrec->subnet_name));

	if((namerec = find_name_on_subnet(subrec, &nmb->question.question_name, FIND_SELF_NAME)) == 0) {
		DEBUG(1,("process_node_status_request: status request for name %s from IP %s on \
subnet %s - name not found.\n", nmb_namestr(&nmb->question.question_name),
			inet_ntoa(p->ip), subrec->subnet_name));

		return;
	}
 
	/* this is not an exact calculation. the 46 is for the stats buffer
		and the 60 is to leave room for the header etc */
	bufend = &rdata[MAX_DGRAM_SIZE] - (18 + 46 + 60);
	countptr = buf = rdata;
	buf += 1;
	buf0 = buf;

	names_added = 0;

	namerec = subrec->namelist;

	while (buf < bufend) {
		if( (namerec->data.source == SELF_NAME) || (namerec->data.source == PERMANENT_NAME) ) {
			int name_type = namerec->name.name_type;
			unstring name;

			pull_ascii_nstring(name, sizeof(name), namerec->name.name);
			strupper_m(name);
			if (!strequal(name,"*") &&
					!strequal(name,"__SAMBA__") &&
					(name_type < 0x1b || name_type >= 0x20 || 
					ques_type < 0x1b || ques_type >= 0x20 ||
					strequal(qname, name))) {
				/* Start with the name. */
				size_t len;
				push_ascii_nstring(buf, name);
				len = strlen(buf);
				memset(buf + len, ' ', MAX_NETBIOSNAME_LEN - len - 1);
				buf[MAX_NETBIOSNAME_LEN - 1] = '\0';

				/* Put the name type and netbios flags in the buffer. */

				buf[15] = name_type;
				set_nb_flags( &buf[16],namerec->data.nb_flags );
				buf[16] |= NB_ACTIVE; /* all our names are active */

				buf += 18;

				names_added++;
			}
		}

		/* Remove duplicate names. */
		if (names_added > 1) {
			qsort( buf0, names_added, 18, QSORT_CAST status_compare );
		}

		for( i=1; i < names_added ; i++ ) {
			if (memcmp(buf0 + 18*i,buf0 + 18*(i-1),16) == 0) {
				names_added--;
				if (names_added == i)
					break;
				memmove(buf0 + 18*i,buf0 + 18*(i+1),18*(names_added-i));
				i--;
			}
		}

		buf = buf0 + 18*names_added;

		namerec = namerec->next;

		if (!namerec) {
			/* End of the subnet specific name list. Now 
				add the names on the unicast subnet . */
			struct subnet_record *uni_subrec = unicast_subnet;

			if (uni_subrec != subrec) {
				subrec = uni_subrec;
				namerec = subrec->namelist;
			}
		}
		if (!namerec)
			break;

	}
  
	SCVAL(countptr,0,names_added);
  
	/* We don't send any stats as they could be used to attack
		the protocol. */
	memset(buf,'\0',46);
  
	buf += 46;
  
	/* Send a NODE STATUS RESPONSE */
	reply_netbios_packet(p,                               /* Packet to reply to. */
				0,                            /* Result code. */
				NMB_STATUS,                   /* nmbd type code. */
				NMB_NAME_QUERY_OPCODE,        /* opcode. */
				0,                            /* ttl. */
				rdata,                        /* data to send. */
				PTR_DIFF(buf,rdata));         /* data length. */
}


/***************************************************************************
Process a name query.

For broadcast name queries:

  - Only reply if the query is for one of YOUR names.
  - NEVER send a negative response to a broadcast query.

****************************************************************************/

void process_name_query_request(struct subnet_record *subrec, struct packet_struct *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	struct nmb_name *question = &nmb->question.question_name;
	int name_type = question->name_type;
	BOOL bcast = nmb->header.nm_flags.bcast;
	int ttl=0;
	int rcode = 0;
	char *prdata = NULL;
	char rdata[6];
	BOOL success = False;
	struct name_record *namerec = NULL;
	int reply_data_len = 0;
	int i;
	
	DEBUG(3,("process_name_query_request: Name query from %s on subnet %s for name %s\n", 
		 inet_ntoa(p->ip), subrec->subnet_name, nmb_namestr(question)));
  
	/* Look up the name in the cache - if the request is a broadcast request that
	   came from a subnet we don't know about then search all the broadcast subnets
	   for a match (as we don't know what interface the request came in on). */

	if(subrec == remote_broadcast_subnet)
		namerec = find_name_for_remote_broadcast_subnet( question, FIND_ANY_NAME);
	else
		namerec = find_name_on_subnet(subrec, question, FIND_ANY_NAME);

	/* Check if it is a name that expired */
	if (namerec && 
	    ((namerec->data.death_time != PERMANENT_TTL) && 
	     (namerec->data.death_time < p->timestamp))) {
		DEBUG(5,("process_name_query_request: expired name %s\n", nmb_namestr(&namerec->name)));
		namerec = NULL;
	}

	if (namerec) {
		/* 
		 * Always respond to unicast queries.
		 * Don't respond to broadcast queries unless the query is for
		 * a name we own, a Primary Domain Controller name, or a WINS_PROXY 
		 * name with type 0 or 0x20. WINS_PROXY names are only ever added
		 * into the namelist if we were configured as a WINS proxy.
		 */
		
		if (!bcast || 
		    (bcast && ((name_type == 0x1b) ||
			       (namerec->data.source == SELF_NAME) ||
			       (namerec->data.source == PERMANENT_NAME) ||
			       ((namerec->data.source == WINS_PROXY_NAME) &&
				((name_type == 0) || (name_type == 0x20)))))) {
			/* The requested name is a directed query, or it's SELF or PERMANENT or WINS_PROXY, 
			   or it's a Domain Master type. */

			/*
			 * If this is a WINS_PROXY_NAME, then ceck that none of the IP 
			 * addresses we are returning is on the same broadcast subnet 
			 * as the requesting packet. If it is then don't reply as the 
			 * actual machine will be replying also and we don't want two 
			 * replies to a broadcast query.
			 */
			
			if (namerec->data.source == WINS_PROXY_NAME) {
				for( i = 0; i < namerec->data.num_ips; i++) {
					if (same_net(namerec->data.ip[i], subrec->myip, subrec->mask_ip)) {
						DEBUG(5,("process_name_query_request: name %s is a WINS proxy name and is also on the same subnet (%s) as the requestor. Not replying.\n", 
							 nmb_namestr(&namerec->name), subrec->subnet_name ));
						return;
					}
				}
			}

			ttl = (namerec->data.death_time != PERMANENT_TTL) ?
				namerec->data.death_time - p->timestamp : lp_max_ttl();

			/* Copy all known ip addresses into the return data. */
			/* Optimise for the common case of one IP address so 
			   we don't need a malloc. */

			if (namerec->data.num_ips == 1) {
				prdata = rdata;
			} else {
				if ((prdata = (char *)SMB_MALLOC( namerec->data.num_ips * 6 )) == NULL) {
					DEBUG(0,("process_name_query_request: malloc fail !\n"));
					return;
				}
			}

			for (i = 0; i < namerec->data.num_ips; i++) {
				set_nb_flags(&prdata[i*6],namerec->data.nb_flags);
				putip((char *)&prdata[2+(i*6)], &namerec->data.ip[i]);
			}

			sort_query_replies(prdata, i, p->ip);
			
			reply_data_len = namerec->data.num_ips * 6;
			success = True;
		}
	}

	/*
	 * If a machine is broadcasting a name lookup request and we have lp_wins_proxy()
	 * set we should initiate a WINS query here. On success we add the resolved name 
	 * into our namelist with a type of WINS_PROXY_NAME and then reply to the query.
	 */
	
	if(!success && (namerec == NULL) && we_are_a_wins_client() && lp_wins_proxy() && 
	   bcast && (subrec != remote_broadcast_subnet)) {
		make_wins_proxy_name_query_request( subrec, p, question );
		return;
	}

	if (!success && bcast) {
		if(prdata != rdata)
			SAFE_FREE(prdata);
		return; /* Never reply with a negative response to broadcasts. */
	}

	/* 
	 * Final check. From observation, if a unicast packet is sent
	 * to a non-WINS server with the recursion desired bit set
	 * then never send a negative response.
	 */
	
	if(!success && !bcast && nmb->header.nm_flags.recursion_desired) {
		if(prdata != rdata)
			SAFE_FREE(prdata);
		return;
	}

	if (success) {
		rcode = 0;
		DEBUG(3,("OK\n"));
	} else {
		rcode = NAM_ERR;
		DEBUG(3,("UNKNOWN\n"));      
	}

	/* See rfc1002.txt 4.2.13. */

	reply_netbios_packet(p,                              /* Packet to reply to. */
			     rcode,                          /* Result code. */
			     NMB_QUERY,                      /* nmbd type code. */
			     NMB_NAME_QUERY_OPCODE,          /* opcode. */
			     ttl,                            /* ttl. */
			     prdata,                         /* data to send. */
			     reply_data_len);                /* data length. */
	
	if(prdata != rdata)
		SAFE_FREE(prdata);
}
