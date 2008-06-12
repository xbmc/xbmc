/* 
   Unix SMB/CIFS implementation.
   name query routines
   Copyright (C) Andrew Tridgell 1994-1998
   
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

#ifdef _XBOX
#define close(a) closesocket(a)
#endif

/* nmbd.c sets this to True. */
BOOL global_in_nmbd = False;


/****************************
 * SERVER AFFINITY ROUTINES *
 ****************************/
 
 /* Server affinity is the concept of preferring the last domain 
    controller with whom you had a successful conversation */
 
/****************************************************************************
****************************************************************************/
#define SAFKEY_FMT	"SAF/DOMAIN/%s"
#define SAF_TTL		900

static char *saf_key(const char *domain)
{
	char *keystr;
	
	asprintf( &keystr, SAFKEY_FMT, strupper_static(domain) );

	return keystr;
}

/****************************************************************************
****************************************************************************/

BOOL saf_store( const char *domain, const char *servername )
{
	char *key;
	time_t expire;
	BOOL ret = False;
	
	if ( !domain || !servername ) {
		DEBUG(2,("saf_store: Refusing to store empty domain or servername!\n"));
		return False;
	}
	
	if ( !gencache_init() ) 
		return False;
	
	key = saf_key( domain );
	expire = time( NULL ) + SAF_TTL;
	
	
	DEBUG(10,("saf_store: domain = [%s], server = [%s], expire = [%u]\n",
		domain, servername, (unsigned int)expire ));
		
	ret = gencache_set( key, servername, expire );
	
	SAFE_FREE( key );
	
	return ret;
}

/****************************************************************************
****************************************************************************/

char *saf_fetch( const char *domain )
{
	char *server = NULL;
	time_t timeout;
	BOOL ret = False;
	char *key = NULL;

	if ( !domain ) {
		DEBUG(2,("saf_fetch: Empty domain name!\n"));
		return NULL;
	}
	
	if ( !gencache_init() ) 
		return False;
	
	key = saf_key( domain );
	
	ret = gencache_get( key, &server, &timeout );
	
	SAFE_FREE( key );
	
	if ( !ret ) {
		DEBUG(5,("saf_fetch: failed to find server for \"%s\" domain\n", domain ));
	} else {
		DEBUG(5,("saf_fetch: Returning \"%s\" for \"%s\" domain\n", 
			server, domain ));
	}
		
	return server;
}


/****************************************************************************
 Generate a random trn_id.
****************************************************************************/

static int generate_trn_id(void)
{
	static int trn_id;

	if (trn_id == 0) {
		sys_srandom(sys_getpid());
	}

	trn_id = sys_random();

	return trn_id % (unsigned)0x7FFF;
}

/****************************************************************************
 Parse a node status response into an array of structures.
****************************************************************************/

static NODE_STATUS_STRUCT *parse_node_status(char *p, int *num_names, struct node_status_extra *extra)
{
	NODE_STATUS_STRUCT *ret;
	int i;

	*num_names = CVAL(p,0);

	if (*num_names == 0)
		return NULL;

	ret = SMB_MALLOC_ARRAY(NODE_STATUS_STRUCT,*num_names);
	if (!ret)
		return NULL;

	p++;
	for (i=0;i< *num_names;i++) {
		StrnCpy(ret[i].name,p,15);
		trim_char(ret[i].name,'\0',' ');
		ret[i].type = CVAL(p,15);
		ret[i].flags = p[16];
		p += 18;
		DEBUG(10, ("%s#%02x: flags = 0x%02x\n", ret[i].name, 
			   ret[i].type, ret[i].flags));
	}
	/*
	 * Also, pick up the MAC address ...
	 */
	if (extra) {
		memcpy(&extra->mac_addr, p, 6); /* Fill in the mac addr */
	}
	return ret;
}


/****************************************************************************
 Do a NBT node status query on an open socket and return an array of
 structures holding the returned names or NULL if the query failed.
**************************************************************************/

NODE_STATUS_STRUCT *node_status_query(int fd,struct nmb_name *name,
				      struct in_addr to_ip, int *num_names,
				      struct node_status_extra *extra)
{
	BOOL found=False;
	int retries = 2;
	int retry_time = 2000;
	struct timeval tval;
	struct packet_struct p;
	struct packet_struct *p2;
	struct nmb_packet *nmb = &p.packet.nmb;
	NODE_STATUS_STRUCT *ret;

	ZERO_STRUCT(p);

	nmb->header.name_trn_id = generate_trn_id();
	nmb->header.opcode = 0;
	nmb->header.response = False;
	nmb->header.nm_flags.bcast = False;
	nmb->header.nm_flags.recursion_available = False;
	nmb->header.nm_flags.recursion_desired = False;
	nmb->header.nm_flags.trunc = False;
	nmb->header.nm_flags.authoritative = False;
	nmb->header.rcode = 0;
	nmb->header.qdcount = 1;
	nmb->header.ancount = 0;
	nmb->header.nscount = 0;
	nmb->header.arcount = 0;
	nmb->question.question_name = *name;
	nmb->question.question_type = 0x21;
	nmb->question.question_class = 0x1;

	p.ip = to_ip;
	p.port = NMB_PORT;
	p.fd = fd;
	p.timestamp = time(NULL);
	p.packet_type = NMB_PACKET;
	
	GetTimeOfDay(&tval);
  
	if (!send_packet(&p)) 
		return NULL;

	retries--;

