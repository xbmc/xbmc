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

uint16 samba_nb_type = 0; /* samba's NetBIOS name type */


/**************************************************************************
 Set Samba's NetBIOS name type.
***************************************************************************/

void set_samba_nb_type(void)
{
	if( lp_wins_support() || wins_srv_count() ) {
		samba_nb_type = NB_HFLAG;               /* samba is a 'hybrid' node type. */
	} else {
		samba_nb_type = NB_BFLAG;           /* samba is broadcast-only node type. */
	}
}

/***************************************************************************
 Convert a NetBIOS name to upper case.
***************************************************************************/

static void upcase_name( struct nmb_name *target, const struct nmb_name *source )
{
	int i;
	unstring targ;
	fstring scope;

	if( NULL != source ) {
		memcpy( target, source, sizeof( struct nmb_name ) );
	}

	pull_ascii_nstring(targ, sizeof(targ), target->name);
	strupper_m( targ );
	push_ascii_nstring( target->name, targ);

	pull_ascii(scope, target->scope, 64, -1, STR_TERMINATE);
	strupper_m( scope );
	push_ascii(target->scope, scope, 64, STR_TERMINATE);

	/* fudge... We're using a byte-by-byte compare, so we must be sure that
	 * unused space doesn't have garbage in it.
	 */

	for( i = strlen( target->name ); i < sizeof( target->name ); i++ ) {
		target->name[i] = '\0';
	}
	for( i = strlen( target->scope ); i < sizeof( target->scope ); i++ ) {
		target->scope[i] = '\0';
	}
}

/**************************************************************************
 Remove a name from the namelist.
***************************************************************************/

void remove_name_from_namelist(struct subnet_record *subrec, 
				struct name_record *namerec )
{
	if (subrec == wins_server_subnet) 
		remove_name_from_wins_namelist(namerec);
	else {
		subrec->namelist_changed = True;
		DLIST_REMOVE(subrec->namelist, namerec);
	}

	SAFE_FREE(namerec->data.ip);
	ZERO_STRUCTP(namerec);
	SAFE_FREE(namerec);
}

/**************************************************************************
 Find a name in a subnet.
**************************************************************************/

struct name_record *find_name_on_subnet(struct subnet_record *subrec,
				const struct nmb_name *nmbname,
				BOOL self_only)
{
	struct nmb_name uc_name;
	struct name_record *name_ret;

	upcase_name( &uc_name, nmbname );
	
	if (subrec == wins_server_subnet) {
		return find_name_on_wins_subnet(&uc_name, self_only);
	}

	for( name_ret = subrec->namelist; name_ret; name_ret = name_ret->next) {
		if (memcmp(&uc_name, &name_ret->name, sizeof(struct nmb_name)) == 0) {
			break;
		}
	}

	if( name_ret ) {
		/* Self names only - these include permanent names. */
		if( self_only && (name_ret->data.source != SELF_NAME) && (name_ret->data.source != PERMANENT_NAME) ) {
			DEBUG( 9, ( "find_name_on_subnet: on subnet %s - self name %s NOT FOUND\n",
						subrec->subnet_name, nmb_namestr(nmbname) ) );
			return NULL;
		}

		DEBUG( 9, ("find_name_on_subnet: on subnet %s - found name %s source=%d\n",
			subrec->subnet_name, nmb_namestr(nmbname), name_ret->data.source) );

		return name_ret;
	}

	DEBUG( 9, ( "find_name_on_subnet: on subnet %s - name %s NOT FOUND\n",
		subrec->subnet_name, nmb_namestr(nmbname) ) );

	return NULL;
}

/**************************************************************************
 Find a name over all known broadcast subnets.
************************************************************************/

struct name_record *find_name_for_remote_broadcast_subnet(struct nmb_name *nmbname,
						BOOL self_only)
{
	struct subnet_record *subrec;
	struct name_record *namerec;

	for( subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec) ) {
		namerec = find_name_on_subnet(subrec, nmbname, self_only);
		if (namerec) {
			return namerec;
		}
	}

	return NULL;
}
  
/**************************************************************************
 Update the ttl of an entry in a subnet name list.
***************************************************************************/

