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

extern int ClientNMB;
extern int ClientDGRAM;
extern int global_nmb_port;

extern int num_response_packets;

extern struct in_addr loopback_ip;

static void queue_packet(struct packet_struct *packet);

BOOL rescan_listen_set = False;


/*******************************************************************
  The global packet linked-list. Incoming entries are 
  added to the end of this list. It is supposed to remain fairly 
  short so we won't bother with an end pointer.
******************************************************************/

static struct packet_struct *packet_queue = NULL;

/***************************************************************************
Utility function to find the specific fd to send a packet out on.
**************************************************************************/

static int find_subnet_fd_for_address( struct in_addr local_ip )
{
	struct subnet_record *subrec;

	for( subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec))
		if(ip_equal(local_ip, subrec->myip))
			return subrec->nmb_sock;

	return ClientNMB;
}

/***************************************************************************
Utility function to find the specific fd to send a mailslot packet out on.
**************************************************************************/

static int find_subnet_mailslot_fd_for_address( struct in_addr local_ip )
{
	struct subnet_record *subrec;

	for( subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec))
		if(ip_equal(local_ip, subrec->myip))
			return subrec->dgram_sock;

	return ClientDGRAM;
}

/***************************************************************************
Get/Set problematic nb_flags as network byte order 16 bit int.
**************************************************************************/

uint16 get_nb_flags(char *buf)
{
	return ((((uint16)*buf)&0xFFFF) & NB_FLGMSK);
}

void set_nb_flags(char *buf, uint16 nb_flags)
{
	*buf++ = ((nb_flags & NB_FLGMSK) & 0xFF);
	*buf = '\0';
}

/***************************************************************************
Dumps out the browse packet data.
**************************************************************************/

static void debug_browse_data(char *outbuf, int len)
{
	int i,j;

	DEBUG( 4, ( "debug_browse_data():\n" ) );
	for (i = 0; i < len; i+= 16) {
		DEBUGADD( 4, ( "%3x char ", i ) );

		for (j = 0; j < 16; j++) {
			unsigned char x;
			if (i+j >= len)
				break;

			x = outbuf[i+j];
			if (x < 32 || x > 127) 
				x = '.';
	    
			DEBUGADD( 4, ( "%c", x ) );
		}

		DEBUGADD( 4, ( "%*s hex", 16-j, "" ) );

		for (j = 0; j < 16; j++) {
			if (i+j >= len) 
				break;
			DEBUGADD( 4, ( " %02x", (unsigned char)outbuf[i+j] ) );
		}

		DEBUGADD( 4, ("\n") );
	}
}

/***************************************************************************
  Generates the unique transaction identifier
**************************************************************************/

static uint16 name_trn_id=0;

static uint16 generate_name_trn_id(void)
{
	if (!name_trn_id) {
		name_trn_id = ((unsigned)time(NULL)%(unsigned)0x7FFF) + ((unsigned)sys_getpid()%(unsigned)100);
	}
	name_trn_id = (name_trn_id+1) % (unsigned)0x7FFF;
	return name_trn_id;
}

/***************************************************************************
 Either loops back or sends out a completed NetBIOS packet.
**************************************************************************/

static BOOL send_netbios_packet(struct packet_struct *p)
{
	BOOL loopback_this_packet = False;

	/* Check if we are sending to or from ourselves as a WINS server. */
	if(ismyip(p->ip) && (p->port == global_nmb_port))
		loopback_this_packet = True;

	if(loopback_this_packet) {
		struct packet_struct *lo_packet = NULL;
		DEBUG(5,("send_netbios_packet: sending packet to ourselves.\n"));
		if((lo_packet = copy_packet(p)) == NULL)
			return False;
		queue_packet(lo_packet);
	} else if (!send_packet(p)) {
		DEBUG(0,("send_netbios_packet: send_packet() to IP %s port %d failed\n",
			inet_ntoa(p->ip),p->port));
		return False;
	}
  
	return True;
} 

/***************************************************************************
 Sets up the common elements of an outgoing NetBIOS packet.

 Note: do not attempt to rationalise whether rec_des should be set or not
 in a particular situation. Just follow rfc_1002 or look at examples from WinXX.
 It does NOT follow the rule that requests to the wins server always have
 rec_des true. See for example name releases and refreshes
**************************************************************************/

static struct packet_struct *create_and_init_netbios_packet(struct nmb_name *nmbname,
                                                            BOOL bcast, BOOL rec_des,
                                                            struct in_addr to_ip)
{
	struct packet_struct *packet = NULL;
	struct nmb_packet *nmb = NULL;

	/* Allocate the packet_struct we will return. */
	if((packet = SMB_MALLOC_P(struct packet_struct)) == NULL) {
		DEBUG(0,("create_and_init_netbios_packet: malloc fail (1) for packet struct.\n"));
		return NULL;
	}
    
	memset((char *)packet,'\0',sizeof(*packet));

	nmb = &packet->packet.nmb;

	nmb->header.name_trn_id = generate_name_trn_id();
	nmb->header.response = False;
	nmb->header.nm_flags.recursion_desired = rec_des;
	nmb->header.nm_flags.recursion_available = False;
	nmb->header.nm_flags.trunc = False;
	nmb->header.nm_flags.authoritative = False;
	nmb->header.nm_flags.bcast = bcast;
  
	nmb->header.rcode = 0;
	nmb->header.qdcount = 1;
	nmb->header.ancount = 0;
	nmb->header.nscount = 0;

	nmb->question.question_name = *nmbname;
	nmb->question.question_type = QUESTION_TYPE_NB_QUERY;
	nmb->question.question_class = QUESTION_CLASS_IN;

	packet->ip = to_ip;
	packet->port = NMB_PORT;
	packet->fd = ClientNMB;
	packet->timestamp = time(NULL);
	packet->packet_type = NMB_PACKET;
	packet->locked = False;
  
	return packet; /* Caller must free. */
}

/***************************************************************************
 Sets up the common elements of register, refresh or release packet.
**************************************************************************/

static BOOL create_and_init_additional_record(struct packet_struct *packet,
                                                     uint16 nb_flags,
                                                     struct in_addr *register_ip)
{
	struct nmb_packet *nmb = &packet->packet.nmb;

	if((nmb->additional = SMB_MALLOC_P(struct res_rec)) == NULL) {
		DEBUG(0,("initiate_name_register_packet: malloc fail for additional record.\n"));
		return False;
	}

	memset((char *)nmb->additional,'\0',sizeof(struct res_rec));

	nmb->additional->rr_name  = nmb->question.question_name;
	nmb->additional->rr_type  = RR_TYPE_NB;
	nmb->additional->rr_class = RR_CLASS_IN;
	
	/* See RFC 1002, sections 5.1.1.1, 5.1.1.2 and 5.1.1.3 */
	if (nmb->header.nm_flags.bcast)
		nmb->additional->ttl = PERMANENT_TTL;
	else
		nmb->additional->ttl = lp_max_ttl();
	
	nmb->additional->rdlength = 6;
	
	set_nb_flags(nmb->additional->rdata,nb_flags);
	
	/* Set the address for the name we are registering. */
	putip(&nmb->additional->rdata[2], register_ip);
	
	/* 
	   it turns out that Jeremys code was correct, we are supposed
	   to send registrations from the IP we are registering. The
	   trick is what to do on timeouts! When we send on a
	   non-routable IP then the reply will timeout, and we should
	   treat this as success, not failure. That means we go into
	   our standard refresh cycle for that name which copes nicely
	   with disconnected networks.
	*/
	packet->fd = find_subnet_fd_for_address(*register_ip);

	return True;
}

/***************************************************************************
 Sends out a name query.
**************************************************************************/

static BOOL initiate_name_query_packet( struct packet_struct *packet)
{
	struct nmb_packet *nmb = NULL;

	nmb = &packet->packet.nmb;

	nmb->header.opcode = NMB_NAME_QUERY_OPCODE;
	nmb->header.arcount = 0;

	nmb->header.nm_flags.recursion_desired = True;

	DEBUG(4,("initiate_name_query_packet: sending query for name %s (bcast=%s) to IP %s\n",
		nmb_namestr(&nmb->question.question_name), 
		BOOLSTR(nmb->header.nm_flags.bcast), inet_ntoa(packet->ip)));

	return send_netbios_packet( packet );
}

