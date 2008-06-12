/* 
   Unix SMB/CIFS implementation.
   NBT netbios routines and daemon - version 2

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
Function called when the name lookup succeeded.
****************************************************************************/

static void wins_proxy_name_query_request_success( struct subnet_record *subrec,
                        struct userdata_struct *userdata,
                        struct nmb_name *nmbname, struct in_addr ip, struct res_rec *rrec)
{
	unstring name;
	struct packet_struct *original_packet;
	struct subnet_record *orig_broadcast_subnet;
	struct name_record *namerec = NULL;
	uint16 nb_flags;
	int num_ips;
	int i;
	int ttl = 3600; /* By default one hour in the cache. */
	struct in_addr *iplist;

	/* Extract the original packet and the original broadcast subnet from
			the userdata. */

	memcpy( (char *)&orig_broadcast_subnet, userdata->data, sizeof(struct subnet_record *) );
	memcpy( (char *)&original_packet, &userdata->data[sizeof(struct subnet_record *)],
			sizeof(struct packet_struct *) );

	if (rrec) {
		nb_flags = get_nb_flags( rrec->rdata );
		num_ips = rrec->rdlength / 6;
	} else {
		nb_flags = 0;
		num_ips = 0;
	}

	if(num_ips == 0) {
		DEBUG(0,("wins_proxy_name_query_request_success: Invalid number of IP records (0) \
returned for name %s.\n", nmb_namestr(nmbname) ));
		return;
	}

	if(num_ips == 1) {
		iplist = &ip;
	} else {
		if((iplist = SMB_MALLOC_ARRAY( struct in_addr, num_ips )) == NULL) {
			DEBUG(0,("wins_proxy_name_query_request_success: malloc fail !\n"));
			return;
		}

		for(i = 0; i < num_ips; i++) {
			putip( (char *)&iplist[i], (char *)&rrec->rdata[ (i*6) + 2]);
		}
	}

	/* Add the queried name to the original subnet as a WINS_PROXY_NAME. */

	if(rrec->ttl == PERMANENT_TTL) {
		ttl = lp_max_ttl();
	}

	pull_ascii_nstring(name, sizeof(name), nmbname->name);
	add_name_to_subnet( orig_broadcast_subnet, name,
					nmbname->name_type, nb_flags, ttl,
					WINS_PROXY_NAME, num_ips, iplist );

	if(iplist != &ip) {
		SAFE_FREE(iplist);
	}

	namerec = find_name_on_subnet(orig_broadcast_subnet, nmbname, FIND_ANY_NAME);
	if (!namerec) {
		DEBUG(0,("wins_proxy_name_query_request_success: failed to add "
			"name %s to subnet %s !\n",
			name,
			orig_broadcast_subnet->subnet_name ));
		return;
	}

	/*
	 * Check that none of the IP addresses we are returning is on the
	 * same broadcast subnet as the original requesting packet. If it
	 * is then don't reply (although we still need to add the name
	 * to the cache) as the actual machine will be replying also
	 * and we don't want two replies to a broadcast query.
	 */

	if(namerec && original_packet->packet.nmb.header.nm_flags.bcast) {
		for( i = 0; i < namerec->data.num_ips; i++) {
			if( same_net( namerec->data.ip[i], orig_broadcast_subnet->myip,
					orig_broadcast_subnet->mask_ip ) ) {
				DEBUG( 5, ( "wins_proxy_name_query_request_success: name %s is a WINS \
proxy name and is also on the same subnet (%s) as the requestor. \
Not replying.\n", nmb_namestr(&namerec->name), orig_broadcast_subnet->subnet_name ) );
				return;
			}
		}
	}

	/* Finally reply to the original name query. */
	reply_netbios_packet(original_packet,                /* Packet to reply to. */
				0,                              /* Result code. */
				NMB_QUERY,                      /* nmbd type code. */
				NMB_NAME_QUERY_OPCODE,          /* opcode. */
				ttl,                            /* ttl. */
				rrec->rdata,                    /* data to send. */
				rrec->rdlength);                /* data length. */
}