	while (1) {
		struct timeval tval2;
		GetTimeOfDay(&tval2);
		if (TvalDiff(&tval,&tval2) > retry_time) {
			if (!retries)
				break;
			if (!found && !send_packet(&p))
				return NULL;
			GetTimeOfDay(&tval);
			retries--;
		}

		if ((p2=receive_nmb_packet(fd,90,nmb->header.name_trn_id))) {     
			struct nmb_packet *nmb2 = &p2->packet.nmb;
			debug_nmb_packet(p2);
			
			if (nmb2->header.opcode != 0 ||
			    nmb2->header.nm_flags.bcast ||
			    nmb2->header.rcode ||
			    !nmb2->header.ancount ||
			    nmb2->answers->rr_type != 0x21) {
				/* XXXX what do we do with this? could be a
				   redirect, but we'll discard it for the
				   moment */
				free_packet(p2);
				continue;
			}

			ret = parse_node_status(&nmb2->answers->rdata[0], num_names, extra);
			free_packet(p2);
			return ret;
		}
	}
	
	return NULL;
}

/****************************************************************************
 Find the first type XX name in a node status reply - used for finding
 a servers name given its IP. Return the matched name in *name.
**************************************************************************/

BOOL name_status_find(const char *q_name, int q_type, int type, struct in_addr to_ip, fstring name)
{
	NODE_STATUS_STRUCT *status = NULL;
	struct nmb_name nname;
	int count, i;
	int sock;
	BOOL result = False;

	if (lp_disable_netbios()) {
		DEBUG(5,("name_status_find(%s#%02x): netbios is disabled\n", q_name, q_type));
		return False;
	}

	DEBUG(10, ("name_status_find: looking up %s#%02x at %s\n", q_name, 
		   q_type, inet_ntoa(to_ip)));

	/* Check the cache first. */

	if (namecache_status_fetch(q_name, q_type, type, to_ip, name))
		return True;

	sock = open_socket_in(SOCK_DGRAM, 0, 3, interpret_addr(lp_socket_address()), True);
	if (sock == -1)
		goto done;

	/* W2K PDC's seem not to respond to '*'#0. JRA */
	make_nmb_name(&nname, q_name, q_type);
	status = node_status_query(sock, &nname, to_ip, &count, NULL);
	close(sock);
	if (!status)
		goto done;

	for (i=0;i<count;i++) {
		if (status[i].type == type)
			break;
	}
	if (i == count)
		goto done;

	pull_ascii_nstring(name, sizeof(fstring), status[i].name);

	/* Store the result in the cache. */
	/* but don't store an entry for 0x1c names here.  Here we have 
	   a single host and DOMAIN<0x1c> names should be a list of hosts */
	   
	if ( q_type != 0x1c )
		namecache_status_store(q_name, q_type, type, to_ip, name);

	result = True;

 done:
	SAFE_FREE(status);

	DEBUG(10, ("name_status_find: name %sfound", result ? "" : "not "));

	if (result)
		DEBUGADD(10, (", name %s ip address is %s", name, inet_ntoa(to_ip)));

	DEBUG(10, ("\n"));	

	return result;
}

/*
  comparison function used by sort_ip_list
*/

static int ip_compare(struct in_addr *ip1, struct in_addr *ip2)
{
	int max_bits1=0, max_bits2=0;
	int num_interfaces = iface_count();
	int i;

	for (i=0;i<num_interfaces;i++) {
		struct in_addr ip;
		int bits1, bits2;
		ip = *iface_n_bcast(i);
		bits1 = matching_quad_bits((uchar *)&ip1->s_addr, (uchar *)&ip.s_addr);
		bits2 = matching_quad_bits((uchar *)&ip2->s_addr, (uchar *)&ip.s_addr);
		max_bits1 = MAX(bits1, max_bits1);
		max_bits2 = MAX(bits2, max_bits2);
	}	
	
	/* bias towards directly reachable IPs */
	if (iface_local(*ip1)) {
		max_bits1 += 32;
	}
	if (iface_local(*ip2)) {
		max_bits2 += 32;
	}

	return max_bits2 - max_bits1;
}

/*******************************************************************
 compare 2 ldap IPs by nearness to our interfaces - used in qsort
*******************************************************************/

static int ip_service_compare(struct ip_service *ip1, struct ip_service *ip2)
{
	int result;
	
	if ( (result = ip_compare(&ip1->ip, &ip2->ip)) != 0 )
		return result;
		
	if ( ip1->port > ip2->port )
		return 1;
	
	if ( ip1->port < ip2->port )
		return -1;
		
	return 0;
}

/*
  sort an IP list so that names that are close to one of our interfaces 
  are at the top. This prevents the problem where a WINS server returns an IP that
  is not reachable from our subnet as the first match
*/

static void sort_ip_list(struct in_addr *iplist, int count)
{
	if (count <= 1) {
		return;
	}

	qsort(iplist, count, sizeof(struct in_addr), QSORT_CAST ip_compare);	
}

static void sort_ip_list2(struct ip_service *iplist, int count)
{
	if (count <= 1) {
		return;
	}

	qsort(iplist, count, sizeof(struct ip_service), QSORT_CAST ip_service_compare);	
}

/**********************************************************************
 Remove any duplicate address/port pairs in the list 
 *********************************************************************/

static int remove_duplicate_addrs2( struct ip_service *iplist, int count )
{
	int i, j;
	
	DEBUG(10,("remove_duplicate_addrs2: looking for duplicate address/port pairs\n"));
	
	/* one loop to remove duplicates */
	for ( i=0; i<count; i++ ) {
		if ( is_zero_ip(iplist[i].ip) )
			continue;
					
		for ( j=i+1; j<count; j++ ) {
			if ( ip_service_equal(iplist[i], iplist[j]) )
				zero_ip(&iplist[j].ip);
		}
	}
			
	/* one loop to clean up any holes we left */
	/* first ip should never be a zero_ip() */
	for (i = 0; i<count; ) {
		if ( is_zero_ip(iplist[i].ip) ) {
			if (i != count-1 )
				memmove(&iplist[i], &iplist[i+1], (count - i - 1)*sizeof(iplist[i]));
			count--;
			continue;
		}
		i++;
	}

	return count;
}