void update_name_ttl( struct name_record *namerec, int ttl )
{
	time_t time_now = time(NULL);

	if( namerec->data.death_time != PERMANENT_TTL) {
		namerec->data.death_time = time_now + ttl;
	}

	namerec->data.refresh_time = time_now + MIN((ttl/2), MAX_REFRESH_TIME);

	if (namerec->subnet == wins_server_subnet) {
		wins_store_changed_namerec(namerec);
	} else {
		namerec->subnet->namelist_changed = True;
	}
}

/**************************************************************************
 Add an entry to a subnet name list.
***********************************************************************/

BOOL add_name_to_subnet( struct subnet_record *subrec,
			const char *name,
			int type,
			uint16 nb_flags,
			int ttl,
			enum name_source source,
			int num_ips,
			struct in_addr *iplist)
{
	BOOL ret = False;
	struct name_record *namerec;
	time_t time_now = time(NULL);

	namerec = SMB_MALLOC_P(struct name_record);
	if( NULL == namerec ) {
		DEBUG( 0, ( "add_name_to_subnet: malloc fail.\n" ) );
		return False;
	}

	memset( (char *)namerec, '\0', sizeof(*namerec) );
	namerec->data.ip = SMB_MALLOC_ARRAY( struct in_addr, num_ips );
	if( NULL == namerec->data.ip ) {
		DEBUG( 0, ( "add_name_to_subnet: malloc fail when creating ip_flgs.\n" ) );
		ZERO_STRUCTP(namerec);
		SAFE_FREE(namerec);
		return False;
	}

	namerec->subnet = subrec;

	make_nmb_name(&namerec->name, name, type);
	upcase_name(&namerec->name, NULL );

	/* Enter the name as active. */
	namerec->data.nb_flags = nb_flags | NB_ACTIVE;
	namerec->data.wins_flags = WINS_ACTIVE;

	/* If it's our primary name, flag it as so. */
	if (strequal( my_netbios_names(0), name )) {
		namerec->data.nb_flags |= NB_PERM;
	}

	/* Copy the IPs. */
	namerec->data.num_ips = num_ips;
	memcpy( (namerec->data.ip), iplist, num_ips * sizeof(struct in_addr) );

	/* Data source. */
	namerec->data.source = source;

	/* Setup the death_time and refresh_time. */
	if (ttl == PERMANENT_TTL) {
		namerec->data.death_time = PERMANENT_TTL;
	} else {
		namerec->data.death_time = time_now + ttl;
	}

	namerec->data.refresh_time = time_now + MIN((ttl/2), MAX_REFRESH_TIME);

	DEBUG( 3, ( "add_name_to_subnet: Added netbios name %s with first IP %s \
ttl=%d nb_flags=%2x to subnet %s\n",
		nmb_namestr( &namerec->name ),
		inet_ntoa( *iplist ),
		ttl,
		(unsigned int)nb_flags,
		subrec->subnet_name ) );

	/* Now add the record to the name list. */    

	if (subrec == wins_server_subnet) {
		ret = add_name_to_wins_subnet(namerec);
		/* Free namerec - it's stored in the tdb. */
		SAFE_FREE(namerec->data.ip);
		SAFE_FREE(namerec);
	} else {
		DLIST_ADD(subrec->namelist, namerec);
		subrec->namelist_changed = True;
		ret = True;
	}

	return ret;
}

/*******************************************************************
 Utility function automatically called when a name refresh or register 
 succeeds. By definition this is a SELF_NAME (or we wouldn't be registering
 it).
 ******************************************************************/

void standard_success_register(struct subnet_record *subrec, 
                             struct userdata_struct *userdata,
                             struct nmb_name *nmbname, uint16 nb_flags, int ttl,
                             struct in_addr registered_ip)
{
	struct name_record *namerec;

	namerec = find_name_on_subnet( subrec, nmbname, FIND_SELF_NAME);
	if (namerec == NULL) {
		unstring name;
		pull_ascii_nstring(name, sizeof(name), nmbname->name);
		add_name_to_subnet( subrec, name, nmbname->name_type,
			nb_flags, ttl, SELF_NAME, 1, &registered_ip );
	} else {
		update_name_ttl( namerec, ttl );
	}
}

