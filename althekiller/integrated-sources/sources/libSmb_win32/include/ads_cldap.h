/* 
   Samba Unix/Linux SMB client library 
   net ads cldap functions 
   Copyright (C) 2001 Andrew Tridgell (tridge@samba.org)
   Copyright (C) 2003 Jim McDonough (jmcd@us.ibm.com)

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

#define MAX_DNS_LABEL 255 + 1

struct cldap_netlogon_reply {
	uint32 type;
	uint32 flags;
	UUID_FLAT guid;

	char forest[MAX_DNS_LABEL];
	char domain[MAX_DNS_LABEL];
	char hostname[MAX_DNS_LABEL];

	char netbios_domain[MAX_DNS_LABEL];
	char netbios_hostname[MAX_DNS_LABEL];

	char unk[MAX_DNS_LABEL];
	char user_name[MAX_DNS_LABEL];
	char site_name[MAX_DNS_LABEL];
	char site_name_2[MAX_DNS_LABEL];

	uint32 version;
	uint16 lmnt_token;
	uint16 lm20_token;
};

/* Mailslot or cldap getdcname response flags */
#define ADS_PDC            0x00000001  /* DC is PDC */
#define ADS_GC             0x00000004  /* DC is a GC of forest */
#define ADS_LDAP           0x00000008  /* DC is an LDAP server */
#define ADS_DS             0x00000010  /* DC supports DS */
#define ADS_KDC            0x00000020  /* DC is running KDC */
#define ADS_TIMESERV       0x00000040  /* DC is running time services */
#define ADS_CLOSEST        0x00000080  /* DC is closest to client */
#define ADS_WRITABLE       0x00000100  /* DC has writable DS */
#define ADS_GOOD_TIMESERV  0x00000200  /* DC has hardware clock (and running time) */
#define ADS_NDNC           0x00000400  /* DomainName is non-domain NC serviced by LDAP server */