/****************************************************************************
 Do a netbios name query to find someones IP.
 Returns an array of IP addresses or NULL if none.
 *count will be set to the number of addresses returned.
 *timed_out is set if we failed by timing out
****************************************************************************/

struct in_addr *name_query(int fd,const char *name,int name_type, 
			   BOOL bcast,BOOL recurse,
			   struct in_addr to_ip, int *count, int *flags,
			   BOOL *timed_out)
{
	BOOL found=False;
	int i, retries = 3;
	int retry_time = bcast?250:2000;
	struct timeval tval;
	struct packet_struct p;
	struct packet_struct *p2;
	struct nmb_packet *nmb = &p.packet.nmb;
	struct in_addr *ip_list = NULL;

	if (lp_disable_netbios()) {
		DEBUG(5,("name_query(%s#%02x): netbios is disabled\n", name, name_type));
		return NULL;
	}

	if (timed_out) {
		*timed_out = False;
	}
	
	memset((char *)&p,'\0',sizeof(p));
	(*count) = 0;
	(*flags) = 0;
	
	nmb->header.name_trn_id = generate_trn_id();
	nmb->header.opcode = 0;
	nmb->header.response = False;
	nmb->header.nm_flags.bcast = bcast;
	nmb->header.nm_flags.recursion_available = False;
	nmb->header.nm_flags.recursion_desired = recurse;
	nmb->header.nm_flags.trunc = False;
	nmb->header.nm_flags.authoritative = False;
	nmb->header.rcode = 0;
	nmb->header.qdcount = 1;
	nmb->header.ancount = 0;
	nmb->header.nscount = 0;
	nmb->header.arcount = 0;
	
	make_nmb_name(&nmb->question.question_name,name,name_type);
	
	nmb->question.question_type = 0x20;
	nmb->question.question_class = 0x1;
	
	p.ip = to_ip;
	p.port = NMB_PORT;
	p.fd = fd;
	p.timestamp = time(NULL);
	p.packet_type = NMB_PACKET;
	
	GetTimeOfDay(&tval);
	
	if (!send_packet(&p)) 
		return NULL;
	
	retries--;
	
	while (1) {
		struct timeval tval2;
		
		GetTimeOfDay(&tval2);
		if (TvalDiff(&tval,&tval2) > retry_time) {
			if (!retries)
				break;
			if (!found && !send_packet(&p))
				return NULL;
			GetTimeOfDay(&tval);
			retries--;
		}
		
		if ((p2=receive_nmb_packet(fd,90,nmb->header.name_trn_id))) {     
			struct nmb_packet *nmb2 = &p2->packet.nmb;
			debug_nmb_packet(p2);
			
			/* If we get a Negative Name Query Response from a WINS
			 * server, we should report it and give up.
			 */
			if( 0 == nmb2->header.opcode		/* A query response   */
			    && !(bcast)			/* from a WINS server */
			    && nmb2->header.rcode		/* Error returned     */
				) {
				
				if( DEBUGLVL( 3 ) ) {
					/* Only executed if DEBUGLEVEL >= 3 */
					dbgtext( "Negative name query response, rcode 0x%02x: ", nmb2->header.rcode );
					switch( nmb2->header.rcode ) {
					case 0x01:
						dbgtext( "Request was invalidly formatted.\n" );
						break;
					case 0x02:
						dbgtext( "Problem with NBNS, cannot process name.\n");
						break;
					case 0x03:
						dbgtext( "The name requested does not exist.\n" );
						break;
					case 0x04:
						dbgtext( "Unsupported request error.\n" );
						break;
					case 0x05:
						dbgtext( "Query refused error.\n" );
						break;
					default:
						dbgtext( "Unrecognized error code.\n" );
						break;
					}
				}
				free_packet(p2);
				return( NULL );
			}
			
			if (nmb2->header.opcode != 0 ||
			    nmb2->header.nm_flags.bcast ||
			    nmb2->header.rcode ||
			    !nmb2->header.ancount) {
				/* 
				 * XXXX what do we do with this? Could be a
				 * redirect, but we'll discard it for the
				 * moment.
				 */
				free_packet(p2);
				continue;
			}
			
			ip_list = SMB_REALLOC_ARRAY( ip_list, struct in_addr,
						(*count) + nmb2->answers->rdlength/6 );
			
			if (!ip_list) {
				DEBUG(0,("name_query: Realloc failed.\n"));
				free_packet(p2);
				return( NULL );
			}
			
			DEBUG(2,("Got a positive name query response from %s ( ", inet_ntoa(p2->ip)));
			for (i=0;i<nmb2->answers->rdlength/6;i++) {
				putip((char *)&ip_list[(*count)],&nmb2->answers->rdata[2+i*6]);
				DEBUGADD(2,("%s ",inet_ntoa(ip_list[(*count)])));
				(*count)++;
			}
			DEBUGADD(2,(")\n"));
			
			found=True;
			retries=0;
			/* We add the flags back ... */
			if (nmb2->header.response)
				(*flags) |= NM_FLAGS_RS;
			if (nmb2->header.nm_flags.authoritative)
				(*flags) |= NM_FLAGS_AA;
			if (nmb2->header.nm_flags.trunc)
				(*flags) |= NM_FLAGS_TC;
			if (nmb2->header.nm_flags.recursion_desired)
				(*flags) |= NM_FLAGS_RD;
			if (nmb2->header.nm_flags.recursion_available)
				(*flags) |= NM_FLAGS_RA;
			if (nmb2->header.nm_flags.bcast)
				(*flags) |= NM_FLAGS_B;
			free_packet(p2);
			/*
			 * If we're doing a unicast lookup we only
			 * expect one reply. Don't wait the full 2
			 * seconds if we got one. JRA.
			 */
			if(!bcast && found)
				break;
		}
	}

	/* only set timed_out if we didn't fund what we where looking for*/
	
	if ( !found && timed_out ) {
		*timed_out = True;
	}

	/* sort the ip list so we choose close servers first if possible */
	sort_ip_list(ip_list, *count);

	return ip_list;
}