/***************************************************************************
 Sends out a name query - from a WINS server. 
**************************************************************************/

static BOOL initiate_name_query_packet_from_wins_server( struct packet_struct *packet)
{   
	struct nmb_packet *nmb = NULL;
  
	nmb = &packet->packet.nmb;

	nmb->header.opcode = NMB_NAME_QUERY_OPCODE;
	nmb->header.arcount = 0;
    
	nmb->header.nm_flags.recursion_desired = False;
  
	DEBUG(4,("initiate_name_query_packet_from_wins_server: sending query for name %s (bcast=%s) to IP %s\n",
		nmb_namestr(&nmb->question.question_name),
		BOOLSTR(nmb->header.nm_flags.bcast), inet_ntoa(packet->ip)));
    
	return send_netbios_packet( packet );
} 

/***************************************************************************
 Sends out a name register.
**************************************************************************/

static BOOL initiate_name_register_packet( struct packet_struct *packet,
                                    uint16 nb_flags, struct in_addr *register_ip)
{
	struct nmb_packet *nmb = &packet->packet.nmb;

	nmb->header.opcode = NMB_NAME_REG_OPCODE;
	nmb->header.arcount = 1;

	nmb->header.nm_flags.recursion_desired = True;

	if(create_and_init_additional_record(packet, nb_flags, register_ip) == False)
		return False;

	DEBUG(4,("initiate_name_register_packet: sending registration for name %s (bcast=%s) to IP %s\n",
		nmb_namestr(&nmb->additional->rr_name),
		BOOLSTR(nmb->header.nm_flags.bcast), inet_ntoa(packet->ip)));

	return send_netbios_packet( packet );
}

/***************************************************************************
 Sends out a multihomed name register.
**************************************************************************/

static BOOL initiate_multihomed_name_register_packet(struct packet_struct *packet,
						     uint16 nb_flags, struct in_addr *register_ip)
{
	struct nmb_packet *nmb = &packet->packet.nmb;
	fstring second_ip_buf;

	fstrcpy(second_ip_buf, inet_ntoa(packet->ip));

	nmb->header.opcode = NMB_NAME_MULTIHOMED_REG_OPCODE;
	nmb->header.arcount = 1;

	nmb->header.nm_flags.recursion_desired = True;
	
	if(create_and_init_additional_record(packet, nb_flags, register_ip) == False)
		return False;
	
	DEBUG(4,("initiate_multihomed_name_register_packet: sending registration \
for name %s IP %s (bcast=%s) to IP %s\n",
		 nmb_namestr(&nmb->additional->rr_name), inet_ntoa(*register_ip),
		 BOOLSTR(nmb->header.nm_flags.bcast), second_ip_buf ));

	return send_netbios_packet( packet );
} 

/***************************************************************************
 Sends out a name refresh.
**************************************************************************/

static BOOL initiate_name_refresh_packet( struct packet_struct *packet,
                                   uint16 nb_flags, struct in_addr *refresh_ip)
{
	struct nmb_packet *nmb = &packet->packet.nmb;

	nmb->header.opcode = NMB_NAME_REFRESH_OPCODE_8;
	nmb->header.arcount = 1;

	nmb->header.nm_flags.recursion_desired = False;

	if(create_and_init_additional_record(packet, nb_flags, refresh_ip) == False)
		return False;

	DEBUG(4,("initiate_name_refresh_packet: sending refresh for name %s (bcast=%s) to IP %s\n",
		nmb_namestr(&nmb->additional->rr_name),
		BOOLSTR(nmb->header.nm_flags.bcast), inet_ntoa(packet->ip)));

	return send_netbios_packet( packet );
} 

/***************************************************************************
 Sends out a name release.
**************************************************************************/

static BOOL initiate_name_release_packet( struct packet_struct *packet,
                                   uint16 nb_flags, struct in_addr *release_ip)
{
	struct nmb_packet *nmb = &packet->packet.nmb;

	nmb->header.opcode = NMB_NAME_RELEASE_OPCODE;
	nmb->header.arcount = 1;

	nmb->header.nm_flags.recursion_desired = False;

	if(create_and_init_additional_record(packet, nb_flags, release_ip) == False)
		return False;

	DEBUG(4,("initiate_name_release_packet: sending release for name %s (bcast=%s) to IP %s\n",
		nmb_namestr(&nmb->additional->rr_name),
		BOOLSTR(nmb->header.nm_flags.bcast), inet_ntoa(packet->ip)));

	return send_netbios_packet( packet );
} 

/***************************************************************************
 Sends out a node status.
**************************************************************************/

static BOOL initiate_node_status_packet( struct packet_struct *packet )
{
	struct nmb_packet *nmb = &packet->packet.nmb;

	nmb->header.opcode = NMB_NAME_QUERY_OPCODE;
	nmb->header.arcount = 0;

	nmb->header.nm_flags.recursion_desired = False;

	nmb->question.question_type = QUESTION_TYPE_NB_STATUS;

	DEBUG(4,("initiate_node_status_packet: sending node status request for name %s to IP %s\n",
		nmb_namestr(&nmb->question.question_name),
		inet_ntoa(packet->ip)));

	return send_netbios_packet( packet );
}

/****************************************************************************
  Simplification functions for queuing standard packets.
  These should be the only publicly callable functions for sending
  out packets.
****************************************************************************/

/****************************************************************************
 Assertion - we should never be sending nmbd packets on the remote
 broadcast subnet.
****************************************************************************/

static BOOL assert_check_subnet(struct subnet_record *subrec)
{
	if( subrec == remote_broadcast_subnet) {
		DEBUG(0,("assert_check_subnet: Attempt to send packet on remote broadcast subnet. \
This is a bug.\n"));
		return True;
	}
	return False;
}

/****************************************************************************
 Queue a register name packet to the broadcast address of a subnet.
****************************************************************************/

struct response_record *queue_register_name( struct subnet_record *subrec,
                          response_function resp_fn,
                          timeout_response_function timeout_fn,
                          register_name_success_function success_fn,
                          register_name_fail_function fail_fn,
                          struct userdata_struct *userdata,
                          struct nmb_name *nmbname,
                          uint16 nb_flags)
{
	struct packet_struct *p;
	struct response_record *rrec;

	if(assert_check_subnet(subrec))
		return NULL;

	/* note that all name registration requests have RD set (rfc1002 - section 4.2.2 */
	if ((p = create_and_init_netbios_packet(nmbname, (subrec != unicast_subnet), True,
				subrec->bcast_ip)) == NULL)
		return NULL;

	if(initiate_name_register_packet( p, nb_flags, iface_ip(subrec->bcast_ip)) == False) {
		p->locked = False;
		free_packet(p);
		return NULL;
	}

	if((rrec = make_response_record(subrec,        /* subnet record. */
				p,                     /* packet we sent. */
				resp_fn,               /* function to call on response. */
				timeout_fn,            /* function to call on timeout. */
				(success_function)success_fn,            /* function to call on operation success. */
				(fail_function)fail_fn,               /* function to call on operation fail. */
				userdata)) == NULL)  {
		p->locked = False;
		free_packet(p);
		return NULL;
	}

	return rrec;
}

/****************************************************************************
 Queue a refresh name packet to the broadcast address of a subnet.
****************************************************************************/

