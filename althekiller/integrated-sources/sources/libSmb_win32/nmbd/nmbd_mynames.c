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

extern uint16 samba_nb_type; /* Samba's NetBIOS type. */

/****************************************************************************
 Fail funtion when registering my netbios names.
**************************************************************************/

static void my_name_register_failed(struct subnet_record *subrec,
                              struct response_record *rrec, struct nmb_name *nmbname)
{
	DEBUG(0,("my_name_register_failed: Failed to register my name %s on subnet %s.\n",
		nmb_namestr(nmbname), subrec->subnet_name));
}


/****************************************************************************
  Add my workgroup and my given names to one subnet
  Also add the magic Samba names.
**************************************************************************/

void register_my_workgroup_one_subnet(struct subnet_record *subrec)
{
	int i;

	struct work_record *work;

	/* Create the workgroup on the subnet. */
	if((work = create_workgroup_on_subnet(subrec, lp_workgroup(), 
					      PERMANENT_TTL)) == NULL) {
		DEBUG(0,("register_my_workgroup_and_names: Failed to create my workgroup %s on subnet %s. \
Exiting.\n", lp_workgroup(), subrec->subnet_name));
		return;
	}

	/* Each subnet entry, except for the wins_server_subnet has
           the magic Samba names. */
	add_samba_names_to_subnet(subrec);

	/* Register all our names including aliases. */
	for (i=0; my_netbios_names(i); i++) {
		register_name(subrec, my_netbios_names(i),0x20,samba_nb_type,
			      NULL,
			      my_name_register_failed, NULL);
		register_name(subrec, my_netbios_names(i),0x03,samba_nb_type,
			      NULL,
			      my_name_register_failed, NULL);
		register_name(subrec, my_netbios_names(i),0x00,samba_nb_type,
			      NULL,
			      my_name_register_failed, NULL);
	}
	
	/* Initiate election processing, register the workgroup names etc. */
	initiate_myworkgroup_startup(subrec, work);
}

/*******************************************************************
 Utility function to add a name to the unicast subnet, or add in
 our IP address if it already exists.
******************************************************************/

static void insert_refresh_name_into_unicast( struct subnet_record *subrec,
                                                struct nmb_name *nmbname, uint16 nb_type )
{
	struct name_record *namerec;

	if (!we_are_a_wins_client()) {
		insert_permanent_name_into_unicast(subrec, nmbname, nb_type);
		return;
	}

	if((namerec = find_name_on_subnet(unicast_subnet, nmbname, FIND_SELF_NAME)) == NULL) {
		unstring name;
		pull_ascii_nstring(name, sizeof(name), nmbname->name);
		/* The name needs to be created on the unicast subnet. */
		(void)add_name_to_subnet( unicast_subnet, name,
				nmbname->name_type, nb_type,
				MIN(lp_max_ttl(), MAX_REFRESH_TIME), SELF_NAME, 1, &subrec->myip);
	} else {
		/* The name already exists on the unicast subnet. Add our local
			IP for the given broadcast subnet to the name. */
		add_ip_to_name_record( namerec, subrec->myip);
	}
}

/****************************************************************************
  Add my workgroup and my given names to the subnet lists.
  Also add the magic Samba names.
**************************************************************************/

BOOL register_my_workgroup_and_names(void)
{
	struct subnet_record *subrec;
	int i;

	for(subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_INCLUDING_UNICAST(subrec)) {
		register_my_workgroup_one_subnet(subrec);
	}

	/* We still need to add the magic Samba
		names and the netbios names to the unicast subnet directly. This is
		to allow unicast node status requests and queries to still work
		in a broadcast only environment. */

	add_samba_names_to_subnet(unicast_subnet);

	for (i=0; my_netbios_names(i); i++) {
		for(subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
			/*
			 * Ensure all the IP addresses are added if we are multihomed.
			 */
			struct nmb_name nmbname;

			make_nmb_name(&nmbname, my_netbios_names(i),0x20);
			insert_refresh_name_into_unicast(subrec, &nmbname, samba_nb_type);

			make_nmb_name(&nmbname, my_netbios_names(i),0x3);
			insert_refresh_name_into_unicast(subrec, &nmbname, samba_nb_type);

			make_nmb_name(&nmbname, my_netbios_names(i),0x0);
			insert_refresh_name_into_unicast(subrec, &nmbname, samba_nb_type);
		}
	}

	/*
	 * Add the WORKGROUP<0> and WORKGROUP<1e> group names to the unicast subnet
	 * also for the same reasons.
	 */

	for(subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
		/*
		 * Ensure all the IP addresses are added if we are multihomed.
		 */
		struct nmb_name nmbname;

		make_nmb_name(&nmbname, lp_workgroup(), 0x0);
		insert_refresh_name_into_unicast(subrec, &nmbname, samba_nb_type|NB_GROUP);

		make_nmb_name(&nmbname, lp_workgroup(), 0x1e);
		insert_refresh_name_into_unicast(subrec, &nmbname, samba_nb_type|NB_GROUP);
	}

	/*
	 * We need to add the Samba names to the remote broadcast subnet,
	 * as NT 4.x does directed broadcast requests to the *<0x0> name.
	 */

	add_samba_names_to_subnet(remote_broadcast_subnet);

	return True;
}

/****************************************************************************
  Remove all the names we registered.
**************************************************************************/

void release_wins_names(void)
{
	struct subnet_record *subrec = unicast_subnet;
	struct name_record *namerec, *nextnamerec;

	for (namerec = subrec->namelist; namerec; namerec = nextnamerec) {
		nextnamerec = namerec->next;
		if( (namerec->data.source == SELF_NAME)
		    && !NAME_IS_DEREGISTERING(namerec) )
			release_name( subrec, namerec, standard_success_release,
				      NULL, NULL);
	}
}

/*******************************************************************
  Refresh our registered names with WINS
******************************************************************/

void refresh_my_names(time_t t)
{
	struct name_record *namerec;

	if (wins_srv_count() < 1)
		return;

	for (namerec = unicast_subnet->namelist; namerec; namerec = namerec->next) {
		/* Each SELF name has an individual time to be refreshed. */
		if ((namerec->data.source == SELF_NAME) &&
		    (namerec->data.refresh_time < t) &&
		    (namerec->data.death_time != PERMANENT_TTL)) {
			/* We cheat here and pretend the refresh is going to be
			   successful & update the refresh times. This stops
			   multiple refresh calls being done. We actually
			   deal with refresh failure in the fail_fn.
			*/
			if (!is_refresh_already_queued(unicast_subnet, namerec)) {
				wins_refresh_name(namerec);
			}
			namerec->data.death_time = t + lp_max_ttl();
			namerec->data.refresh_time = t + MIN(lp_max_ttl()/2, MAX_REFRESH_TIME);
		}
	}
}