/*******************************************************************
 Utility function automatically called when a name refresh or register 
 fails. Note that this is only ever called on a broadcast subnet with
 one IP address per name. This is why it can just delete the name 
 without enumerating the IP adresses. JRA.
 ******************************************************************/

void standard_fail_register( struct subnet_record   *subrec,
                             struct response_record *rrec,
                             struct nmb_name        *nmbname )
{
	struct name_record *namerec;

	namerec = find_name_on_subnet( subrec, nmbname, FIND_SELF_NAME);

	DEBUG( 0, ( "standard_fail_register: Failed to register/refresh name %s \
on subnet %s\n", nmb_namestr(nmbname), subrec->subnet_name) );

	/* Remove the name from the subnet. */
	if( namerec ) {
		remove_name_from_namelist(subrec, namerec);
	}
}

/*******************************************************************
 Utility function to remove an IP address from a name record.
 ******************************************************************/

static void remove_nth_ip_in_record( struct name_record *namerec, int ind)
{
	if( ind != namerec->data.num_ips ) {
		memmove( (char *)(&namerec->data.ip[ind]),
				(char *)(&namerec->data.ip[ind+1]), 
				( namerec->data.num_ips - ind - 1) * sizeof(struct in_addr) );
	}

	namerec->data.num_ips--;
	if (namerec->subnet == wins_server_subnet) {
		wins_store_changed_namerec(namerec);
	} else {
		namerec->subnet->namelist_changed = True;
	}
}

/*******************************************************************
 Utility function to check if an IP address exists in a name record.
 ******************************************************************/

BOOL find_ip_in_name_record( struct name_record *namerec, struct in_addr ip )
{
	int i;

	for(i = 0; i < namerec->data.num_ips; i++) {
		if(ip_equal( namerec->data.ip[i], ip)) {
			return True;
		}
	}

	return False;
}

/*******************************************************************
 Utility function to add an IP address to a name record.
 ******************************************************************/

void add_ip_to_name_record( struct name_record *namerec, struct in_addr new_ip )
{
	struct in_addr *new_list;

	/* Don't add one we already have. */
	if( find_ip_in_name_record( namerec, new_ip )) {
		return;
	}
  
	new_list = SMB_MALLOC_ARRAY( struct in_addr, namerec->data.num_ips + 1);
	if( NULL == new_list ) {
		DEBUG(0,("add_ip_to_name_record: Malloc fail !\n"));
		return;
	}

	memcpy( (char *)new_list, (char *)namerec->data.ip, namerec->data.num_ips * sizeof(struct in_addr) );
	new_list[namerec->data.num_ips] = new_ip;

	SAFE_FREE(namerec->data.ip);
	namerec->data.ip = new_list;
	namerec->data.num_ips += 1;

	if (namerec->subnet == wins_server_subnet) {
		wins_store_changed_namerec(namerec);
	} else {
		namerec->subnet->namelist_changed = True;
	}
}

/*******************************************************************
 Utility function to remove an IP address from a name record.
 ******************************************************************/

void remove_ip_from_name_record( struct name_record *namerec,
                                 struct in_addr      remove_ip )
{
	/* Try and find the requested ip address - remove it. */
	int i;
	int orig_num = namerec->data.num_ips;

	for(i = 0; i < orig_num; i++) {
		if( ip_equal( remove_ip, namerec->data.ip[i]) ) {
			remove_nth_ip_in_record( namerec, i);
			break;
		}
	}
}

/*******************************************************************
 Utility function that release_name callers can plug into as the
 success function when a name release is successful. Used to save
 duplication of success_function code.
 ******************************************************************/

void standard_success_release( struct subnet_record   *subrec,
                               struct userdata_struct *userdata,
                               struct nmb_name        *nmbname,
                               struct in_addr          released_ip )
{
	struct name_record *namerec;

