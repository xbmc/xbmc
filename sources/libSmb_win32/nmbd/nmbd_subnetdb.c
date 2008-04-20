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
   
   Revision History:

*/

#include "includes.h"

extern struct in_addr loopback_ip;
extern int global_nmb_port;

/* This is the broadcast subnets database. */
struct subnet_record *subnetlist = NULL;

/* Extra subnets - keep these separate so enumeration code doesn't
   run onto it by mistake. */

struct subnet_record *unicast_subnet = NULL;
struct subnet_record *remote_broadcast_subnet = NULL;
struct subnet_record *wins_server_subnet = NULL;

extern uint16 samba_nb_type; /* Samba's NetBIOS name type. */

/****************************************************************************
  Add a subnet into the list.
  **************************************************************************/

static void add_subnet(struct subnet_record *subrec)
{
	DLIST_ADD(subnetlist, subrec);
}

/****************************************************************************
stop listening on a subnet
we don't free the record as we don't have proper reference counting for it
yet and it may be in use by a response record
  ****************************************************************************/

void close_subnet(struct subnet_record *subrec)
{
	if (subrec->dgram_sock != -1) {
		close(subrec->dgram_sock);
		subrec->dgram_sock = -1;
	}
	if (subrec->nmb_sock != -1) {
		close(subrec->nmb_sock);
		subrec->nmb_sock = -1;
	}

	DLIST_REMOVE(subnetlist, subrec);
}

/****************************************************************************
  Create a subnet entry.
  ****************************************************************************/

static struct subnet_record *make_subnet(const char *name, enum subnet_type type,
					 struct in_addr myip, struct in_addr bcast_ip, 
					 struct in_addr mask_ip)
{
	struct subnet_record *subrec = NULL;
	int nmb_sock, dgram_sock;

	/* Check if we are creating a non broadcast subnet - if so don't create
		sockets.  */

	if(type != NORMAL_SUBNET) {
		nmb_sock = -1;
		dgram_sock = -1;
	} else {
		/*
		 * Attempt to open the sockets on port 137/138 for this interface
		 * and bind them.
		 * Fail the subnet creation if this fails.
		 */

		if((nmb_sock = open_socket_in(SOCK_DGRAM, global_nmb_port,0, myip.s_addr,True)) == -1) {
			if( DEBUGLVL( 0 ) ) {
				Debug1( "nmbd_subnetdb:make_subnet()\n" );
				Debug1( "  Failed to open nmb socket on interface %s ", inet_ntoa(myip) );
				Debug1( "for port %d.  ", global_nmb_port );
				Debug1( "Error was %s\n", strerror(errno) );
			}
			return NULL;
		}

		if((dgram_sock = open_socket_in(SOCK_DGRAM,DGRAM_PORT,3, myip.s_addr,True)) == -1) {
			if( DEBUGLVL( 0 ) ) {
				Debug1( "nmbd_subnetdb:make_subnet()\n" );
				Debug1( "  Failed to open dgram socket on interface %s ", inet_ntoa(myip) );
				Debug1( "for port %d.  ", DGRAM_PORT );
				Debug1( "Error was %s\n", strerror(errno) );
			}
			return NULL;
		}

		/* Make sure we can broadcast from these sockets. */
		set_socket_options(nmb_sock,"SO_BROADCAST");
		set_socket_options(dgram_sock,"SO_BROADCAST");

		/* Set them non-blocking. */
		set_blocking(nmb_sock, False);
		set_blocking(dgram_sock, False);
	}

	subrec = SMB_MALLOC_P(struct subnet_record);
	if (!subrec) {
		DEBUG(0,("make_subnet: malloc fail !\n"));
		close(nmb_sock);
		close(dgram_sock);
		return(NULL);
	}
  
	ZERO_STRUCTP(subrec);

	if((subrec->subnet_name = SMB_STRDUP(name)) == NULL) {
		DEBUG(0,("make_subnet: malloc fail for subnet name !\n"));
		close(nmb_sock);
		close(dgram_sock);
		ZERO_STRUCTP(subrec);
		SAFE_FREE(subrec);
		return(NULL);
	}

	DEBUG(2, ("making subnet name:%s ", name ));
	DEBUG(2, ("Broadcast address:%s ", inet_ntoa(bcast_ip)));
	DEBUG(2, ("Subnet mask:%s\n", inet_ntoa(mask_ip)));
 
	subrec->namelist_changed = False;
	subrec->work_changed = False;
 
	subrec->bcast_ip = bcast_ip;
	subrec->mask_ip  = mask_ip;
	subrec->myip = myip;
	subrec->type = type;
	subrec->nmb_sock = nmb_sock;
	subrec->dgram_sock = dgram_sock;
  