void queue_wins_refresh(struct nmb_name *nmbname,
			response_function resp_fn,
			timeout_response_function timeout_fn,
			uint16 nb_flags,
			struct in_addr refresh_ip,
			const char *tag)
{
	struct packet_struct *p;
	struct response_record *rrec;
	struct in_addr wins_ip;
	struct userdata_struct *userdata;
	fstring ip_str;

	wins_ip = wins_srv_ip_tag(tag, refresh_ip);

	if ((p = create_and_init_netbios_packet(nmbname, False, False, wins_ip)) == NULL) {
		return;
	}

	if (!initiate_name_refresh_packet(p, nb_flags, &refresh_ip)) {
		p->locked = False;
		free_packet(p);
		return;
	}

	fstrcpy(ip_str, inet_ntoa(refresh_ip));

	DEBUG(6,("Refreshing name %s IP %s with WINS server %s using tag '%s'\n",
		 nmb_namestr(nmbname), ip_str, inet_ntoa(wins_ip), tag));

	userdata = (struct userdata_struct *)SMB_MALLOC(sizeof(*userdata) + strlen(tag) + 1);
	if (!userdata) {
		p->locked = False;
		free_packet(p);
		DEBUG(0,("Failed to allocate userdata structure!\n"));
		return;
	}
	ZERO_STRUCTP(userdata);
	userdata->userdata_len = strlen(tag) + 1;
	strlcpy(userdata->data, tag, userdata->userdata_len);	

	if ((rrec = make_response_record(unicast_subnet,
					 p,
					 resp_fn, timeout_fn,
					 NULL,
					 NULL,
					 userdata)) == NULL) {
		p->locked = False;
		free_packet(p);
		return;
	}

	free(userdata);

	/* we don't want to repeat refresh packets */
	rrec->repeat_count = 0;
}


/****************************************************************************
 Queue a multihomed register name packet to a given WINS server IP
****************************************************************************/

struct response_record *queue_register_multihomed_name( struct subnet_record *subrec,
							response_function resp_fn,
							timeout_response_function timeout_fn,
							register_name_success_function success_fn,
							register_name_fail_function fail_fn,
							struct userdata_struct *userdata,
							struct nmb_name *nmbname,
							uint16 nb_flags,
							struct in_addr register_ip,
							struct in_addr wins_ip)
{
	struct packet_struct *p;
	struct response_record *rrec;
	BOOL ret;
	
	/* Sanity check. */
	if(subrec != unicast_subnet) {
		DEBUG(0,("queue_register_multihomed_name: should only be done on \
unicast subnet. subnet is %s\n.", subrec->subnet_name ));
		return NULL;
	}

	if(assert_check_subnet(subrec))
		return NULL;
     
	if ((p = create_and_init_netbios_packet(nmbname, False, True, wins_ip)) == NULL)
		return NULL;

	if (nb_flags & NB_GROUP)
		ret = initiate_name_register_packet( p, nb_flags, &register_ip);
	else
		ret = initiate_multihomed_name_register_packet(p, nb_flags, &register_ip);

	if (ret == False) {  
		p->locked = False;
		free_packet(p);
		return NULL;
	}  
  
	if ((rrec = make_response_record(subrec,    /* subnet record. */
					 p,                     /* packet we sent. */
					 resp_fn,               /* function to call on response. */
					 timeout_fn,            /* function to call on timeout. */
					 (success_function)success_fn, /* function to call on operation success. */
					 (fail_function)fail_fn,       /* function to call on operation fail. */
					 userdata)) == NULL) {  
		p->locked = False;
		free_packet(p);
		return NULL;
	}  
	
	return rrec;
}

/****************************************************************************
 Queue a release name packet to the broadcast address of a subnet.
****************************************************************************/

struct response_record *queue_release_name( struct subnet_record *subrec,
					    response_function resp_fn,
					    timeout_response_function timeout_fn,
					    release_name_success_function success_fn,
					    release_name_fail_function fail_fn,
					    struct userdata_struct *userdata,
					    struct nmb_name *nmbname,
					    uint16 nb_flags,
					    struct in_addr release_ip,
					    struct in_addr dest_ip)
{
	struct packet_struct *p;
	struct response_record *rrec;

	if(assert_check_subnet(subrec))
		return NULL;

	if ((p = create_and_init_netbios_packet(nmbname, (subrec != unicast_subnet), False, dest_ip)) == NULL)
		return NULL;

	if(initiate_name_release_packet( p, nb_flags, &release_ip) == False) {
		p->locked = False;
		free_packet(p);
		return NULL;
	}

	if((rrec = make_response_record(subrec,                /* subnet record. */
					p,                     /* packet we sent. */
					resp_fn,               /* function to call on response. */
					timeout_fn,            /* function to call on timeout. */
					(success_function)success_fn,            /* function to call on operation success. */
					(fail_function)fail_fn,               /* function to call on operation fail. */
					userdata)) == NULL)  {
		p->locked = False;
		free_packet(p);
		return NULL;
	}

	/*
	 * For a broadcast release packet, only send once.
	 * This will cause us to remove the name asap. JRA.
	 */

	if (subrec != unicast_subnet) {
		rrec->repeat_count = 0;
		rrec->repeat_time = 0;
	}

	return rrec;
}

/****************************************************************************
 Queue a query name packet to the broadcast address of a subnet.
****************************************************************************/
 
struct response_record *queue_query_name( struct subnet_record *subrec,
                          response_function resp_fn,
                          timeout_response_function timeout_fn,
                          query_name_success_function success_fn,
                          query_name_fail_function fail_fn,
                          struct userdata_struct *userdata,
                          struct nmb_name *nmbname)
{
	struct packet_struct *p;
	struct response_record *rrec;
	struct in_addr to_ip;

	if(assert_check_subnet(subrec))
		return NULL;

	to_ip = subrec->bcast_ip;
  
	/* queries to the WINS server turn up here as queries to IP 0.0.0.0 
			These need to be handled a bit differently */
	if (subrec->type == UNICAST_SUBNET && is_zero_ip(to_ip)) {
		/* What we really need to do is loop over each of our wins
		 * servers and wins server tags here, but that just doesn't
		 * fit our architecture at the moment (userdata may already
		 * be used when we get here). For now we just query the first
		 * active wins server on the first tag.
		 */ 
		char **tags = wins_srv_tags();
		if (!tags) {
			return NULL;
		}
		to_ip = wins_srv_ip_tag(tags[0], to_ip);
		wins_srv_tags_free(tags);
	}

	if(( p = create_and_init_netbios_packet(nmbname, 
					(subrec != unicast_subnet), 
					(subrec == unicast_subnet), 
					to_ip)) == NULL)
		return NULL;

	if(lp_bind_interfaces_only()) {
		int i;

		DEBUG(10,("queue_query_name: bind_interfaces_only is set, looking for suitable source IP\n"));
		for(i = 0; i < iface_count(); i++) {
			struct in_addr *ifip = iface_n_ip(i);

			if(ifip == NULL) {
				DEBUG(0,("queue_query_name: interface %d has NULL IP address !\n", i));
				continue;
			}

			if (ip_equal(*ifip,loopback_ip)) {
				DEBUG(5,("queue_query_name: ignoring loopback interface (%d)\n", i));
				continue;
			}

			DEBUG(10,("queue_query_name: using source IP %s\n",inet_ntoa(*ifip)));
				p->fd = find_subnet_fd_for_address( *ifip );
				break;
		}
	}

	if(initiate_name_query_packet( p ) == False) {
		p->locked = False;
		free_packet(p);
		return NULL;
	}

	if((rrec = make_response_record(subrec,                /* subnet record. */
					p,                     /* packet we sent. */
					resp_fn,               /* function to call on response. */
					timeout_fn,            /* function to call on timeout. */
					(success_function)success_fn,            /* function to call on operation success. */
					(fail_function)fail_fn,               /* function to call on operation fail. */
					userdata)) == NULL) {
		p->locked = False;
		free_packet(p);
		return NULL;
	}

	return rrec;
}

/****************************************************************************
 Queue a query name packet to a given address from the WINS subnet.
****************************************************************************/
 
struct response_record *queue_query_name_from_wins_server( struct in_addr to_ip,
                          response_function resp_fn,
                          timeout_response_function timeout_fn,
                          query_name_success_function success_fn,
                          query_name_fail_function fail_fn,
                          struct userdata_struct *userdata,
                          struct nmb_name *nmbname)
{
	struct packet_struct *p;
	struct response_record *rrec;

	if ((p = create_and_init_netbios_packet(nmbname, False, False, to_ip)) == NULL)
		return NULL;