	namerec = find_name_on_subnet( subrec, nmbname, FIND_ANY_NAME );
	if( namerec == NULL ) {
		DEBUG( 0, ( "standard_success_release: Name release for name %s IP %s \
on subnet %s. Name was not found on subnet.\n", nmb_namestr(nmbname), inet_ntoa(released_ip),
				subrec->subnet_name) );
		return;
	} else {
		int orig_num = namerec->data.num_ips;

		remove_ip_from_name_record( namerec, released_ip );

		if( namerec->data.num_ips == orig_num ) {
			DEBUG( 0, ( "standard_success_release: Name release for name %s IP %s \
on subnet %s. This ip is not known for this name.\n", nmb_namestr(nmbname), inet_ntoa(released_ip), subrec->subnet_name ) );
		}
	}

	if( namerec->data.num_ips == 0 ) {
		remove_name_from_namelist( subrec, namerec );
	}
}

/*******************************************************************
 Expires old names in a subnet namelist.
 NB. Does not touch the wins_subnet - no wins specific processing here.
******************************************************************/

static void expire_names_on_subnet(struct subnet_record *subrec, time_t t)
{
	struct name_record *namerec;
	struct name_record *next_namerec;

	for( namerec = subrec->namelist; namerec; namerec = next_namerec ) {
		next_namerec = namerec->next;
		if( (namerec->data.death_time != PERMANENT_TTL) && (namerec->data.death_time < t) ) {
			if( namerec->data.source == SELF_NAME ) {
				DEBUG( 3, ( "expire_names_on_subnet: Subnet %s not expiring SELF \
name %s\n", subrec->subnet_name, nmb_namestr(&namerec->name) ) );
				namerec->data.death_time += 300;
				namerec->subnet->namelist_changed = True;
				continue;
			}

			DEBUG(3,("expire_names_on_subnet: Subnet %s - removing expired name %s\n",
				subrec->subnet_name, nmb_namestr(&namerec->name)));
  
			remove_name_from_namelist(subrec, namerec );
		}
	}
}

/*******************************************************************
 Expires old names in all subnet namelists.
 NB. Does not touch the wins_subnet.
******************************************************************/

void expire_names(time_t t)
{
	struct subnet_record *subrec;

	for( subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_INCLUDING_UNICAST(subrec) ) {
		expire_names_on_subnet( subrec, t );
	}
}

/****************************************************************************
  Add the magic samba names, useful for finding samba servers.
  These go directly into the name list for a particular subnet,
  without going through the normal registration process.
  When adding them to the unicast subnet, add them as a list of
  all broadcast subnet IP addresses.
**************************************************************************/

void add_samba_names_to_subnet( struct subnet_record *subrec )
{
	struct in_addr *iplist = &subrec->myip;
	int num_ips = 1;

	/* These names are added permanently (ttl of zero) and will NOT be refreshed.  */

	if( (subrec == unicast_subnet) || (subrec == wins_server_subnet) || (subrec == remote_broadcast_subnet) ) {
		struct subnet_record *bcast_subrecs;
		int i;

		/* Create an IP list containing all our known subnets. */

		num_ips = iface_count();
		iplist = SMB_MALLOC_ARRAY( struct in_addr, num_ips);
		if( NULL == iplist ) {
			DEBUG(0,("add_samba_names_to_subnet: Malloc fail !\n"));
			return;
		}

		for( bcast_subrecs = FIRST_SUBNET, i = 0; bcast_subrecs; bcast_subrecs = NEXT_SUBNET_EXCLUDING_UNICAST(bcast_subrecs), i++ )
			iplist[i] = bcast_subrecs->myip;
	}

	add_name_to_subnet(subrec,"*",0x0,samba_nb_type, PERMANENT_TTL,
				PERMANENT_NAME, num_ips, iplist);
	add_name_to_subnet(subrec,"*",0x20,samba_nb_type,PERMANENT_TTL,
				PERMANENT_NAME, num_ips, iplist);
	add_name_to_subnet(subrec,"__SAMBA__",0x20,samba_nb_type,PERMANENT_TTL,
				PERMANENT_NAME, num_ips, iplist);
	add_name_to_subnet(subrec,"__SAMBA__",0x00,samba_nb_type,PERMANENT_TTL,
				PERMANENT_NAME, num_ips, iplist);

	if(iplist != &subrec->myip) {
		SAFE_FREE(iplist);
	}
}

