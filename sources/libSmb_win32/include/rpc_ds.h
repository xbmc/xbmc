/* 
   Unix SMB/CIFS implementation.
   SMB parameters and setup
   Copyright (C) Gerald Carter			2002
      
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

#ifndef _RPC_DS_H /* _RPC_LSA_H */
#define _RPC_DS_H 

/* Opcodes available on PIPE_LSARPC_DS */

#define DS_GETPRIMDOMINFO      0x00
#define DS_NOP                 0xFF	/* no op -- placeholder */

/* Opcodes available on PIPE_NETLOGON */

#define DS_ENUM_DOM_TRUSTS      0x28


/* macros for RPC's */

/* DSROLE_PRIMARY_DOMAIN_INFO_BASIC */

/* flags */

#define DSROLE_PRIMARY_DS_RUNNING           0x00000001
#define DSROLE_PRIMARY_DS_MIXED_MODE        0x00000002
#define DSROLE_UPGRADE_IN_PROGRESS          0x00000004
#define DSROLE_PRIMARY_DOMAIN_GUID_PRESENT  0x01000000

/* machine role */

#define DSROLE_STANDALONE_SRV		2	
#define DSROLE_DOMAIN_MEMBER_SRV	3
#define DSROLE_BDC			4
#define DSROLE_PDC			5

/* Settings for the domainFunctionality attribteu in the rootDSE */

#define DS_DOMAIN_FUNCTION_2000		0
#define DS_DOMAIN_FUCNTION_2003_MIXED	1
#define DS_DOMAIN_FUNCTION_2003		2



typedef struct
{
	uint16		machine_role;
	uint16		unknown;		/* 0x6173 -- maybe just alignment? */
	
	uint32		flags;
	
	uint32		netbios_ptr;
	uint32		dnsname_ptr;
	uint32		forestname_ptr;
	
	struct uuid	domain_guid;
	
	UNISTR2	netbios_domain;

	UNISTR2	dns_domain;	/* our dns domain */
	UNISTR2	forest_domain;	/* root domain of the forest to which we belong */
} DSROLE_PRIMARY_DOMAIN_INFO_BASIC;

typedef struct
{
	DSROLE_PRIMARY_DOMAIN_INFO_BASIC	*basic;
} DS_DOMINFO_CTR;

/* info levels for ds_getprimdominfo() */

#define DsRolePrimaryDomainInfoBasic		1


/* DS_Q_GETPRIMDOMINFO - DsGetPrimaryDomainInformation() request */
typedef struct 
{
	uint16	level;
} DS_Q_GETPRIMDOMINFO;

/* DS_R_GETPRIMDOMINFO - DsGetPrimaryDomainInformation() response */
typedef struct 
{
	uint32		ptr;
		
	uint16		level;
	uint16		unknown0;	/* 0x455c -- maybe just alignment? */

	DS_DOMINFO_CTR	info;
	
	NTSTATUS status;
} DS_R_GETPRIMDOMINFO;

typedef struct {
	/* static portion of structure */
	uint32		netbios_ptr;
	uint32		dns_ptr;
	uint32		flags;
	uint32		parent_index;
	uint32		trust_type;
	uint32		trust_attributes;
	uint32		sid_ptr;
	struct uuid	guid;
	
	UNISTR2		netbios_domain;
	UNISTR2		dns_domain;
	DOM_SID2	sid;

} DS_DOMAIN_TRUSTS;

struct ds_domain_trust {
	/* static portion of structure */
	uint32		flags;
	uint32		parent_index;
	uint32		trust_type;
	uint32		trust_attributes;
	struct uuid	guid;
	
	DOM_SID	sid;
	char *netbios_domain;
	char *dns_domain;
};

typedef struct {

	uint32			ptr;
	uint32			max_count;
	DS_DOMAIN_TRUSTS 	*trusts;
	
} DS_DOMAIN_TRUSTS_CTR;

#define DS_DOMAIN_IN_FOREST           0x0001 	/* domains in the forest to which 
						   we belong; even different domain trees */
#define DS_DOMAIN_DIRECT_OUTBOUND     0x0002  	/* trusted domains */
#define DS_DOMAIN_TREE_ROOT           0x0004  	/* root of our forest; also available in
						   DsRoleGetPrimaryDomainInfo() */
#define DS_DOMAIN_PRIMARY             0x0008  	/* our domain */
#define DS_DOMAIN_NATIVE_MODE         0x0010  	/* native mode AD servers */
#define DS_DOMAIN_DIRECT_INBOUND      0x0020 	/* trusting domains */

/* DS_Q_ENUM_DOM_TRUSTS - DsEnumerateDomainTrusts() request */
typedef struct 
{
	uint32		server_ptr;
	UNISTR2		server;
	uint32		flags;
	
} DS_Q_ENUM_DOM_TRUSTS;

/* DS_R_ENUM_DOM_TRUSTS - DsEnumerateDomainTrusts() response */
typedef struct 
{
	uint32			num_domains;
	DS_DOMAIN_TRUSTS_CTR	domains;
		
	NTSTATUS status;

} DS_R_ENUM_DOM_TRUSTS;


#endif /* _RPC_DS_H */