	if(initiate_name_query_packet_from_wins_server( p ) == False) {
		p->locked = False;
		free_packet(p);
		return NULL;
	}

	if((rrec = make_response_record(wins_server_subnet,            /* subnet record. */
						p,                     /* packet we sent. */
						resp_fn,               /* function to call on response. */
						timeout_fn,            /* function to call on timeout. */
						(success_function)success_fn,            /* function to call on operation success. */
						(fail_function)fail_fn,               /* function to call on operation fail. */
						userdata)) == NULL) {
		p->locked = False;
		free_packet(p);
		return NULL;
	}

	return rrec;
}

/****************************************************************************
 Queue a node status packet to a given name and address.
****************************************************************************/
 
struct response_record *queue_node_status( struct subnet_record *subrec,
                          response_function resp_fn,
                          timeout_response_function timeout_fn,
                          node_status_success_function success_fn,
                          node_status_fail_function fail_fn,
                          struct userdata_struct *userdata,
                          struct nmb_name *nmbname,
                          struct in_addr send_ip)
{
	struct packet_struct *p;
	struct response_record *rrec;

	/* Sanity check. */
	if(subrec != unicast_subnet) {
		DEBUG(0,("queue_register_multihomed_name: should only be done on \
unicast subnet. subnet is %s\n.", subrec->subnet_name ));
		return NULL;
	}

	if(assert_check_subnet(subrec))
		return NULL;

	if(( p = create_and_init_netbios_packet(nmbname, False, False, send_ip)) == NULL)
		return NULL;

	if(initiate_node_status_packet(p) == False) {
		p->locked = False;
		free_packet(p);
		return NULL;
	} 

	if((rrec = make_response_record(subrec,           /* subnet record. */
					p,                     /* packet we sent. */
					resp_fn,               /* function to call on response. */
					timeout_fn,            /* function to call on timeout. */
					(success_function)success_fn,            /* function to call on operation success. */
					(fail_function)fail_fn,               /* function to call on operation fail. */
					userdata)) == NULL) {
		p->locked = False;
		free_packet(p);
		return NULL;
	}

	return rrec;
}

/****************************************************************************
  Reply to a netbios name packet.  see rfc1002.txt
****************************************************************************/

void reply_netbios_packet(struct packet_struct *orig_packet,
                          int rcode, enum netbios_reply_type_code rcv_code, int opcode,
                          int ttl, char *data,int len)
{
	struct packet_struct packet;
	struct nmb_packet *nmb = NULL;
	struct res_rec answers;
	struct nmb_packet *orig_nmb = &orig_packet->packet.nmb;
	BOOL loopback_this_packet = False;
	int rr_type = RR_TYPE_NB;
	const char *packet_type = "unknown";
  
	/* Check if we are sending to or from ourselves. */
	if(ismyip(orig_packet->ip) && (orig_packet->port == global_nmb_port))
		loopback_this_packet = True;
  
	nmb = &packet.packet.nmb;

	/* Do a partial copy of the packet. We clear the locked flag and
			the resource record pointers. */
	packet = *orig_packet;   /* Full structure copy. */
	packet.locked = False;
	nmb->answers = NULL;
	nmb->nsrecs = NULL;
	nmb->additional = NULL;

	switch (rcv_code) {
		case NMB_STATUS:
			packet_type = "nmb_status";
			nmb->header.nm_flags.recursion_desired = False;
			nmb->header.nm_flags.recursion_available = False;
			rr_type = RR_TYPE_NBSTAT;
			break;
		case NMB_QUERY:
			packet_type = "nmb_query";
			nmb->header.nm_flags.recursion_desired = True;
			nmb->header.nm_flags.recursion_available = True;
			if (rcode) {
				rr_type = RR_TYPE_NULL;
			}
			break;
		case NMB_REG:
		case NMB_REG_REFRESH:
			packet_type = "nmb_reg";
			nmb->header.nm_flags.recursion_desired = True;
			nmb->header.nm_flags.recursion_available = True;
			break;
		case NMB_REL:
			packet_type = "nmb_rel";
			nmb->header.nm_flags.recursion_desired = False;
			nmb->header.nm_flags.recursion_available = False;
			break;
		case NMB_WAIT_ACK:
			packet_type = "nmb_wack";
			nmb->header.nm_flags.recursion_desired = False;
			nmb->header.nm_flags.recursion_available = False;
			rr_type = RR_TYPE_NULL;
			break;
		case WINS_REG:
			packet_type = "wins_reg";
			nmb->header.nm_flags.recursion_desired = True;
			nmb->header.nm_flags.recursion_available = True;
			break;
		case WINS_QUERY:
			packet_type = "wins_query";
			nmb->header.nm_flags.recursion_desired = True;
			nmb->header.nm_flags.recursion_available = True;
			if (rcode) {
				rr_type = RR_TYPE_NULL;
			}
			break;
		default:
			DEBUG(0,("reply_netbios_packet: Unknown packet type: %s %s to ip %s\n",
				packet_type, nmb_namestr(&orig_nmb->question.question_name),
				inet_ntoa(packet.ip)));
			return;
	}

	DEBUG(4,("reply_netbios_packet: sending a reply of packet type: %s %s to ip %s \
for id %hu\n", packet_type, nmb_namestr(&orig_nmb->question.question_name),
			inet_ntoa(packet.ip), orig_nmb->header.name_trn_id));

	nmb->header.name_trn_id = orig_nmb->header.name_trn_id;
	nmb->header.opcode = opcode;
	nmb->header.response = True;
	nmb->header.nm_flags.bcast = False;
	nmb->header.nm_flags.trunc = False;
	nmb->header.nm_flags.authoritative = True;
  
	nmb->header.rcode = rcode;
	nmb->header.qdcount = 0;
	nmb->header.ancount = 1;
	nmb->header.nscount = 0;
	nmb->header.arcount = 0;
  
	memset((char*)&nmb->question,'\0',sizeof(nmb->question));
  
	nmb->answers = &answers;
	memset((char*)nmb->answers,'\0',sizeof(*nmb->answers));
  
	nmb->answers->rr_name  = orig_nmb->question.question_name;
	nmb->answers->rr_type  = rr_type;
	nmb->answers->rr_class = RR_CLASS_IN;
	nmb->answers->ttl      = ttl;
  
	if (data && len) {
		nmb->answers->rdlength = len;
		memcpy(nmb->answers->rdata, data, len);
	}
  
	packet.packet_type = NMB_PACKET;
	/* Ensure we send out on the same fd that the original
		packet came in on to give the correct source IP address. */
	packet.fd = orig_packet->fd;
	packet.timestamp = time(NULL);

	debug_nmb_packet(&packet);
  
	if(loopback_this_packet) {
		struct packet_struct *lo_packet;
		DEBUG(5,("reply_netbios_packet: sending packet to ourselves.\n"));
		if((lo_packet = copy_packet(&packet)) == NULL)
			return;
		queue_packet(lo_packet);
	} else if (!send_packet(&packet)) {
		DEBUG(0,("reply_netbios_packet: send_packet to IP %s port %d failed\n",
			inet_ntoa(packet.ip),packet.port));
	}
}

/*******************************************************************
  Queue a packet into a packet queue
******************************************************************/

static void queue_packet(struct packet_struct *packet)
{
	struct packet_struct *p;

	if (!packet_queue) {
		packet->prev = NULL;
		packet->next = NULL;
		packet_queue = packet;
		return;
	}
  
	/* find the bottom */
	for (p=packet_queue;p->next;p=p->next) 
		;

	p->next = packet;
	packet->next = NULL;
	packet->prev = p;
}

/****************************************************************************
 Try and find a matching subnet record for a datagram port 138 packet.
****************************************************************************/

static struct subnet_record *find_subnet_for_dgram_browse_packet(struct packet_struct *p)
{
	struct subnet_record *subrec;

	/* Go through all the broadcast subnets and see if the mask matches. */
	for (subrec = FIRST_SUBNET; subrec ; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
		if(same_net(p->ip, subrec->bcast_ip, subrec->mask_ip))
			return subrec;
	}