/****************************************************************************
Function called when the name lookup failed.
****************************************************************************/

static void wins_proxy_name_query_request_fail(struct subnet_record *subrec,
                                    struct response_record *rrec,
                                    struct nmb_name *question_name, int fail_code)
{
	DEBUG(4,("wins_proxy_name_query_request_fail: WINS server returned error code %d for lookup \
of name %s.\n", fail_code, nmb_namestr(question_name) ));
}

/****************************************************************************
Function to make a deep copy of the userdata we will need when the WINS
proxy query returns.
****************************************************************************/

static struct userdata_struct *wins_proxy_userdata_copy_fn(struct userdata_struct *userdata)
{
	struct packet_struct *p, *copy_of_p;
	struct userdata_struct *new_userdata = (struct userdata_struct *)SMB_MALLOC( userdata->userdata_len );

	if(new_userdata == NULL)
		return NULL;

	new_userdata->copy_fn = userdata->copy_fn;
	new_userdata->free_fn = userdata->free_fn;
	new_userdata->userdata_len = userdata->userdata_len;

	/* Copy the subnet_record pointer. */
	memcpy( new_userdata->data, userdata->data, sizeof(struct subnet_record *) );

	/* Extract the pointer to the packet struct */
	memcpy((char *)&p, &userdata->data[sizeof(struct subnet_record *)], sizeof(struct packet_struct *) );

	/* Do a deep copy of the packet. */
	if((copy_of_p = copy_packet(p)) == NULL) {
		SAFE_FREE(new_userdata);
		return NULL;
	}

	/* Lock the copy. */
	copy_of_p->locked = True;

	memcpy( &new_userdata->data[sizeof(struct subnet_record *)], (char *)&copy_of_p,
		sizeof(struct packet_struct *) );

	return new_userdata;
}

/****************************************************************************
Function to free the deep copy of the userdata we used when the WINS
proxy query returned.
****************************************************************************/

static void wins_proxy_userdata_free_fn(struct userdata_struct *userdata)
{
	struct packet_struct *p;

	/* Extract the pointer to the packet struct */
	memcpy((char *)&p, &userdata->data[sizeof(struct subnet_record *)],
		sizeof(struct packet_struct *));

	/* Unlock the packet. */
	p->locked = False;

	free_packet(p);
	ZERO_STRUCTP(userdata);
	SAFE_FREE(userdata);
}

/****************************************************************************
 Make a WINS query on behalf of a broadcast client name query request.
****************************************************************************/

void make_wins_proxy_name_query_request( struct subnet_record *subrec, 
                                         struct packet_struct *incoming_packet,
                                         struct nmb_name *question_name)
{
	union {
	    struct userdata_struct ud;
	    char c[sizeof(struct userdata_struct) + sizeof(struct subrec *) + 
		sizeof(struct packet_struct *)+sizeof(long*)];
	} ud;
	struct userdata_struct *userdata = &ud.ud;
	unstring qname;

	memset(&ud, '\0', sizeof(ud));
 
	userdata->copy_fn = wins_proxy_userdata_copy_fn;
	userdata->free_fn = wins_proxy_userdata_free_fn;
	userdata->userdata_len = sizeof(ud);
	memcpy( userdata->data, (char *)&subrec, sizeof(struct subnet_record *));
	memcpy( &userdata->data[sizeof(struct subnet_record *)], (char *)&incoming_packet,
			sizeof(struct packet_struct *));

	/* Now use the unicast subnet to query the name with the WINS server. */
	pull_ascii_nstring(qname, sizeof(qname), question_name->name);
	query_name( unicast_subnet, qname, question_name->name_type,
		wins_proxy_name_query_request_success,
		wins_proxy_name_query_request_fail,
		userdata);
}