/********************************************************
 Start parsing the lmhosts file.
*********************************************************/

XFILE *startlmhosts(char *fname)
{
	XFILE *fp = x_fopen(fname,O_RDONLY, 0);
	if (!fp) {
		DEBUG(4,("startlmhosts: Can't open lmhosts file %s. Error was %s\n",
			 fname, strerror(errno)));
		return NULL;
	}
	return fp;
}

/********************************************************
 Parse the next line in the lmhosts file.
*********************************************************/

BOOL getlmhostsent( XFILE *fp, pstring name, int *name_type, struct in_addr *ipaddr)
{
	pstring line;

	while(!x_feof(fp) && !x_ferror(fp)) {
		pstring ip,flags,extra;
		const char *ptr;
		char *ptr1;
		int count = 0;

		*name_type = -1;

		if (!fgets_slash(line,sizeof(pstring),fp)) {
			continue;
		}

		if (*line == '#') {
			continue;
		}

		pstrcpy(ip,"");
		pstrcpy(name,"");
		pstrcpy(flags,"");

		ptr = line;

		if (next_token(&ptr,ip   ,NULL,sizeof(ip)))
			++count;
		if (next_token(&ptr,name ,NULL, sizeof(pstring)))
			++count;
		if (next_token(&ptr,flags,NULL, sizeof(flags)))
			++count;
		if (next_token(&ptr,extra,NULL, sizeof(extra)))
			++count;

		if (count <= 0)
			continue;

		if (count > 0 && count < 2) {
			DEBUG(0,("getlmhostsent: Ill formed hosts line [%s]\n",line));
			continue;
		}

		if (count >= 4) {
			DEBUG(0,("getlmhostsent: too many columns in lmhosts file (obsolete syntax)\n"));
			continue;
		}

		DEBUG(4, ("getlmhostsent: lmhost entry: %s %s %s\n", ip, name, flags));

		if (strchr_m(flags,'G') || strchr_m(flags,'S')) {
			DEBUG(0,("getlmhostsent: group flag in lmhosts ignored (obsolete)\n"));
			continue;
		}

		*ipaddr = *interpret_addr2(ip);

		/* Extra feature. If the name ends in '#XX', where XX is a hex number,
			then only add that name type. */
		if((ptr1 = strchr_m(name, '#')) != NULL) {
			char *endptr;
      			ptr1++;

			*name_type = (int)strtol(ptr1, &endptr, 16);
			if(!*ptr1 || (endptr == ptr1)) {
				DEBUG(0,("getlmhostsent: invalid name %s containing '#'.\n", name));
				continue;
			}

			*(--ptr1) = '\0'; /* Truncate at the '#' */
		}

		return True;
	}

	return False;
}

/********************************************************
 Finish parsing the lmhosts file.
*********************************************************/

void endlmhosts(XFILE *fp)
{
	x_fclose(fp);
}

/********************************************************
 convert an array if struct in_addrs to struct ip_service
 return False on failure.  Port is set to PORT_NONE;
*********************************************************/

static BOOL convert_ip2service( struct ip_service **return_iplist, struct in_addr *ip_list, int count )
{
	int i;

	if ( count==0 || !ip_list )
		return False;
		
	/* copy the ip address; port will be PORT_NONE */
	if ( (*return_iplist = SMB_MALLOC_ARRAY(struct ip_service, count)) == NULL ) {
		DEBUG(0,("convert_ip2service: malloc failed for %d enetries!\n", count ));
		return False;
	}
	
	for ( i=0; i<count; i++ ) {
		(*return_iplist)[i].ip   = ip_list[i];
		(*return_iplist)[i].port = PORT_NONE;
	}

	return True;
}	
/********************************************************
 Resolve via "bcast" method.
*********************************************************/

BOOL name_resolve_bcast(const char *name, int name_type,
			struct ip_service **return_iplist, int *return_count)
{
	int sock, i;
	int num_interfaces = iface_count();
	struct in_addr *ip_list;
	BOOL ret;

	if (lp_disable_netbios()) {
		DEBUG(5,("name_resolve_bcast(%s#%02x): netbios is disabled\n", name, name_type));
		return False;
	}

	*return_iplist = NULL;
	*return_count = 0;
	
	/*
	 * "bcast" means do a broadcast lookup on all the local interfaces.
	 */

	DEBUG(3,("name_resolve_bcast: Attempting broadcast lookup for name %s<0x%x>\n", name, name_type));

	sock = open_socket_in( SOCK_DGRAM, 0, 3,
			       interpret_addr(lp_socket_address()), True );

	if (sock == -1) return False;

	set_socket_options(sock,"SO_BROADCAST");
	/*
	 * Lookup the name on all the interfaces, return on
	 * the first successful match.
	 */
	for( i = num_interfaces-1; i >= 0; i--) {
		struct in_addr sendto_ip;
		int flags;
		/* Done this way to fix compiler error on IRIX 5.x */
		sendto_ip = *iface_n_bcast(i);
		ip_list = name_query(sock, name, name_type, True, 
				    True, sendto_ip, return_count, &flags, NULL);
		if( ip_list ) 
			goto success;
	}
	
	/* failed - no response */
	
	close(sock);
	return False;
	
success:
	ret = True;
	if ( !convert_ip2service(return_iplist, ip_list, *return_count) )
		ret = False;
	
	SAFE_FREE( ip_list );
	close(sock);
	return ret;
}

/********************************************************
 Resolve via "wins" method.
*********************************************************/