	/* If the subnet record is the remote announce broadcast subnet,
		hack it here to be the first subnet. This is really gross and
		is needed due to people turning on port 137/138 broadcast
		forwarding on their routers. May fire and brimstone rain
		down upon them...
	*/

	return FIRST_SUBNET;
}

/****************************************************************************
Dispatch a browse frame from port 138 to the correct processing function.
****************************************************************************/

static void process_browse_packet(struct packet_struct *p, char *buf,int len)
{
	struct dgram_packet *dgram = &p->packet.dgram;
	int command = CVAL(buf,0);
	struct subnet_record *subrec = find_subnet_for_dgram_browse_packet(p);
	char scope[64];
	unstring src_name;

	/* Drop the packet if it's a different NetBIOS scope, or the source is from one of our names. */
	pull_ascii(scope, dgram->dest_name.scope, 64, 64, STR_TERMINATE);
	if (!strequal(scope, global_scope())) {
		DEBUG(7,("process_browse_packet: Discarding datagram from IP %s. Scope (%s) \
mismatch with our scope (%s).\n", inet_ntoa(p->ip), scope, global_scope()));
		return;
	}

	pull_ascii_nstring(src_name, sizeof(src_name), dgram->source_name.name);
	if (is_myname(src_name)) {
		DEBUG(0,("process_browse_packet: Discarding datagram from IP %s. Source name \
%s is one of our names !\n", inet_ntoa(p->ip), nmb_namestr(&dgram->source_name)));
		return;
	}

	switch (command) {
		case ANN_HostAnnouncement:
			debug_browse_data(buf, len);
			process_host_announce(subrec, p, buf+1);
			break;
		case ANN_DomainAnnouncement:
			debug_browse_data(buf, len);
			process_workgroup_announce(subrec, p, buf+1);
			break;
		case ANN_LocalMasterAnnouncement:
			debug_browse_data(buf, len);
			process_local_master_announce(subrec, p, buf+1);
			break;
		case ANN_AnnouncementRequest:
			debug_browse_data(buf, len);
			process_announce_request(subrec, p, buf+1);
			break;
		case ANN_Election:
			debug_browse_data(buf, len);
			process_election(subrec, p, buf+1);
			break;
		case ANN_GetBackupListReq:
			debug_browse_data(buf, len);
			process_get_backup_list_request(subrec, p, buf+1);
			break;
		case ANN_GetBackupListResp:
			debug_browse_data(buf, len);
			/* We never send ANN_GetBackupListReq so we should never get these. */
			DEBUG(0,("process_browse_packet: Discarding GetBackupListResponse \
packet from %s IP %s\n", nmb_namestr(&dgram->source_name), inet_ntoa(p->ip)));
			break;
		case ANN_ResetBrowserState:
			debug_browse_data(buf, len);
			process_reset_browser(subrec, p, buf+1);
			break;
		case ANN_MasterAnnouncement:
			/* Master browser datagrams must be processed on the unicast subnet. */
			subrec = unicast_subnet;

			debug_browse_data(buf, len);
			process_master_browser_announce(subrec, p, buf+1);
			break;
		case ANN_BecomeBackup:
			/* 
			 * We don't currently implement this. Log it just in case.
			 */
			debug_browse_data(buf, len);
			DEBUG(10,("process_browse_packet: On subnet %s ignoring browse packet \
command ANN_BecomeBackup from %s IP %s to %s\n", subrec->subnet_name, nmb_namestr(&dgram->source_name),
					inet_ntoa(p->ip), nmb_namestr(&dgram->dest_name)));
			break;
		default:
			debug_browse_data(buf, len);
			DEBUG(0,("process_browse_packet: On subnet %s ignoring browse packet \
command code %d from %s IP %s to %s\n", subrec->subnet_name, command, nmb_namestr(&dgram->source_name),
				inet_ntoa(p->ip), nmb_namestr(&dgram->dest_name)));
			break;
	} 
}

/****************************************************************************
 Dispatch a LanMan browse frame from port 138 to the correct processing function.
****************************************************************************/

static void process_lanman_packet(struct packet_struct *p, char *buf,int len)
{
	struct dgram_packet *dgram = &p->packet.dgram;
	int command = SVAL(buf,0);
	struct subnet_record *subrec = find_subnet_for_dgram_browse_packet(p);
	char scope[64];
	unstring src_name;

	/* Drop the packet if it's a different NetBIOS scope, or the source is from one of our names. */

	pull_ascii(scope, dgram->dest_name.scope, 64, 64, STR_TERMINATE);
	if (!strequal(scope, global_scope())) {
		DEBUG(7,("process_lanman_packet: Discarding datagram from IP %s. Scope (%s) \
mismatch with our scope (%s).\n", inet_ntoa(p->ip), scope, global_scope()));
		return;
	}

	pull_ascii_nstring(src_name, sizeof(src_name), dgram->source_name.name);
	if (is_myname(src_name)) {
		DEBUG(0,("process_lanman_packet: Discarding datagram from IP %s. Source name \
%s is one of our names !\n", inet_ntoa(p->ip), nmb_namestr(&dgram->source_name)));
		return;
	}

	switch (command) {
		case ANN_HostAnnouncement:
			debug_browse_data(buf, len);
			process_lm_host_announce(subrec, p, buf+1);
			break;
		case ANN_AnnouncementRequest:
			process_lm_announce_request(subrec, p, buf+1);
			break;
		default:
			DEBUG(0,("process_lanman_packet: On subnet %s ignoring browse packet \
command code %d from %s IP %s to %s\n", subrec->subnet_name, command, nmb_namestr(&dgram->source_name),
				inet_ntoa(p->ip), nmb_namestr(&dgram->dest_name)));
			break;
	}
}

/****************************************************************************
  Determine if a packet is for us on port 138. Note that to have any chance of
  being efficient we need to drop as many packets as possible at this
  stage as subsequent processing is expensive. 
****************************************************************************/

static BOOL listening(struct packet_struct *p,struct nmb_name *nbname)
{
	struct subnet_record *subrec = NULL;

	for (subrec = FIRST_SUBNET; subrec ; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
		if(same_net(p->ip, subrec->bcast_ip, subrec->mask_ip))
			break;
	}

	if(subrec == NULL)
		subrec = unicast_subnet;

	return (find_name_on_subnet(subrec, nbname, FIND_SELF_NAME) != NULL);
}

/****************************************************************************
  Process udp 138 datagrams
****************************************************************************/

