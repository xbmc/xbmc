/* 
   Unix SMB/CIFS implementation.
   DNS utility library
   Copyright (C) Gerald (Jerry) Carter           2006.

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

/* AIX resolv.h uses 'class' in struct ns_rr */

#if defined(AIX)
#  if defined(class)
#    undef class
#  endif
#endif	/* AIX */

/* resolver headers */

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <netdb.h>

#define MAX_DNS_PACKET_SIZE 0xffff

#ifdef NS_HFIXEDSZ	/* Bind 8/9 interface */
#if !defined(C_IN)	/* AIX 5.3 already defines C_IN */
#  define C_IN		ns_c_in
#endif
#if !defined(T_A)	/* AIX 5.3 already defines T_A */
#  define T_A   	ns_t_a
#endif
#  define T_SRV 	ns_t_srv
#else
#  ifdef HFIXEDSZ
#    define NS_HFIXEDSZ HFIXEDSZ
#  else
#    define NS_HFIXEDSZ sizeof(HEADER)
#  endif	/* HFIXEDSZ */
#  ifdef PACKETSZ
#    define NS_PACKETSZ	PACKETSZ
#  else	/* 512 is usually the default */
#    define NS_PACKETSZ	512
#  endif	/* PACKETSZ */
#  define T_SRV 	33
#endif

/*********************************************************************
*********************************************************************/

static BOOL ads_dns_parse_query( TALLOC_CTX *ctx, uint8 *start, uint8 *end,
                          uint8 **ptr, struct dns_query *q )
{
	uint8 *p = *ptr;
	pstring hostname;
	int namelen;

	ZERO_STRUCTP( q );
	
	if ( !start || !end || !q || !*ptr)
		return False;

	/* See RFC 1035 for details. If this fails, then return. */

	namelen = dn_expand( start, end, p, hostname, sizeof(hostname) );
	if ( namelen < 0 ) {
		return False;
	}
	p += namelen;
	q->hostname = talloc_strdup( ctx, hostname );

	/* check that we have space remaining */

	if ( PTR_DIFF(p+4, end) > 0 )
		return False;

	q->type     = RSVAL( p, 0 );
	q->in_class = RSVAL( p, 2 );
	p += 4;

	*ptr = p;

	return True;
}

/*********************************************************************
*********************************************************************/

static BOOL ads_dns_parse_rr( TALLOC_CTX *ctx, uint8 *start, uint8 *end,
                       uint8 **ptr, struct dns_rr *rr )
{
	uint8 *p = *ptr;
	pstring hostname;
	int namelen;

	if ( !start || !end || !rr || !*ptr)
		return -1;

	ZERO_STRUCTP( rr );
	/* pull the name from the answer */

	namelen = dn_expand( start, end, p, hostname, sizeof(hostname) );
	if ( namelen < 0 ) {
		return -1;
	}
	p += namelen;
	rr->hostname = talloc_strdup( ctx, hostname );

	/* check that we have space remaining */

	if ( PTR_DIFF(p+10, end) > 0 )
		return False;

	/* pull some values and then skip onto the string */

	rr->type     = RSVAL(p, 0);
	rr->in_class = RSVAL(p, 2);
	rr->ttl      = RIVAL(p, 4);
	rr->rdatalen = RSVAL(p, 8);
	
	p += 10;

	/* sanity check the available space */

	if ( PTR_DIFF(p+rr->rdatalen, end ) > 0 ) {
		return False;

	}

	/* save a point to the rdata for this section */

	rr->rdata = p;
	p += rr->rdatalen;

	*ptr = p;

	return True;
}

/*********************************************************************
*********************************************************************/

