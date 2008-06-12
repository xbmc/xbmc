/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell               1992-1997.
 *  Copyright (C) Luke Kenneth Casson Leighton  1996-1997.
 *  Copyright (C) Paul Ashton                        1997.
 *  Copyright (C) Jeremy Allison                     2001.
 *  Copyright (C) Gerald Carter                      2002.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Implementation of registry functions. */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

/********************************************************************
 Fill in a DS_DOMINFO_CTR structure
 ********************************************************************/

static NTSTATUS fill_dsrole_dominfo_basic(TALLOC_CTX *ctx, DSROLE_PRIMARY_DOMAIN_INFO_BASIC **info) 
{
	DSROLE_PRIMARY_DOMAIN_INFO_BASIC *basic;
	const char *netbios_domain;
	fstring dnsdomain;

	DEBUG(10,("fill_dsrole_dominfo_basic: enter\n"));

	if ( !(basic = TALLOC_ZERO_P(ctx, DSROLE_PRIMARY_DOMAIN_INFO_BASIC)) ) {
		DEBUG(0,("fill_dsrole_dominfo_basic: FATAL error!  talloc_xero() failed\n"));
		return NT_STATUS_NO_MEMORY;
	}

	get_mydnsdomname(dnsdomain);
	strlower_m(dnsdomain);

	switch ( lp_server_role() ) {
		case ROLE_STANDALONE:
			basic->machine_role = DSROLE_STANDALONE_SRV;
			break;
		case ROLE_DOMAIN_MEMBER:
			basic->machine_role = DSROLE_DOMAIN_MEMBER_SRV;
			break;
		case ROLE_DOMAIN_BDC:
			basic->machine_role = DSROLE_BDC;
			basic->flags = DSROLE_PRIMARY_DS_RUNNING|DSROLE_PRIMARY_DS_MIXED_MODE;
			if ( secrets_fetch_domain_guid( lp_workgroup(), &basic->domain_guid ) )
				basic->flags |= DSROLE_PRIMARY_DOMAIN_GUID_PRESENT;
			break;
		case ROLE_DOMAIN_PDC:
			basic->machine_role = DSROLE_PDC;
			basic->flags = DSROLE_PRIMARY_DS_RUNNING|DSROLE_PRIMARY_DS_MIXED_MODE;
			if ( secrets_fetch_domain_guid( lp_workgroup(), &basic->domain_guid ) )
				basic->flags |= DSROLE_PRIMARY_DOMAIN_GUID_PRESENT;
			break;
	}

	basic->unknown = 0x6173;		/* seen on the wire; maybe padding */

	/* always set netbios name */

	basic->netbios_ptr = 1;
	netbios_domain = get_global_sam_name();
	init_unistr2( &basic->netbios_domain, netbios_domain, UNI_FLAGS_NONE);

	basic->dnsname_ptr = 1;
	init_unistr2( &basic->dns_domain, dnsdomain, UNI_FLAGS_NONE);
	basic->forestname_ptr = 1;
	init_unistr2( &basic->forest_domain, dnsdomain, UNI_FLAGS_NONE);
	

	/* fill in some additional fields if we are a member of an AD domain */

	if ( lp_security() == SEC_ADS ) {	
		/* TODO */
		;;
	}

	*info = basic;

	return NT_STATUS_OK;
}

/********************************************************************
 Implement the DsroleGetPrimaryDomainInfo() call
 ********************************************************************/

NTSTATUS _dsrole_get_primary_dominfo(pipes_struct *p, DS_Q_GETPRIMDOMINFO *q_u, DS_R_GETPRIMDOMINFO *r_u)
{
	NTSTATUS result = NT_STATUS_OK;
	uint32 level = q_u->level;

	switch ( level ) {

		case DsRolePrimaryDomainInfoBasic:
			r_u->level = DsRolePrimaryDomainInfoBasic;
			r_u->ptr = 1;
			result = fill_dsrole_dominfo_basic( p->mem_ctx, &r_u->info.basic );
			break;

		default:
			DEBUG(0,("_dsrole_get_primary_dominfo: Unsupported info level [%d]!\n",
				level));
			result = NT_STATUS_INVALID_LEVEL;
	}

	return result;
}