static void process_dgram(struct packet_struct *p)
{
	char *buf;
	char *buf2;
	int len;
	struct dgram_packet *dgram = &p->packet.dgram;

	/* If we aren't listening to the destination name then ignore the packet */
	if (!listening(p,&dgram->dest_name)) {
			unexpected_packet(p);
			DEBUG(5,("process_dgram: ignoring dgram packet sent to name %s from %s\n",
				nmb_namestr(&dgram->dest_name), inet_ntoa(p->ip)));
			return;
	}

	if (dgram->header.msg_type != 0x10 && dgram->header.msg_type != 0x11 && dgram->header.msg_type != 0x12) {
		unexpected_packet(p);
		/* Don't process error packets etc yet */
		DEBUG(5,("process_dgram: ignoring dgram packet sent to name %s from IP %s as it is \
an error packet of type %x\n", nmb_namestr(&dgram->dest_name), inet_ntoa(p->ip), dgram->header.msg_type));
		return;
	}

	/* Ensure we have a large enough packet before looking inside. */
	if (dgram->datasize < (smb_vwv12 - 2)) {
		/* That's the offset minus the 4 byte length + 2 bytes of offset. */
		DEBUG(0,("process_dgram: ignoring too short dgram packet (%u) sent to name %s from IP %s\n",
			(unsigned int)dgram->datasize,
			nmb_namestr(&dgram->dest_name),
			inet_ntoa(p->ip) ));
		return;
	}

	buf = &dgram->data[0];
	buf -= 4; /* XXXX for the pseudo tcp length - someday I need to get rid of this */

	if (CVAL(buf,smb_com) != SMBtrans)
		return;

	len = SVAL(buf,smb_vwv11);
	buf2 = smb_base(buf) + SVAL(buf,smb_vwv12);

	if (len <= 0 || len > dgram->datasize) {
		DEBUG(0,("process_dgram: ignoring malformed1 (datasize = %d, len = %d) datagram \
packet sent to name %s from IP %s\n",
			dgram->datasize,
			len,
			nmb_namestr(&dgram->dest_name),
			inet_ntoa(p->ip) ));
		return;
	}

	if (buf2 < dgram->data || (buf2 >= dgram->data + dgram->datasize)) {
		DEBUG(0,("process_dgram: ignoring malformed2 (datasize = %d, len=%d, off=%d) datagram \
packet sent to name %s from IP %s\n",
			dgram->datasize,
			len,
			(int)PTR_DIFF(buf2, dgram->data),
			nmb_namestr(&dgram->dest_name),
			inet_ntoa(p->ip) ));
		return;
	}

	if ((buf2 + len < dgram->data) || (buf2 + len > dgram->data + dgram->datasize)) {
		DEBUG(0,("process_dgram: ignoring malformed3 (datasize = %d, len=%d, off=%d) datagram \
packet sent to name %s from IP %s\n",
			dgram->datasize,
			len,
			(int)PTR_DIFF(buf2, dgram->data),
			nmb_namestr(&dgram->dest_name),
			inet_ntoa(p->ip) ));
		return;
	}

	DEBUG(4,("process_dgram: datagram from %s to %s IP %s for %s of type %d len=%d\n",
		nmb_namestr(&dgram->source_name),nmb_namestr(&dgram->dest_name),
		inet_ntoa(p->ip), smb_buf(buf),CVAL(buf2,0),len));

	/* Datagram packet received for the browser mailslot */
	if (strequal(smb_buf(buf),BROWSE_MAILSLOT)) {
		process_browse_packet(p,buf2,len);
		return;
	}

	/* Datagram packet received for the LAN Manager mailslot */
	if (strequal(smb_buf(buf),LANMAN_MAILSLOT)) {
		process_lanman_packet(p,buf2,len);
		return;
	}

	/* Datagram packet received for the domain logon mailslot */
	if (strequal(smb_buf(buf),NET_LOGON_MAILSLOT)) {
		process_logon_packet(p,buf2,len,NET_LOGON_MAILSLOT);
		return;
	}

	/* Datagram packet received for the NT domain logon mailslot */
	if (strequal(smb_buf(buf),NT_LOGON_MAILSLOT)) {
		process_logon_packet(p,buf2,len,NT_LOGON_MAILSLOT);
		return;
	}

	unexpected_packet(p);
}

/****************************************************************************
  Validate a response nmb packet.
****************************************************************************/

static BOOL validate_nmb_response_packet( struct nmb_packet *nmb )
{
	BOOL ignore = False;

	switch (nmb->header.opcode) {
		case NMB_NAME_REG_OPCODE:
		case NMB_NAME_REFRESH_OPCODE_8: /* ambiguity in rfc1002 about which is correct. */
		case NMB_NAME_REFRESH_OPCODE_9: /* WinNT uses 8 by default. */
			if (nmb->header.ancount == 0) {
				DEBUG(0,("validate_nmb_response_packet: Bad REG/REFRESH Packet. "));
				ignore = True;
			}
			break;

		case NMB_NAME_QUERY_OPCODE:
			if ((nmb->header.ancount != 0) && (nmb->header.ancount != 1)) {
				DEBUG(0,("validate_nmb_response_packet: Bad QUERY Packet. "));
				ignore = True;
			}
			break;

		case NMB_NAME_RELEASE_OPCODE:
			if (nmb->header.ancount == 0) {
				DEBUG(0,("validate_nmb_response_packet: Bad RELEASE Packet. "));
				ignore = True;
			}
			break;

		case NMB_WACK_OPCODE:
			/* Check WACK response here. */
			if (nmb->header.ancount != 1) {
				DEBUG(0,("validate_nmb_response_packet: Bad WACK Packet. "));
				ignore = True;
			}
			break;
		default:
			DEBUG(0,("validate_nmb_response_packet: Ignoring packet with unknown opcode %d.\n",
					nmb->header.opcode));
			return True;
	}

	if(ignore)
		DEBUG(0,("Ignoring response packet with opcode %d.\n", nmb->header.opcode));

	return ignore;
}
 
/****************************************************************************
  Validate a request nmb packet.
****************************************************************************/

static BOOL validate_nmb_packet( struct nmb_packet *nmb )
{
	BOOL ignore = False;

	switch (nmb->header.opcode) {
		case NMB_NAME_REG_OPCODE:
		case NMB_NAME_REFRESH_OPCODE_8: /* ambiguity in rfc1002 about which is correct. */
		case NMB_NAME_REFRESH_OPCODE_9: /* WinNT uses 8 by default. */
		case NMB_NAME_MULTIHOMED_REG_OPCODE:
			if (nmb->header.qdcount==0 || nmb->header.arcount==0) {
				DEBUG(0,("validate_nmb_packet: Bad REG/REFRESH Packet. "));
				ignore = True;
			}
			break;

		case NMB_NAME_QUERY_OPCODE:
			if ((nmb->header.qdcount == 0) || ((nmb->question.question_type != QUESTION_TYPE_NB_QUERY) &&
					(nmb->question.question_type != QUESTION_TYPE_NB_STATUS))) {
				DEBUG(0,("validate_nmb_packet: Bad QUERY Packet. "));
				ignore = True;
			}
			break;

		case NMB_NAME_RELEASE_OPCODE:
			if (nmb->header.qdcount==0 || nmb->header.arcount==0) {
				DEBUG(0,("validate_nmb_packet: Bad RELEASE Packet. "));
				ignore = True;
			}
			break;
		default:
			DEBUG(0,("validate_nmb_packet: Ignoring packet with unknown opcode %d.\n",
				nmb->header.opcode));
			return True;
	}

	if(ignore)
		DEBUG(0,("validate_nmb_packet: Ignoring request packet with opcode %d.\n", nmb->header.opcode));

	return ignore;
}

/****************************************************************************
  Find a subnet (and potentially a response record) for a packet.
****************************************************************************/

static struct subnet_record *find_subnet_for_nmb_packet( struct packet_struct *p,
                                                         struct response_record **pprrec)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	struct response_record *rrec = NULL;
	struct subnet_record *subrec = NULL;

	if(pprrec != NULL)
		*pprrec = NULL;

	if(nmb->header.response) {
		/* It's a response packet. Find a record for it or it's an error. */

		rrec = find_response_record( &subrec, nmb->header.name_trn_id);
		if(rrec == NULL) {
			DEBUG(3,("find_subnet_for_nmb_packet: response record not found for response id %hu\n",
				nmb->header.name_trn_id));
			unexpected_packet(p);
			return NULL;
		}

		if(subrec == NULL) {
			DEBUG(0,("find_subnet_for_nmb_packet: subnet record not found for response id %hu\n",
				nmb->header.name_trn_id));
			return NULL;
		}

		if(pprrec != NULL)
			*pprrec = rrec;
		return subrec;
	}

	/* Try and see what subnet this packet belongs to. */

	/* WINS server ? */
	if(packet_is_for_wins_server(p))
		return wins_server_subnet;

	/* If it wasn't a broadcast packet then send to the UNICAST subnet. */
	if(nmb->header.nm_flags.bcast == False)
		return unicast_subnet;

	/* Go through all the broadcast subnets and see if the mask matches. */
	for (subrec = FIRST_SUBNET; subrec ; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
		if(same_net(p->ip, subrec->bcast_ip, subrec->mask_ip))
			return subrec;
	}

	/* If none match it must have been a directed broadcast - assign the remote_broadcast_subnet. */
	return remote_broadcast_subnet;
}

/****************************************************************************
  Process a nmb request packet - validate the packet and route it.
****************************************************************************/