BOOL resolve_wins(const char *name, int name_type,
		  struct ip_service **return_iplist, int *return_count)
{
	int sock, t, i;
	char **wins_tags;
	struct in_addr src_ip, *ip_list = NULL;
	BOOL ret;

	if (lp_disable_netbios()) {
		DEBUG(5,("resolve_wins(%s#%02x): netbios is disabled\n", name, name_type));
		return False;
	}

	*return_iplist = NULL;
	*return_count = 0;
	
	DEBUG(3,("resolve_wins: Attempting wins lookup for name %s<0x%x>\n", name, name_type));

	if (wins_srv_count() < 1) {
		DEBUG(3,("resolve_wins: WINS server resolution selected and no WINS servers listed.\n"));
		return False;
	}

	/* we try a lookup on each of the WINS tags in turn */
	wins_tags = wins_srv_tags();

	if (!wins_tags) {
		/* huh? no tags?? give up in disgust */
		return False;
	}

	/* the address we will be sending from */
	src_ip = *interpret_addr2(lp_socket_address());

	/* in the worst case we will try every wins server with every
	   tag! */
	for (t=0; wins_tags && wins_tags[t]; t++) {
		int srv_count = wins_srv_count_tag(wins_tags[t]);
		for (i=0; i<srv_count; i++) {
			struct in_addr wins_ip;
			int flags;
			BOOL timed_out;

			wins_ip = wins_srv_ip_tag(wins_tags[t], src_ip);

			if (global_in_nmbd && ismyip(wins_ip)) {
				/* yikes! we'll loop forever */
				continue;
			}

			/* skip any that have been unresponsive lately */
			if (wins_srv_is_dead(wins_ip, src_ip)) {
				continue;
			}

			DEBUG(3,("resolve_wins: using WINS server %s and tag '%s'\n", inet_ntoa(wins_ip), wins_tags[t]));

			sock = open_socket_in(SOCK_DGRAM, 0, 3, src_ip.s_addr, True);
			if (sock == -1) {
				continue;
			}

			ip_list = name_query(sock,name,name_type, False, 
						    True, wins_ip, return_count, &flags, 
						    &timed_out);
						    
			/* exit loop if we got a list of addresses */
			
			if ( ip_list ) 
				goto success;
				
			close(sock);

			if (timed_out) {
				/* Timed out wating for WINS server to respond.  Mark it dead. */
				wins_srv_died(wins_ip, src_ip);
			} else {
				/* The name definately isn't in this
				   group of WINS servers. goto the next group  */
				break;
			}
		}
	}

	wins_srv_tags_free(wins_tags);
	return False;

success:
	ret = True;
	if ( !convert_ip2service( return_iplist, ip_list, *return_count ) )
		ret = False;
	
	SAFE_FREE( ip_list );
	wins_srv_tags_free(wins_tags);
	close(sock);
	
	return ret;
}

/********************************************************
 Resolve via "lmhosts" method.
*********************************************************/

static BOOL resolve_lmhosts(const char *name, int name_type,
                         struct ip_service **return_iplist, int *return_count)
{
	/*
	 * "lmhosts" means parse the local lmhosts file.
	 */
	
	XFILE *fp;
	pstring lmhost_name;
	int name_type2;
	struct in_addr return_ip;
	BOOL result = False;

	*return_iplist = NULL;
	*return_count = 0;

	DEBUG(3,("resolve_lmhosts: Attempting lmhosts lookup for name %s<0x%x>\n", name, name_type));

	fp = startlmhosts(dyn_LMHOSTSFILE);

	if ( fp == NULL )
		return False;

	while (getlmhostsent(fp, lmhost_name, &name_type2, &return_ip)) 
	{

		if (!strequal(name, lmhost_name))
			continue;

		if ((name_type2 != -1) && (name_type != name_type2))
			continue;

		*return_iplist = SMB_REALLOC_ARRAY((*return_iplist), struct ip_service,
					(*return_count)+1);

		if ((*return_iplist) == NULL) {
			endlmhosts(fp);
			DEBUG(3,("resolve_lmhosts: malloc fail !\n"));
			return False;
		}

		(*return_iplist)[*return_count].ip   = return_ip;
		(*return_iplist)[*return_count].port = PORT_NONE;
		*return_count += 1;

		/* we found something */
		result = True;

		/* Multiple names only for DC lookup */
		if (name_type != 0x1c)
			break;
	}

	endlmhosts(fp);

	return result;
}


/********************************************************
 Resolve via "hosts" method.
*********************************************************/

static BOOL resolve_hosts(const char *name, int name_type,
                         struct ip_service **return_iplist, int *return_count)
{
	/*
	 * "host" means do a localhost, or dns lookup.
	 */
	struct hostent *hp;
	
	if ( name_type != 0x20 && name_type != 0x0) {
		DEBUG(5, ("resolve_hosts: not appropriate for name type <0x%x>\n", name_type));
		return False;
	}

	*return_iplist = NULL;
	*return_count = 0;

	DEBUG(3,("resolve_hosts: Attempting host lookup for name %s<0x%x>\n", name, name_type));
	
	if (((hp = sys_gethostbyname(name)) != NULL) && (hp->h_addr != NULL)) {
		struct in_addr return_ip;
		putip((char *)&return_ip,(char *)hp->h_addr);
		*return_iplist = SMB_MALLOC_P(struct ip_service);
		if(*return_iplist == NULL) {
			DEBUG(3,("resolve_hosts: malloc fail !\n"));
			return False;
		}
		(*return_iplist)->ip   = return_ip;
		(*return_iplist)->port = PORT_NONE;
		*return_count = 1;
		return True;
	}
	return False;
}

/********************************************************
 Resolve via "ADS" method.
*********************************************************/

