/* 
   Samba Unix/Linux SMB client library 
   net lookup command
   Copyright (C) 2001 Andrew Tridgell (tridge@samba.org)

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "includes.h"
#include "utils/net.h"

int net_lookup_usage(int argc, const char **argv)
{
	d_printf(
"  net lookup [host] HOSTNAME[#<type>]\n\tgives IP for a hostname\n\n"
"  net lookup ldap [domain]\n\tgives IP of domain's ldap server\n\n"
"  net lookup kdc [realm]\n\tgives IP of realm's kerberos KDC\n\n"
"  net lookup dc [domain]\n\tgives IP of domains Domain Controllers\n\n"
"  net lookup master [domain|wg]\n\tgive IP of master browser\n\n"
"  net lookup name [name]\n\tLookup name's sid and type\n\n"
"  net lookup sid [sid]\n\tGive sid's name and type\n\n"
);
	return -1;
}

/* lookup a hostname giving an IP */
static int net_lookup_host(int argc, const char **argv)
{
	struct in_addr ip;
	int name_type = 0x20;
	const char *name = argv[0];
	char *p;

	if (argc == 0) 
		return net_lookup_usage(argc, argv);

	p = strchr_m(name,'#');
	if (p) {
		*p = '\0';
		sscanf(++p,"%x",&name_type);
	}
	
	if (!resolve_name(name, &ip, name_type)) {
		/* we deliberately use DEBUG() here to send it to stderr 
		   so scripts aren't mucked up */
		DEBUG(0,("Didn't find %s#%02x\n", name, name_type));
		return -1;
	}

	d_printf("%s\n", inet_ntoa(ip));
	return 0;
}

#ifdef HAVE_ADS
static void print_ldap_srvlist(struct dns_rr_srv *dclist, int numdcs )
{
	struct in_addr ip;
	int i;

	for ( i=0; i<numdcs; i++ ) {
		if ( resolve_name(dclist[i].hostname, &ip, 0x20) ) {
			d_printf("%s:%d\n", inet_ntoa(ip), dclist[i].port); 
		}
	}
}
#endif

static int net_lookup_ldap(int argc, const char **argv)
{
#ifdef HAVE_ADS
	const char *domain;
	struct in_addr addr;
	struct hostent *hostent;
	struct dns_rr_srv *dcs = NULL;
	int numdcs = 0;
	TALLOC_CTX *ctx;
	NTSTATUS status;

	if (argc > 0)
		domain = argv[0];
	else
		domain = opt_target_workgroup;

	if ( (ctx = talloc_init("net_lookup_ldap")) == NULL ) {
		d_fprintf(stderr, "net_lookup_ldap: talloc_inti() failed!\n");
		return -1;
	}

	DEBUG(9, ("Lookup up ldap for domain %s\n", domain));

	status = ads_dns_query_dcs( ctx, domain, &dcs, &numdcs );
	if ( NT_STATUS_IS_OK(status) && numdcs ) {
		print_ldap_srvlist(dcs, numdcs);
		TALLOC_FREE( ctx );

		return 0;
	}

     	DEBUG(9, ("Looking up DC for domain %s\n", domain));
	if (!get_pdc_ip(domain, &addr)) {
		TALLOC_FREE( ctx );
		return -1;
	}

	hostent = gethostbyaddr((char *) &addr.s_addr, sizeof(addr.s_addr),
				AF_INET);
	if (!hostent) {
		TALLOC_FREE( ctx );
		return -1;
	}

	DEBUG(9, ("Found DC with DNS name %s\n", hostent->h_name));
	domain = strchr(hostent->h_name, '.');
	if (!domain) {
		TALLOC_FREE( ctx );
		return -1;
	}
	domain++;

	DEBUG(9, ("Looking up ldap for domain %s\n", domain));

	status = ads_dns_query_dcs( ctx, domain, &dcs, &numdcs );
	if ( NT_STATUS_IS_OK(status) && numdcs ) {
		print_ldap_srvlist(dcs, numdcs);
		TALLOC_FREE( ctx );

		return 0;
	}

	TALLOC_FREE( ctx );


	return -1;
#endif
	DEBUG(1,("No ADS support\n"));
	return -1;
}

static int net_lookup_dc(int argc, const char **argv)
{
	struct ip_service *ip_list;
	struct in_addr addr;
	char *pdc_str = NULL;
	const char *domain=opt_target_workgroup;
	int count, i;

	if (argc > 0)
		domain=argv[0];

	/* first get PDC */
	if (!get_pdc_ip(domain, &addr))
		return -1;

	asprintf(&pdc_str, "%s", inet_ntoa(addr));
	d_printf("%s\n", pdc_str);

	if (!get_sorted_dc_list(domain, &ip_list, &count, False)) {
		SAFE_FREE(pdc_str);
		return 0;
	}
	for (i=0;i<count;i++) {
		char *dc_str = inet_ntoa(ip_list[i].ip);
		if (!strequal(pdc_str, dc_str))
			d_printf("%s\n", dc_str);
	}
	SAFE_FREE(pdc_str);
	return 0;
}