static void process_nmb_request(struct packet_struct *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	struct subnet_record *subrec = NULL;

	debug_nmb_packet(p);

	/* Ensure we have a good packet. */
	if(validate_nmb_packet(nmb))
		return;

	/* Allocate a subnet to this packet - if we cannot - fail. */
	if((subrec = find_subnet_for_nmb_packet(p, NULL))==NULL)
		return;

	switch (nmb->header.opcode) {
		case NMB_NAME_REG_OPCODE:
			if(subrec == wins_server_subnet)
				wins_process_name_registration_request(subrec, p);
			else
				process_name_registration_request(subrec, p);
			break;

		case NMB_NAME_REFRESH_OPCODE_8: /* ambiguity in rfc1002 about which is correct. */
		case NMB_NAME_REFRESH_OPCODE_9:
			if(subrec == wins_server_subnet)
				wins_process_name_refresh_request(subrec, p);
			else
				process_name_refresh_request(subrec, p);
			break;

		case NMB_NAME_MULTIHOMED_REG_OPCODE:
			if(subrec == wins_server_subnet) {
				wins_process_multihomed_name_registration_request(subrec, p);
			} else {
				DEBUG(0,("process_nmb_request: Multihomed registration request must be \
directed at a WINS server.\n"));
			}
			break;

		case NMB_NAME_QUERY_OPCODE:
			switch (nmb->question.question_type) {
				case QUESTION_TYPE_NB_QUERY:
					if(subrec == wins_server_subnet)
						wins_process_name_query_request(subrec, p);
					else
						process_name_query_request(subrec, p);
					break;
				case QUESTION_TYPE_NB_STATUS:
					if(subrec == wins_server_subnet) {
						DEBUG(0,("process_nmb_request: NB_STATUS request directed at WINS server is \
not allowed.\n"));
						break;
					} else {
						process_node_status_request(subrec, p);
					}
					break;
			}
			break;
      
		case NMB_NAME_RELEASE_OPCODE:
			if(subrec == wins_server_subnet)
				wins_process_name_release_request(subrec, p);
			else
				process_name_release_request(subrec, p);
			break;
	}
}

/****************************************************************************
  Process a nmb response packet - validate the packet and route it.
  to either the WINS server or a normal response.
****************************************************************************/

static void process_nmb_response(struct packet_struct *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	struct subnet_record *subrec = NULL;
	struct response_record *rrec = NULL;

	debug_nmb_packet(p);

	if(validate_nmb_response_packet(nmb))
		return;

	if((subrec = find_subnet_for_nmb_packet(p, &rrec))==NULL)
		return;

	if(rrec == NULL) {
		DEBUG(0,("process_nmb_response: response packet received but no response record \
found for id = %hu. Ignoring packet.\n", nmb->header.name_trn_id));
		return;
	}

	/* Increment the number of responses received for this record. */
	rrec->num_msgs++;
	/* Ensure we don't re-send the request. */
	rrec->repeat_count = 0;
  
	/* Call the response received function for this packet. */
	(*rrec->resp_fn)(subrec, rrec, p);
}

/*******************************************************************
  Run elements off the packet queue till its empty
******************************************************************/

void run_packet_queue(void)
{
	struct packet_struct *p;

	while ((p = packet_queue)) {
		packet_queue = p->next;
		if (packet_queue)
			packet_queue->prev = NULL;
		p->next = p->prev = NULL;

		switch (p->packet_type) {
			case NMB_PACKET:
				if(p->packet.nmb.header.response)
					process_nmb_response(p);
				else
					process_nmb_request(p);
				break;

			case DGRAM_PACKET:
				process_dgram(p);
				break;
		}
		free_packet(p);
	}
} 

/*******************************************************************
 Retransmit or timeout elements from all the outgoing subnet response
 record queues. NOTE that this code must also check the WINS server
 subnet for response records to timeout as the WINS server code
 can send requests to check if a client still owns a name.
 (Patch from Andrey Alekseyev <fetch@muffin.arcadia.spb.ru>).
******************************************************************/

void retransmit_or_expire_response_records(time_t t)
{
	struct subnet_record *subrec;

	for (subrec = FIRST_SUBNET; subrec; subrec = get_next_subnet_maybe_unicast_or_wins_server(subrec)) {
		struct response_record *rrec, *nextrrec;

		for (rrec = subrec->responselist; rrec; rrec = nextrrec) {
			nextrrec = rrec->next;
   
			if (rrec->repeat_time <= t) {
				if (rrec->repeat_count > 0) {
					/* Resend while we have a non-zero repeat_count. */
					if(!send_packet(rrec->packet)) {
						DEBUG(0,("retransmit_or_expire_response_records: Failed to resend packet id %hu \
to IP %s on subnet %s\n", rrec->response_id, inet_ntoa(rrec->packet->ip), subrec->subnet_name));
					}
					rrec->repeat_time = t + rrec->repeat_interval;
					rrec->repeat_count--;
				} else {
					DEBUG(4,("retransmit_or_expire_response_records: timeout for packet id %hu to IP %s \
on subnet %s\n", rrec->response_id, inet_ntoa(rrec->packet->ip), subrec->subnet_name));

					/*
					 * Check the flag in this record to prevent recursion if we end
					 * up in this function again via the timeout function call.
					 */

					if(!rrec->in_expiration_processing) {

						/*
						 * Set the recursion protection flag in this record.
						 */

						rrec->in_expiration_processing = True;

						/* Call the timeout function. This will deal with removing the
								timed out packet. */
						if(rrec->timeout_fn) {
							(*rrec->timeout_fn)(subrec, rrec);
						} else {
							/* We must remove the record ourself if there is
									no timeout function. */
							remove_response_record(subrec, rrec);
						}
					} /* !rrec->in_expitation_processing */
				} /* rrec->repeat_count > 0 */
			} /* rrec->repeat_time <= t */
		} /* end for rrec */
	} /* end for subnet */
}

/****************************************************************************
  Create an fd_set containing all the sockets in the subnet structures,
  plus the broadcast sockets.
***************************************************************************/

static BOOL create_listen_fdset(fd_set **ppset, int **psock_array, int *listen_number, int *maxfd)
{
	int *sock_array = NULL;
	struct subnet_record *subrec = NULL;
	int count = 0;
	int num = 0;
	fd_set *pset = SMB_MALLOC_P(fd_set);

	if(pset == NULL) {
		DEBUG(0,("create_listen_fdset: malloc fail !\n"));
		return True;
	}

	/* Check that we can add all the fd's we need. */
	for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec))
		count++;

	if((count*2) + 2 > FD_SETSIZE) {
		DEBUG(0,("create_listen_fdset: Too many file descriptors needed (%d). We can \
only use %d.\n", (count*2) + 2, FD_SETSIZE));
		SAFE_FREE(pset);
		return True;
	}

	if((sock_array = SMB_MALLOC_ARRAY(int, (count*2) + 2)) == NULL) {
		DEBUG(0,("create_listen_fdset: malloc fail for socket array.\n"));
		SAFE_FREE(pset);
		return True;
	}

	FD_ZERO(pset);

	/* Add in the broadcast socket on 137. */
	FD_SET(ClientNMB,pset);
	sock_array[num++] = ClientNMB;
	*maxfd = MAX( *maxfd, ClientNMB);

	/* Add in the 137 sockets on all the interfaces. */
	for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
		FD_SET(subrec->nmb_sock,pset);
		sock_array[num++] = subrec->nmb_sock;
		*maxfd = MAX( *maxfd, subrec->nmb_sock);
	}

	/* Add in the broadcast socket on 138. */
	FD_SET(ClientDGRAM,pset);
	sock_array[num++] = ClientDGRAM;
	*maxfd = MAX( *maxfd, ClientDGRAM);

	/* Add in the 138 sockets on all the interfaces. */
	for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
		FD_SET(subrec->dgram_sock,pset);
		sock_array[num++] = subrec->dgram_sock;
		*maxfd = MAX( *maxfd, subrec->dgram_sock);
	}

	*listen_number = (count*2) + 2;

	SAFE_FREE(*ppset);
	SAFE_FREE(*psock_array);

	*ppset = pset;
	*psock_array = sock_array;
 
	return False;
}

/****************************************************************************
  Listens for NMB or DGRAM packets, and queues them.
  return True if the socket is dead
***************************************************************************/