static BOOL resolve_ads(const char *name, int name_type,
                         struct ip_service **return_iplist, int *return_count)
{
#ifdef _XBOX
  OutputDebugString("WARING: ads lookup method is not supported due to lack of proper dns routines");
  return False;
#else
	int 			i, j;
	NTSTATUS  		status;
	TALLOC_CTX		*ctx;
	struct dns_rr_srv	*dcs = NULL;
	int			numdcs = 0;
	int			numaddrs = 0;

	if ( name_type != 0x1c )
		return False;
		
	DEBUG(5,("resolve_ads: Attempting to resolve DC's for %s using DNS\n",
		name));
			
	if ( (ctx = talloc_init("resolve_ads")) == NULL ) {
		DEBUG(0,("resolve_ads: talloc_init() failed!\n"));
		return False;
	}
		
	status = ads_dns_query_dcs( ctx, name, &dcs, &numdcs );
	if ( !NT_STATUS_IS_OK( status ) ) {
		talloc_destroy(ctx);
		return False;
	}

	for (i=0;i<numdcs;i++) {
		numaddrs += MAX(dcs[i].num_ips,1);
	}
		
	if ( (*return_iplist = SMB_MALLOC_ARRAY(struct ip_service, numaddrs)) == NULL ) {
		DEBUG(0,("resolve_ads: malloc failed for %d entries\n", numaddrs ));
		talloc_destroy(ctx);
		return False;
	}
	
	/* now unroll the list of IP addresses */

	*return_count = 0;
	i = 0;
	j = 0;
	while ( i < numdcs && (*return_count<numaddrs) ) {
		struct ip_service *r = &(*return_iplist)[*return_count];

		r->port = dcs[i].port;
		
		/* If we don't have an IP list for a name, lookup it up */
		
		if ( !dcs[i].ips ) {
			r->ip = *interpret_addr2(dcs[i].hostname);
			i++;
			j = 0;
		} else {
			/* use the IP addresses from the SRV sresponse */
			
			if ( j >= dcs[i].num_ips ) {
				i++;
				j = 0;
				continue;
			}
			
			r->ip = dcs[i].ips[j];
			j++;
		}
			
		/* make sure it is a valid IP.  I considered checking the negative
		   connection cache, but this is the wrong place for it.  Maybe only
		   as a hac.  After think about it, if all of the IP addresses retuend
		   from DNS are dead, what hope does a netbios name lookup have?
		   The standard reason for falling back to netbios lookups is that 
		   our DNS server doesn't know anything about the DC's   -- jerry */	
			   
		if ( ! is_zero_ip(r->ip) )
			(*return_count)++;
	}
		
	talloc_destroy(ctx);
	return True;
#endif
}

/*******************************************************************
 Internal interface to resolve a name into an IP address.
 Use this function if the string is either an IP address, DNS
 or host name or NetBIOS name. This uses the name switch in the
 smb.conf to determine the order of name resolution.
 
 Added support for ip addr/port to support ADS ldap servers.
 the only place we currently care about the port is in the 
 resolve_hosts() when looking up DC's via SRV RR entries in DNS
**********************************************************************/

BOOL internal_resolve_name(const char *name, int name_type,
			   struct ip_service **return_iplist, 
			   int *return_count, const char *resolve_order)
{
	pstring name_resolve_list;
	fstring tok;
	const char *ptr;
	BOOL allones = (strcmp(name,"255.255.255.255") == 0);
	BOOL allzeros = (strcmp(name,"0.0.0.0") == 0);
	BOOL is_address = is_ipaddress(name);
	BOOL result = False;
	int i;

	*return_iplist = NULL;
	*return_count = 0;

	DEBUG(10, ("internal_resolve_name: looking up %s#%x\n", name, name_type));

	if (allzeros || allones || is_address) {
  
		if ( (*return_iplist = SMB_MALLOC_P(struct ip_service)) == NULL ) {
			DEBUG(0,("internal_resolve_name: malloc fail !\n"));
			return False;
		}
	
		if(is_address) { 
			/* ignore the port here */
			(*return_iplist)->port = PORT_NONE;
		
			/* if it's in the form of an IP address then get the lib to interpret it */
			if (((*return_iplist)->ip.s_addr = inet_addr(name)) == 0xFFFFFFFF ){
				DEBUG(1,("internal_resolve_name: inet_addr failed on %s\n", name));
				SAFE_FREE(*return_iplist);
				return False;
			}
		} else {
			(*return_iplist)->ip.s_addr = allones ? 0xFFFFFFFF : 0;
		}
		*return_count = 1;
		return True;
	}
  
	/* Check name cache */

	if (namecache_fetch(name, name_type, return_iplist, return_count)) {
		/* This could be a negative response */
		return (*return_count > 0);
	}

	/* set the name resolution order */

	if ( strcmp( resolve_order, "NULL") == 0 ) {
		DEBUG(8,("internal_resolve_name: all lookups disabled\n"));
		return False;
	}
  
	if ( !resolve_order ) {
		pstrcpy(name_resolve_list, lp_name_resolve_order());
	} else {
		pstrcpy(name_resolve_list, resolve_order);
	}

	if ( !name_resolve_list[0] ) {
		ptr = "host";
	} else {
		ptr = name_resolve_list;
	}

	/* iterate through the name resolution backends */
  
	while (next_token(&ptr, tok, LIST_SEP, sizeof(tok))) {
		if((strequal(tok, "host") || strequal(tok, "hosts"))) {
			if (resolve_hosts(name, name_type, return_iplist, return_count)) {
				result = True;
				goto done;
			}
		} else if(strequal( tok, "ads")) {
			/* deal with 0x1c names here.  This will result in a
				SRV record lookup */
			if (resolve_ads(name, name_type, return_iplist, return_count)) {
				result = True;
				goto done;
			}
		} else if(strequal( tok, "lmhosts")) {
			if (resolve_lmhosts(name, name_type, return_iplist, return_count)) {
				result = True;
				goto done;
			}
		} else if(strequal( tok, "wins")) {
			/* don't resolve 1D via WINS */
			if (name_type != 0x1D && resolve_wins(name, name_type, return_iplist, return_count)) {
				result = True;
				goto done;
			}
		} else if(strequal( tok, "bcast")) {
			if (name_resolve_bcast(name, name_type, return_iplist, return_count)) {
				result = True;
				goto done;
			}
		} else {
			DEBUG(0,("resolve_name: unknown name switch type %s\n", tok));
		}
	}

	/* All of the resolve_* functions above have returned false. */

	SAFE_FREE(*return_iplist);
	*return_count = 0;

	return False;

  done:

	/* Remove duplicate entries.  Some queries, notably #1c (domain
	controllers) return the PDC in iplist[0] and then all domain
	controllers including the PDC in iplist[1..n].  Iterating over
	the iplist when the PDC is down will cause two sets of timeouts. */

	if ( *return_count ) {
		*return_count = remove_duplicate_addrs2( *return_iplist, *return_count );
	}
 
	/* Save in name cache */
	if ( DEBUGLEVEL >= 100 ) {
		for (i = 0; i < *return_count && DEBUGLEVEL == 100; i++)
			DEBUG(100, ("Storing name %s of type %d (%s:%d)\n", name,
				name_type, inet_ntoa((*return_iplist)[i].ip), (*return_iplist)[i].port));
	}
   
	namecache_store(name, name_type, *return_count, *return_iplist);

	/* Display some debugging info */

	if ( DEBUGLEVEL >= 10 ) {
		DEBUG(10, ("internal_resolve_name: returning %d addresses: ", *return_count));

		for (i = 0; i < *return_count; i++) {
			DEBUGADD(10, ("%s:%d ", inet_ntoa((*return_iplist)[i].ip), (*return_iplist)[i].port));
		}
		DEBUG(10, ("\n"));
	}
  
	return result;
}