static int net_lookup_master(int argc, const char **argv)
{
	struct in_addr master_ip;
	const char *domain=opt_target_workgroup;

	if (argc > 0)
		domain=argv[0];

	if (!find_master_ip(domain, &master_ip))
		return -1;
	d_printf("%s\n", inet_ntoa(master_ip));
	return 0;
}

static int net_lookup_kdc(int argc, const char **argv)
{
#ifdef HAVE_KRB5
	krb5_error_code rc;
	krb5_context ctx;
	struct sockaddr_in *addrs;
	int num_kdcs,i;
	krb5_data realm;
	char **realms;

	initialize_krb5_error_table();
	rc = krb5_init_context(&ctx);
	if (rc) {
		DEBUG(1,("krb5_init_context failed (%s)\n", 
			 error_message(rc)));
		return -1;
	}

	if (argc>0) {
                realm.data = CONST_DISCARD(krb5_pointer, argv[0]);
		realm.length = strlen(argv[0]);
	} else if (lp_realm() && *lp_realm()) {
		realm.data = (krb5_pointer) lp_realm();
		realm.length = strlen(realm.data);
	} else {
		rc = krb5_get_host_realm(ctx, NULL, &realms);
		if (rc) {
			DEBUG(1,("krb5_gethost_realm failed (%s)\n",
				 error_message(rc)));
			return -1;
		}
		realm.data = (krb5_pointer) *realms;
		realm.length = strlen(realm.data);
	}

	rc = krb5_locate_kdc(ctx, &realm, (struct sockaddr **)(void *)&addrs, &num_kdcs, 0);
	if (rc) {
		DEBUG(1, ("krb5_locate_kdc failed (%s)\n", error_message(rc)));
		return -1;
	}
	for (i=0;i<num_kdcs;i++)
		if (addrs[i].sin_family == AF_INET) 
			d_printf("%s:%hd\n", inet_ntoa(addrs[i].sin_addr),
				 ntohs(addrs[i].sin_port));
	return 0;

#endif	
	DEBUG(1, ("No kerberos support\n"));
	return -1;
}

static int net_lookup_name(int argc, const char **argv)
{
	const char *dom, *name;
	DOM_SID sid;
	enum SID_NAME_USE type;

	if (argc != 1) {
		d_printf("usage: net lookup name <name>\n");
		return -1;
	}

	if (!lookup_name(tmp_talloc_ctx(), argv[0], LOOKUP_NAME_ALL,
			 &dom, &name, &sid, &type)) {
		d_printf("Could not lookup name %s\n", argv[0]);
		return -1;
	}

	d_printf("%s %d (%s) %s\\%s\n", sid_string_static(&sid),
		 type, sid_type_lookup(type), dom, name);
	return 0;
}

static int net_lookup_sid(int argc, const char **argv)
{
	const char *dom, *name;
	DOM_SID sid;
	enum SID_NAME_USE type;

	if (argc != 1) {
		d_printf("usage: net lookup sid <sid>\n");
		return -1;
	}

	if (!string_to_sid(&sid, argv[0])) {
		d_printf("Could not convert %s to SID\n", argv[0]);
		return -1;
	}

	if (!lookup_sid(tmp_talloc_ctx(), &sid,
			&dom, &name, &type)) {
		d_printf("Could not lookup name %s\n", argv[0]);
		return -1;
	}

	d_printf("%s %d (%s) %s\\%s\n", sid_string_static(&sid),
		 type, sid_type_lookup(type), dom, name);
	return 0;
}

/* lookup hosts or IP addresses using internal samba lookup fns */
int net_lookup(int argc, const char **argv)
{
	int i;

	struct functable table[] = {
		{"HOST", net_lookup_host},
		{"LDAP", net_lookup_ldap},
		{"DC", net_lookup_dc},
		{"MASTER", net_lookup_master},
		{"KDC", net_lookup_kdc},
		{"NAME", net_lookup_name},
		{"SID", net_lookup_sid},
		{NULL, NULL}
	};

	if (argc < 1) {
		d_printf("\nUsage: \n");
		return net_lookup_usage(argc, argv);
	}
	for (i=0; table[i].funcname; i++) {
		if (StrCaseCmp(argv[0], table[i].funcname) == 0)
			return table[i].fn(argc-1, argv+1);
	}

	/* Default to lookup a hostname so 'net lookup foo#1b' can be 
	   used instead of 'net lookup host foo#1b'.  The host syntax
	   is a bit confusing as non #00 names can't really be 
	   considered hosts as such. */

	return net_lookup_host(argc, argv);
}