static BOOL ads_dns_parse_rr_srv( TALLOC_CTX *ctx, uint8 *start, uint8 *end,
                       uint8 **ptr, struct dns_rr_srv *srv )
{
	struct dns_rr rr;
	uint8 *p;
	pstring dcname;
	int namelen;

	if ( !start || !end || !srv || !*ptr)
		return -1;

	/* Parse the RR entry.  Coming out of the this, ptr is at the beginning 
	   of the next record */

	if ( !ads_dns_parse_rr( ctx, start, end, ptr, &rr ) ) {
		DEBUG(1,("ads_dns_parse_rr_srv: Failed to parse RR record\n"));
		return False;
	}

	if ( rr.type != T_SRV ) {
		DEBUG(1,("ads_dns_parse_rr_srv: Bad answer type (%d)\n", rr.type));
		return False;
	}

	p = rr.rdata;

	srv->priority = RSVAL(p, 0);
	srv->weight   = RSVAL(p, 2);
	srv->port     = RSVAL(p, 4);

	p += 6;

	namelen = dn_expand( start, end, p, dcname, sizeof(dcname) );
	if ( namelen < 0 ) {
		DEBUG(1,("ads_dns_parse_rr_srv: Failed to uncompress name!\n"));
		return False;
	}
	srv->hostname = talloc_strdup( ctx, dcname );

	return True;
}


/*********************************************************************
 Sort SRV record list based on weight and priority.  See RFC 2782.
*********************************************************************/

static int dnssrvcmp( struct dns_rr_srv *a, struct dns_rr_srv *b )
{
	if ( a->priority == b->priority ) {

		/* randomize entries with an equal weight and priority */
		if ( a->weight == b->weight ) 
			return 0;

		/* higher weights should be sorted lower */ 
		if ( a->weight > b->weight )
			return -1;
		else
			return 1;
	}
		
	if ( a->priority < b->priority )
		return -1;

	return 1;
}

/*********************************************************************
 Simple wrapper for a DNS SRV query
*********************************************************************/