/********************************************************
 Internal interface to resolve a name into one IP address.
 Use this function if the string is either an IP address, DNS
 or host name or NetBIOS name. This uses the name switch in the
 smb.conf to determine the order of name resolution.
*********************************************************/

BOOL resolve_name(const char *name, struct in_addr *return_ip, int name_type)
{
	struct ip_service *ip_list = NULL;
	int count = 0;

	if (is_ipaddress(name)) {
		*return_ip = *interpret_addr2(name);
		return True;
	}

	if (internal_resolve_name(name, name_type, &ip_list, &count, lp_name_resolve_order())) {
		int i;
		
		/* only return valid addresses for TCP connections */
		for (i=0; i<count; i++) {
			char *ip_str = inet_ntoa(ip_list[i].ip);
			if (ip_str &&
			    strcmp(ip_str, "255.255.255.255") != 0 &&
			    strcmp(ip_str, "0.0.0.0") != 0) 
			{
				*return_ip = ip_list[i].ip;
				SAFE_FREE(ip_list);
				return True;
			}
		}
	}
	
	SAFE_FREE(ip_list);
	return False;
}

/********************************************************
 Find the IP address of the master browser or DMB for a workgroup.
*********************************************************/

BOOL find_master_ip(const char *group, struct in_addr *master_ip)
{
	struct ip_service *ip_list = NULL;
	int count = 0;

	if (lp_disable_netbios()) {
		DEBUG(5,("find_master_ip(%s): netbios is disabled\n", group));
		return False;
	}

	if (internal_resolve_name(group, 0x1D, &ip_list, &count, lp_name_resolve_order())) {
		*master_ip = ip_list[0].ip;
		SAFE_FREE(ip_list);
		return True;
	}
	if(internal_resolve_name(group, 0x1B, &ip_list, &count, lp_name_resolve_order())) {
		*master_ip = ip_list[0].ip;
		SAFE_FREE(ip_list);
		return True;
	}

	SAFE_FREE(ip_list);
	return False;
}

/********************************************************
 Get the IP address list of the primary domain controller
 for a domain.
*********************************************************/

BOOL get_pdc_ip(const char *domain, struct in_addr *ip)
{
	struct ip_service *ip_list;
	int count;

	/* Look up #1B name */

	if (!internal_resolve_name(domain, 0x1b, &ip_list, &count, lp_name_resolve_order())) {
		return False;
	}

	/* if we get more than 1 IP back we have to assume it is a
	   multi-homed PDC and not a mess up */

	if ( count > 1 ) {
		DEBUG(6,("get_pdc_ip: PDC has %d IP addresses!\n", count));		
		sort_ip_list2( ip_list, count );
	}

	*ip = ip_list[0].ip;
	
	SAFE_FREE(ip_list);

	return True;
}

/********************************************************
 Get the IP address list of the domain controllers for
 a domain.
*********************************************************/