BOOL listen_for_packets(BOOL run_election)
{
	static fd_set *listen_set = NULL;
	static int listen_number = 0;
	static int *sock_array = NULL;
	int i;
	static int maxfd = 0;

	fd_set fds;
	int selrtn;
	struct timeval timeout;
#ifndef SYNC_DNS
	int dns_fd;
#endif

	if(listen_set == NULL || rescan_listen_set) {
		if(create_listen_fdset(&listen_set, &sock_array, &listen_number, &maxfd)) {
			DEBUG(0,("listen_for_packets: Fatal error. unable to create listen set. Exiting.\n"));
			return True;
		}
		rescan_listen_set = False;
	}

	memcpy((char *)&fds, (char *)listen_set, sizeof(fd_set));

#ifndef SYNC_DNS
	dns_fd = asyncdns_fd();
	if (dns_fd != -1) {
		FD_SET(dns_fd, &fds);
		maxfd = MAX( maxfd, dns_fd);
	}
#endif

	/* 
	 * During elections and when expecting a netbios response packet we
	 * need to send election packets at tighter intervals.
	 * Ideally it needs to be the interval (in ms) between time now and
	 * the time we are expecting the next netbios packet.
	 */

	timeout.tv_sec = (run_election||num_response_packets) ? 1 : NMBD_SELECT_LOOP;
	timeout.tv_usec = 0;

	/* Prepare for the select - allow certain signals. */

	BlockSignals(False, SIGTERM);

	selrtn = sys_select(maxfd+1,&fds,NULL,NULL,&timeout);

	/* We can only take signals when we are in the select - block them again here. */

	BlockSignals(True, SIGTERM);

	if(selrtn == -1) {
		return False;
	}

#ifndef SYNC_DNS
	if (dns_fd != -1 && FD_ISSET(dns_fd,&fds)) {
		run_dns_queue();
	}
#endif

	for(i = 0; i < listen_number; i++) {
		if (i < (listen_number/2)) {
			/* Processing a 137 socket. */
			if (FD_ISSET(sock_array[i],&fds)) {
				struct packet_struct *packet = read_packet(sock_array[i], NMB_PACKET);
				if (packet) {
					/*
					 * If we got a packet on the broadcast socket and interfaces
					 * only is set then check it came from one of our local nets. 
					 */
					if(lp_bind_interfaces_only() && (sock_array[i] == ClientNMB) && 
								(!is_local_net(packet->ip))) {
						DEBUG(7,("discarding nmb packet sent to broadcast socket from %s:%d\n",
							inet_ntoa(packet->ip),packet->port));	  
						free_packet(packet);
					} else if ((ip_equal(loopback_ip, packet->ip) || 
								ismyip(packet->ip)) && packet->port == global_nmb_port &&
								packet->packet.nmb.header.nm_flags.bcast) {
						DEBUG(7,("discarding own bcast packet from %s:%d\n",
							inet_ntoa(packet->ip),packet->port));	  
						free_packet(packet);
					} else {
						/* Save the file descriptor this packet came in on. */
						packet->fd = sock_array[i];
						queue_packet(packet);
					}
				}
			}
		} else {
			/* Processing a 138 socket. */
				if (FD_ISSET(sock_array[i],&fds)) {
				struct packet_struct *packet = read_packet(sock_array[i], DGRAM_PACKET);
				if (packet) {
					/*
					 * If we got a packet on the broadcast socket and interfaces
					 * only is set then check it came from one of our local nets. 
					 */
					if(lp_bind_interfaces_only() && (sock_array[i] == ClientDGRAM) && 
								(!is_local_net(packet->ip))) {
						DEBUG(7,("discarding dgram packet sent to broadcast socket from %s:%d\n",
						inet_ntoa(packet->ip),packet->port));	  
						free_packet(packet);
					} else if ((ip_equal(loopback_ip, packet->ip) || 
							ismyip(packet->ip)) && packet->port == DGRAM_PORT) {
						DEBUG(7,("discarding own dgram packet from %s:%d\n",
							inet_ntoa(packet->ip),packet->port));	  
						free_packet(packet);
					} else {
						/* Save the file descriptor this packet came in on. */
						packet->fd = sock_array[i];
						queue_packet(packet);
					}
				}
			}
		} /* end processing 138 socket. */
	} /* end for */
	return False;
}

/****************************************************************************
  Construct and send a netbios DGRAM.
**************************************************************************/

BOOL send_mailslot(BOOL unique, const char *mailslot,char *buf, size_t len,
                   const char *srcname, int src_type,
                   const char *dstname, int dest_type,
                   struct in_addr dest_ip,struct in_addr src_ip,
		   int dest_port)
{
	BOOL loopback_this_packet = False;
	struct packet_struct p;
	struct dgram_packet *dgram = &p.packet.dgram;
	char *ptr,*p2;
	char tmp[4];

	memset((char *)&p,'\0',sizeof(p));

	if(ismyip(dest_ip) && (dest_port == DGRAM_PORT)) /* Only if to DGRAM_PORT */
		loopback_this_packet = True;

	/* generate_name_trn_id(); */ /* Not used, so gone, RJS */

	/* DIRECT GROUP or UNIQUE datagram. */
	dgram->header.msg_type = unique ? 0x10 : 0x11; 
	dgram->header.flags.node_type = M_NODE;
	dgram->header.flags.first = True;
	dgram->header.flags.more = False;
	dgram->header.dgm_id = generate_name_trn_id();
	dgram->header.source_ip = src_ip;
	dgram->header.source_port = DGRAM_PORT;
	dgram->header.dgm_length = 0; /* Let build_dgram() handle this. */
	dgram->header.packet_offset = 0;
  
	make_nmb_name(&dgram->source_name,srcname,src_type);
	make_nmb_name(&dgram->dest_name,dstname,dest_type);

	ptr = &dgram->data[0];

	/* Setup the smb part. */
	ptr -= 4; /* XXX Ugliness because of handling of tcp SMB length. */
	memcpy(tmp,ptr,4);
	set_message(ptr,17,strlen(mailslot) + 1 + len,True);
	memcpy(ptr,tmp,4);

	SCVAL(ptr,smb_com,SMBtrans);
	SSVAL(ptr,smb_vwv1,len);
	SSVAL(ptr,smb_vwv11,len);
	SSVAL(ptr,smb_vwv12,70 + strlen(mailslot));
	SSVAL(ptr,smb_vwv13,3);
	SSVAL(ptr,smb_vwv14,1);
	SSVAL(ptr,smb_vwv15,1);
	SSVAL(ptr,smb_vwv16,2);
	p2 = smb_buf(ptr);
	safe_strcpy_base(p2, mailslot, dgram->data, sizeof(dgram->data));
	p2 = skip_string(p2,1);
  
	if (((p2+len) > dgram->data+sizeof(dgram->data)) || ((p2+len) < p2)) {
		DEBUG(0, ("send_mailslot: Cannot write beyond end of packet\n"));
		return False;
	} else {
		memcpy(p2,buf,len);
		p2 += len;
	}

	dgram->datasize = PTR_DIFF(p2,ptr+4); /* +4 for tcp length. */

	p.ip = dest_ip;
	p.port = dest_port;
	p.fd = find_subnet_mailslot_fd_for_address( src_ip );
	p.timestamp = time(NULL);
	p.packet_type = DGRAM_PACKET;

	DEBUG(4,("send_mailslot: Sending to mailslot %s from %s IP %s ", mailslot,
			nmb_namestr(&dgram->source_name), inet_ntoa(src_ip)));
	DEBUG(4,("to %s IP %s\n", nmb_namestr(&dgram->dest_name), inet_ntoa(dest_ip)));

	debug_browse_data(buf, len);

	if(loopback_this_packet) {
		struct packet_struct *lo_packet = NULL;
		DEBUG(5,("send_mailslot: sending packet to ourselves.\n"));
		if((lo_packet = copy_packet(&p)) == NULL)
			return False;
		queue_packet(lo_packet);
		return True;
	} else {
		return(send_packet(&p));
	}
}