	return subrec;
}

/****************************************************************************
  Create a normal subnet
**************************************************************************/

struct subnet_record *make_normal_subnet(struct interface *iface)
{
	struct subnet_record *subrec;

	subrec = make_subnet(inet_ntoa(iface->ip), NORMAL_SUBNET,
			     iface->ip, iface->bcast, iface->nmask);
	if (subrec) {
		add_subnet(subrec);
	}
	return subrec;
}

/****************************************************************************
  Create subnet entries.
**************************************************************************/

BOOL create_subnets(void)
{    
	int num_interfaces = iface_count();
	int i;
	struct in_addr unicast_ip, ipzero;

	if(num_interfaces == 0) {
		DEBUG(0,("create_subnets: No local interfaces !\n"));
		DEBUG(0,("create_subnets: Waiting for an interface to appear ...\n"));
		while (iface_count() == 0) {
			sleep(5);
			load_interfaces();
		}
	}

	num_interfaces = iface_count();

	/* 
	 * Create subnets from all the local interfaces and thread them onto
	 * the linked list. 
	 */

	for (i = 0 ; i < num_interfaces; i++) {
		struct interface *iface = get_interface(i);

		if (!iface) {
			DEBUG(2,("create_subnets: can't get interface %d.\n", i ));
			continue;
		}

		/*
		 * We don't want to add a loopback interface, in case
		 * someone has added 127.0.0.1 for smbd, nmbd needs to
		 * ignore it here. JRA.
		 */

		if (ip_equal(iface->ip, loopback_ip)) {
			DEBUG(2,("create_subnets: Ignoring loopback interface.\n" ));
			continue;
		}

		if (!make_normal_subnet(iface))
			return False;
	}

	if (lp_we_are_a_wins_server()) {
		/* Pick the first interface ip address as the WINS server ip. */
		struct in_addr *nip = iface_n_ip(0);

		if (!nip) {
			return False;
		}

		unicast_ip = *nip;
	} else {
		/* note that we do not set the wins server IP here. We just
			set it at zero and let the wins registration code cope
			with getting the IPs right for each packet */
		zero_ip(&unicast_ip);
	}

	/*
	 * Create the unicast and remote broadcast subnets.
	 * Don't put these onto the linked list.
	 * The ip address of the unicast subnet is set to be
	 * the WINS server address, if it exists, or ipzero if not.
	 */

	unicast_subnet = make_subnet( "UNICAST_SUBNET", UNICAST_SUBNET, 
				unicast_ip, unicast_ip, unicast_ip);

	zero_ip(&ipzero);

	remote_broadcast_subnet = make_subnet( "REMOTE_BROADCAST_SUBNET",
				REMOTE_BROADCAST_SUBNET,
				ipzero, ipzero, ipzero);

	if((unicast_subnet == NULL) || (remote_broadcast_subnet == NULL))
		return False;

	/* 
	 * If we are WINS server, create the WINS_SERVER_SUBNET - don't put on
	 * the linked list.
	 */

	if (lp_we_are_a_wins_server()) {
		if( (wins_server_subnet = make_subnet( "WINS_SERVER_SUBNET",
						WINS_SERVER_SUBNET, 
						ipzero, ipzero, ipzero )) == NULL )
			return False;
	}

	return True;
}

/*******************************************************************
Function to tell us if we can use the unicast subnet.
******************************************************************/

BOOL we_are_a_wins_client(void)
{
	if (wins_srv_count() > 0) {
		return True;
	}

	return False;
}

/*******************************************************************
Access function used by NEXT_SUBNET_INCLUDING_UNICAST
******************************************************************/

struct subnet_record *get_next_subnet_maybe_unicast(struct subnet_record *subrec)
{
	if(subrec == unicast_subnet)
		return NULL;
	else if((subrec->next == NULL) && we_are_a_wins_client())
		return unicast_subnet;
	else
		return subrec->next;
}

/*******************************************************************
 Access function used by retransmit_or_expire_response_records() in
 nmbd_packets.c. Patch from Andrey Alekseyev <fetch@muffin.arcadia.spb.ru>
 Needed when we need to enumerate all the broadcast, unicast and
 WINS subnets.
******************************************************************/

struct subnet_record *get_next_subnet_maybe_unicast_or_wins_server(struct subnet_record *subrec)
{
	if(subrec == unicast_subnet) {
		if(wins_server_subnet)
			return wins_server_subnet;
		else
			return NULL;
	}

	if(wins_server_subnet && subrec == wins_server_subnet)
		return NULL;

	if((subrec->next == NULL) && we_are_a_wins_client())
		return unicast_subnet;
	else
		return subrec->next;
}