static BOOL get_dc_list(const char *domain, struct ip_service **ip_list, 
                 int *count, BOOL ads_only, int *ordered)
{
	fstring resolve_order;
	char *saf_servername;
	pstring pserver;
	const char *p;
	char *port_str;
	int port;
	fstring name;
	int num_addresses = 0;
	int  local_count, i, j;
	struct ip_service *return_iplist = NULL;
	struct ip_service *auto_ip_list = NULL;
	BOOL done_auto_lookup = False;
	int auto_count = 0;

	*ordered = False;

	/* if we are restricted to solely using DNS for looking
	   up a domain controller, make sure that host lookups
	   are enabled for the 'name resolve order'.  If host lookups
	   are disabled and ads_only is True, then set the string to
	   NULL. */

	fstrcpy( resolve_order, lp_name_resolve_order() );
	strlower_m( resolve_order );
	if ( ads_only )  {
		if ( strstr( resolve_order, "host" ) ) {
			fstrcpy( resolve_order, "ads" );

			/* DNS SRV lookups used by the ads resolver
			   are already sorted by priority and weight */
			*ordered = True;
		} else {
                        fstrcpy( resolve_order, "NULL" );
		}
	}

	/* fetch the server we have affinity for.  Add the 
	   'password server' list to a search for our domain controllers */
	
	saf_servername = saf_fetch( domain );
	
	if ( strequal(domain, lp_workgroup()) || strequal(domain, lp_realm()) ) {
		pstr_sprintf( pserver, "%s, %s", 
			saf_servername ? saf_servername : "",
			lp_passwordserver() );
	} else {
		pstr_sprintf( pserver, "%s, *", 
			saf_servername ? saf_servername : "" );
	}

	SAFE_FREE( saf_servername );

	/* if we are starting from scratch, just lookup DOMAIN<0x1c> */

	if ( !*pserver ) {
		DEBUG(10,("get_dc_list: no preferred domain controllers.\n"));
		return internal_resolve_name(domain, 0x1C, ip_list, count, resolve_order);
	}

	DEBUG(3,("get_dc_list: preferred server list: \"%s\"\n", pserver ));
	
	/*
	 * if '*' appears in the "password server" list then add
	 * an auto lookup to the list of manually configured
	 * DC's.  If any DC is listed by name, then the list should be 
	 * considered to be ordered 
	 */

	p = pserver;
	while (next_token(&p,name,LIST_SEP,sizeof(name))) {
		if (strequal(name, "*")) {
			if ( internal_resolve_name(domain, 0x1C, &auto_ip_list, &auto_count, resolve_order) )
				num_addresses += auto_count;
			done_auto_lookup = True;
			DEBUG(8,("Adding %d DC's from auto lookup\n", auto_count));
		} else  {
			num_addresses++;
		}
	}

	/* if we have no addresses and haven't done the auto lookup, then
	   just return the list of DC's.  Or maybe we just failed. */
		   
	if ( (num_addresses == 0) ) {
		if ( !done_auto_lookup ) {
			return internal_resolve_name(domain, 0x1C, ip_list, count, resolve_order);
		} else {
			DEBUG(4,("get_dc_list: no servers found\n")); 
			return False;
		}
	}

	if ( (return_iplist = SMB_MALLOC_ARRAY(struct ip_service, num_addresses)) == NULL ) {
		DEBUG(3,("get_dc_list: malloc fail !\n"));
		return False;
	}

	p = pserver;
	local_count = 0;

	/* fill in the return list now with real IP's */
				
	while ( (local_count<num_addresses) && next_token(&p,name,LIST_SEP,sizeof(name)) ) {
		struct in_addr name_ip;
			
		/* copy any addersses from the auto lookup */
			
		if ( strequal(name, "*") ) {
			for ( j=0; j<auto_count; j++ ) {
				/* Check for and don't copy any known bad DC IP's. */
				if(!NT_STATUS_IS_OK(check_negative_conn_cache(domain, 
						inet_ntoa(auto_ip_list[j].ip)))) {
					DEBUG(5,("get_dc_list: negative entry %s removed from DC list\n",
						inet_ntoa(auto_ip_list[j].ip) ));
					continue;
				}
				return_iplist[local_count].ip   = auto_ip_list[j].ip;
				return_iplist[local_count].port = auto_ip_list[j].port;
				local_count++;
			}
			continue;
		}
			
			
		/* added support for address:port syntax for ads (not that I think 
		   anyone will ever run the LDAP server in an AD domain on something 
		   other than port 389 */
			
		port = (lp_security() == SEC_ADS) ? LDAP_PORT : PORT_NONE;
		if ( (port_str=strchr(name, ':')) != NULL ) {
			*port_str = '\0';
			port_str++;
			port = atoi( port_str );
		}

		/* explicit lookup; resolve_name() will handle names & IP addresses */
		if ( resolve_name( name, &name_ip, 0x20 ) ) {

			/* Check for and don't copy any known bad DC IP's. */
			if( !NT_STATUS_IS_OK(check_negative_conn_cache(domain, inet_ntoa(name_ip))) ) {
				DEBUG(5,("get_dc_list: negative entry %s removed from DC list\n",name ));
				continue;
			}

			return_iplist[local_count].ip 	= name_ip;
			return_iplist[local_count].port = port;
			local_count++;
			*ordered = True;
		}
	}
				
	SAFE_FREE(auto_ip_list);

	/* need to remove duplicates in the list if we have any 
	   explicit password servers */
	   
	if ( local_count ) {
		local_count = remove_duplicate_addrs2( return_iplist, local_count );
	}
		
	if ( DEBUGLEVEL >= 4 ) {
		DEBUG(4,("get_dc_list: returning %d ip addresses in an %sordered list\n", local_count, 
			*ordered ? "":"un"));
		DEBUG(4,("get_dc_list: "));
		for ( i=0; i<local_count; i++ )
			DEBUGADD(4,("%s:%d ", inet_ntoa(return_iplist[i].ip), return_iplist[i].port ));
		DEBUGADD(4,("\n"));
	}
			
	*ip_list = return_iplist;
	*count = local_count;

	return (*count != 0);
}

/*********************************************************************
 Small wrapper function to get the DC list and sort it if neccessary.
*********************************************************************/

BOOL get_sorted_dc_list( const char *domain, struct ip_service **ip_list, int *count, BOOL ads_only )
{
	BOOL ordered;
	
	DEBUG(8,("get_sorted_dc_list: attempting lookup using [%s]\n",
		(ads_only ? "ads" : lp_name_resolve_order())));
	
	if ( !get_dc_list(domain, ip_list, count, ads_only, &ordered) ) {
		return False; 
	}
		
	/* only sort if we don't already have an ordered list */
	if ( !ordered ) {
		sort_ip_list2( *ip_list, *count );
	}
		
	return True;
}