NTSTATUS ads_dns_lookup_srv( TALLOC_CTX *ctx, const char *name, struct dns_rr_srv **dclist, int *numdcs )
{
	uint8 *buffer = NULL;
	size_t buf_len;
	int resp_len = NS_PACKETSZ;
	struct dns_rr_srv *dcs = NULL;
	int query_count, answer_count, auth_count, additional_count;
	uint8 *p = buffer;
	int rrnum;
	int idx = 0;

	if ( !ctx || !name || !dclist ) {
		return NT_STATUS_INVALID_PARAMETER;
	}
	
	/* Send the request.  May have to loop several times in case 
	   of large replies */

	do {
		if ( buffer )
			TALLOC_FREE( buffer );
		
		buf_len = resp_len * sizeof(uint8);

		if ( (buffer = TALLOC_ARRAY(ctx, uint8, buf_len)) == NULL ) {
			DEBUG(0,("ads_dns_lookup_srv: talloc() failed!\n"));
			return NT_STATUS_NO_MEMORY;
		}

		if ( (resp_len = res_query(name, C_IN, T_SRV, buffer, buf_len)) < 0 ) {
			DEBUG(1,("ads_dns_lookup_srv: Failed to resolve %s (%s)\n", name, strerror(errno)));
			TALLOC_FREE( buffer );
			return NT_STATUS_UNSUCCESSFUL;
		}
	} while ( buf_len < resp_len && resp_len < MAX_DNS_PACKET_SIZE );

	p = buffer;

	/* For some insane reason, the ns_initparse() et. al. routines are only
	   available in libresolv.a, and not the shared lib.  Who knows why....
	   So we have to parse the DNS reply ourselves */

	/* Pull the answer RR's count from the header.  Use the NMB ordering macros */

	query_count      = RSVAL( p, 4 );
	answer_count     = RSVAL( p, 6 );
	auth_count       = RSVAL( p, 8 );
	additional_count = RSVAL( p, 10 );

	DEBUG(4,("ads_dns_lookup_srv: %d records returned in the answer section.\n", 
		answer_count));
		
	if ( (dcs = TALLOC_ZERO_ARRAY(ctx, struct dns_rr_srv, answer_count)) == NULL ) {
		DEBUG(0,("ads_dns_lookup_srv: talloc() failure for %d char*'s\n", 
			answer_count));
		return NT_STATUS_NO_MEMORY;
	}

	/* now skip the header */

	p += NS_HFIXEDSZ;

	/* parse the query section */

	for ( rrnum=0; rrnum<query_count; rrnum++ ) {
		struct dns_query q;

		if ( !ads_dns_parse_query( ctx, buffer, buffer+resp_len, &p, &q ) ) {
			DEBUG(1,("ads_dns_lookup_srv: Failed to parse query record!\n"));
			return NT_STATUS_UNSUCCESSFUL;
		}
	}

	/* now we are at the answer section */

	for ( rrnum=0; rrnum<answer_count; rrnum++ ) {
		if ( !ads_dns_parse_rr_srv( ctx, buffer, buffer+resp_len, &p, &dcs[rrnum] ) ) {
			DEBUG(1,("ads_dns_lookup_srv: Failed to parse answer record!\n"));
			return NT_STATUS_UNSUCCESSFUL;
		}		
	}
	idx = rrnum;

	/* Parse the authority section */
	/* just skip these for now */

	for ( rrnum=0; rrnum<auth_count; rrnum++ ) {
		struct dns_rr rr;

		if ( !ads_dns_parse_rr( ctx, buffer, buffer+resp_len, &p, &rr ) ) {
			DEBUG(1,("ads_dns_lookup_srv: Failed to parse authority record!\n"));
			return NT_STATUS_UNSUCCESSFUL;
		}
	}

	/* Parse the additional records section */

	for ( rrnum=0; rrnum<additional_count; rrnum++ ) {
		struct dns_rr rr;
		int i;

		if ( !ads_dns_parse_rr( ctx, buffer, buffer+resp_len, &p, &rr ) ) {
			DEBUG(1,("ads_dns_lookup_srv: Failed to parse additional records section!\n"));
			return NT_STATUS_UNSUCCESSFUL;
		}

		/* only interested in A records as a shortcut for having to come 
		   back later and lookup the name.  For multi-homed hosts, the 
		   number of additional records and exceed the number of answer 
		   records. */
		  

		if ( (rr.type != T_A) || (rr.rdatalen != 4) ) 
			continue;

		for ( i=0; i<idx; i++ ) {
			if ( strcmp( rr.hostname, dcs[i].hostname ) == 0 ) {
				int num_ips = dcs[i].num_ips;
				uint8 *buf;
				struct in_addr *tmp_ips;

				/* allocate new memory */
				
				if ( dcs[i].num_ips == 0 ) {
					if ( (dcs[i].ips = TALLOC_ARRAY( dcs, 
						struct in_addr, 1 )) == NULL ) 
					{
						return NT_STATUS_NO_MEMORY;
					}
				} else {
					if ( (tmp_ips = TALLOC_REALLOC_ARRAY( dcs, dcs[i].ips,
						struct in_addr, dcs[i].num_ips+1)) == NULL ) 
					{
						return NT_STATUS_NO_MEMORY;
					}
					
					dcs[i].ips = tmp_ips;
				}
				dcs[i].num_ips++;
				
				/* copy the new IP address */
				
				buf = (uint8*)&dcs[i].ips[num_ips].s_addr;
				memcpy( buf, rr.rdata, 4 );
			}
		}
	}

	qsort( dcs, idx, sizeof(struct dns_rr_srv), QSORT_CAST dnssrvcmp );
	
	*dclist = dcs;
	*numdcs = idx;
	
	return NT_STATUS_OK;
}

/********************************************************************
********************************************************************/

NTSTATUS ads_dns_query_dcs( TALLOC_CTX *ctx, const char *domain, struct dns_rr_srv **dclist, int *numdcs )
{
	pstring name;

	snprintf( name, sizeof(name), "_ldap._tcp.dc._msdcs.%s", domain );

	return ads_dns_lookup_srv( ctx, name, dclist, numdcs );
}

