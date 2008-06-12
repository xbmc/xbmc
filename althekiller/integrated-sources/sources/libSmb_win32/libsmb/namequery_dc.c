/* 
   Unix SMB/CIFS implementation.

   Winbind daemon connection manager

   Copyright (C) Tim Potter 2001
   Copyright (C) Andrew Bartlett 2002
   Copyright (C) Gerald Carter 2003
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include "includes.h"

/**************************************************************************
 Find the name and IP address for a server in he realm/domain
 *************************************************************************/
 
static BOOL ads_dc_name(const char *domain, const char *realm, struct in_addr *dc_ip, fstring srv_name)
{
	ADS_STRUCT *ads;

	if (!realm && strequal(domain, lp_workgroup()))
		realm = lp_realm();

	ads = ads_init(realm, domain, NULL);
	if (!ads)
		return False;

	DEBUG(4,("ads_dc_name: domain=%s\n", domain));

#ifdef HAVE_ADS
	/* we don't need to bind, just connect */
	ads->auth.flags |= ADS_AUTH_NO_BIND;

	ads_connect(ads);
#endif

	if (!ads->config.realm) {
		ads_destroy(&ads);
		return False;
	}

	fstrcpy(srv_name, ads->config.ldap_server_name);
	strupper_m(srv_name);
	*dc_ip = ads->ldap_ip;
	ads_destroy(&ads);
	
	DEBUG(4,("ads_dc_name: using server='%s' IP=%s\n",
		 srv_name, inet_ntoa(*dc_ip)));
	
	return True;
}

/****************************************************************************
 Utility function to return the name of a DC. The name is guaranteed to be 
 valid since we have already done a name_status_find on it 
 ***************************************************************************/

static BOOL rpc_dc_name(const char *domain, fstring srv_name, struct in_addr *ip_out)
{
	struct ip_service *ip_list = NULL;
	struct in_addr dc_ip, exclude_ip;
	int count, i;
	NTSTATUS result;
	
	zero_ip(&exclude_ip);

	/* get a list of all domain controllers */
	
	if ( !get_sorted_dc_list(domain, &ip_list, &count, False) ) {
		DEBUG(3, ("Could not look up dc's for domain %s\n", domain));
		return False;
	}

	/* Remove the entry we've already failed with (should be the PDC). */

	for (i = 0; i < count; i++) {
		if (is_zero_ip(ip_list[i].ip))
			continue;

		if (name_status_find(domain, 0x1c, 0x20, ip_list[i].ip, srv_name)) {
			result = check_negative_conn_cache( domain, srv_name );
			if ( NT_STATUS_IS_OK(result) ) {
				dc_ip = ip_list[i].ip;
				goto done;
			}
		}
	}
	

	SAFE_FREE(ip_list);

	/* No-one to talk to )-: */
	return False;		/* Boo-hoo */
	
 done:
	/* We have the netbios name and IP address of a domain controller.
	   Ideally we should sent a SAMLOGON request to determine whether
	   the DC is alive and kicking.  If we can catch a dead DC before
	   performing a cli_connect() we can avoid a 30-second timeout. */

	DEBUG(3, ("rpc_dc_name: Returning DC %s (%s) for domain %s\n", srv_name,
		  inet_ntoa(dc_ip), domain));

	*ip_out = dc_ip;

	SAFE_FREE(ip_list);

	return True;
}

/**********************************************************************
 wrapper around ads and rpc methods of finds DC's
**********************************************************************/

BOOL get_dc_name(const char *domain, const char *realm, fstring srv_name, struct in_addr *ip_out)
{
	struct in_addr dc_ip;
	BOOL ret;
	BOOL our_domain = False;

	zero_ip(&dc_ip);

	ret = False;
	
	if ( strequal(lp_workgroup(), domain) || strequal(lp_realm(), realm) )
		our_domain = True;
	
	/* always try to obey what the admin specified in smb.conf 
	   (for the local domain) */
	
	if ( (our_domain && lp_security()==SEC_ADS) || realm ) {
		ret = ads_dc_name(domain, realm, &dc_ip, srv_name);
	}
	
	if (!ret) {
		/* fall back on rpc methods if the ADS methods fail */
		ret = rpc_dc_name(domain, srv_name, &dc_ip);
	}
		
	*ip_out = dc_ip;

	return ret;
}