/****************************************************************************
 Dump a name_record struct.
**************************************************************************/

void dump_name_record( struct name_record *namerec, XFILE *fp)
{
	const char *src_type;
	struct tm *tm;
	int i;

	x_fprintf(fp,"\tName = %s\t", nmb_namestr(&namerec->name));
	switch(namerec->data.source) {
		case LMHOSTS_NAME:
			src_type = "LMHOSTS_NAME";
			break;
		case WINS_PROXY_NAME:
			src_type = "WINS_PROXY_NAME";
			break;
		case REGISTER_NAME:
			src_type = "REGISTER_NAME";
			break;
		case SELF_NAME:
			src_type = "SELF_NAME";
			break;
		case DNS_NAME:
			src_type = "DNS_NAME";
			break;
		case DNSFAIL_NAME:
			src_type = "DNSFAIL_NAME";
			break;
		case PERMANENT_NAME:
			src_type = "PERMANENT_NAME";
			break;
		default:
			src_type = "unknown!";
			break;
	}

	x_fprintf(fp,"Source = %s\nb_flags = %x\t", src_type, namerec->data.nb_flags);

	if(namerec->data.death_time != PERMANENT_TTL) {
		const char *asct;
		tm = localtime(&namerec->data.death_time);
		if (!tm) {
			return;
		}
		asct = asctime(tm);
		if (!asct) {
			return;
		}
		x_fprintf(fp, "death_time = %s\t", asct);
	} else {
		x_fprintf(fp, "death_time = PERMANENT\t");
	}

	if(namerec->data.refresh_time != PERMANENT_TTL) {
		const char *asct;
		tm = localtime(&namerec->data.refresh_time);
		if (!tm) {
			return;
		}
		asct = asctime(tm);
		if (!asct) {
			return;
		}
		x_fprintf(fp, "refresh_time = %s\n", asct);
	} else {
		x_fprintf(fp, "refresh_time = PERMANENT\n");
	}

	x_fprintf(fp, "\t\tnumber of IPS = %d", namerec->data.num_ips);
	for(i = 0; i < namerec->data.num_ips; i++) {
		x_fprintf(fp, "\t%s", inet_ntoa(namerec->data.ip[i]));
	}

	x_fprintf(fp, "\n\n");
	
}

/****************************************************************************
 Dump the contents of the namelists on all the subnets (including unicast)
 into a file. Initiated by SIGHUP - used to debug the state of the namelists.
**************************************************************************/

static void dump_subnet_namelist( struct subnet_record *subrec, XFILE *fp)
{
	struct name_record *namerec;
	x_fprintf(fp, "Subnet %s\n----------------------\n", subrec->subnet_name);
	for( namerec = subrec->namelist; namerec; namerec = namerec->next) {
		dump_name_record(namerec, fp);
	}
}

/****************************************************************************
 Dump the contents of the namelists on all the subnets (including unicast)
 into a file. Initiated by SIGHUP - used to debug the state of the namelists.
**************************************************************************/

void dump_all_namelists(void)
{
	XFILE *fp; 
	struct subnet_record *subrec;

	fp = x_fopen(lock_path("namelist.debug"),O_WRONLY|O_CREAT|O_TRUNC, 0644);
     
	if (!fp) { 
		DEBUG(0,("dump_all_namelists: Can't open file %s. Error was %s\n",
			"namelist.debug",strerror(errno)));
		return;
	}
      
	for (subrec = FIRST_SUBNET; subrec; subrec = NEXT_SUBNET_INCLUDING_UNICAST(subrec)) {
		dump_subnet_namelist( subrec, fp );
	}

	if (!we_are_a_wins_client()) {
		dump_subnet_namelist( unicast_subnet, fp );
	}

	if (remote_broadcast_subnet->namelist != NULL) {
		dump_subnet_namelist( remote_broadcast_subnet, fp );
	}

	if (wins_server_subnet != NULL) {
		dump_wins_subnet_namelist(fp );
	}

	x_fclose( fp );
}
