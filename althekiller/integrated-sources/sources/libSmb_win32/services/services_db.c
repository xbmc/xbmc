/* 
 *  Unix SMB/CIFS implementation.
 *  Service Control API Implementation
 * 
 *  Copyright (C) Marcin Krzysztof Porwit         2005.
 *  Largely Rewritten by:
 *  Copyright (C) Gerald (Jerry) Carter           2005.
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

#include "includes.h"

struct rcinit_file_information {
	char *description;
};

struct service_display_info {
	const char *servicename;
	const char *daemon;
	const char *dispname;
	const char *description;
};

struct service_display_info builtin_svcs[] = {  
  { "Spooler",	      "smbd", 	"Print Spooler", "Internal service for spooling files to print devices" },
  { "NETLOGON",	      "smbd", 	"Net Logon", "File service providing access to policy and profile data (not remotely manageable)" },
  { "RemoteRegistry", "smbd", 	"Remote Registry Service", "Internal service providing remote access to "
				"the Samba registry" },
  { "WINS",           "nmbd", 	"Windows Internet Name Service (WINS)", "Internal service providing a "
				"NetBIOS point-to-point name server (not remotely manageable)" },
  { NULL, NULL, NULL, NULL }
};

struct service_display_info common_unix_svcs[] = {  
  { "cups",          NULL, "Common Unix Printing System","Provides unified printing support for all operating systems" },
  { "postfix",       NULL, "Internet Mail Service", 	"Provides support for sending and receiving electonic mail" },
  { "sendmail",      NULL, "Internet Mail Service", 	"Provides support for sending and receiving electonic mail" },
  { "portmap",       NULL, "TCP Port to RPC PortMapper",NULL },
  { "xinetd",        NULL, "Internet Meta-Daemon", 	NULL },
  { "inet",          NULL, "Internet Meta-Daemon", 	NULL },
  { "xntpd",         NULL, "Network Time Service", 	NULL },
  { "ntpd",          NULL, "Network Time Service", 	NULL },
  { "lpd",           NULL, "BSD Print Spooler", 	NULL },
  { "nfsserver",     NULL, "Network File Service", 	NULL },
  { "cron",          NULL, "Scheduling Service", 	NULL },
  { "at",            NULL, "Scheduling Service", 	NULL },
  { "nscd",          NULL, "Name Service Cache Daemon",	NULL },
  { "slapd",         NULL, "LDAP Directory Service", 	NULL },
  { "ldap",          NULL, "LDAP DIrectory Service", 	NULL },
  { "ypbind",        NULL, "NIS Directory Service", 	NULL },
  { "courier-imap",  NULL, "IMAP4 Mail Service", 	NULL },
  { "courier-pop3",  NULL, "POP3 Mail Service", 	NULL },
  { "named",         NULL, "Domain Name Service", 	NULL },
  { "bind",          NULL, "Domain Name Service", 	NULL },
  { "httpd",         NULL, "HTTP Server", 		NULL },
  { "apache",        NULL, "HTTP Server", 		"Provides s highly scalable and flexible web server "
							"capable of implementing various protocols incluing "
							"but not limited to HTTP" },
  { "autofs",        NULL, "Automounter", 		NULL },
  { "squid",         NULL, "Web Cache Proxy ",		NULL },
  { "perfcountd",    NULL, "Performance Monitoring Daemon", NULL },
  { "pgsql",	     NULL, "PgSQL Database Server", 	"Provides service for SQL database from Postgresql.org" },
  { "arpwatch",	     NULL, "ARP Tables watcher", 	"Provides service for monitoring ARP tables for changes" },
  { "dhcpd",	     NULL, "DHCP Server", 		"Provides service for dynamic host configuration and IP assignment" },
  { "nwserv",	     NULL, "NetWare Server Emulator", 	"Provides service for emulating Novell NetWare 3.12 server" },
  { "proftpd",	     NULL, "Professional FTP Server", 	"Provides high configurable service for FTP connection and "
							"file transferring" },
  { "ssh2",	     NULL, "SSH Secure Shell", 		"Provides service for secure connection for remote administration" },
  { "sshd",	     NULL, "SSH Secure Shell", 		"Provides service for secure connection for remote administration" },
  { NULL, NULL, NULL, NULL }
};


/********************************************************************
********************************************************************/

static SEC_DESC* construct_service_sd( TALLOC_CTX *ctx )
{
	SEC_ACE ace[4];	
	SEC_ACCESS mask;
	size_t i = 0;
	SEC_DESC *sd;
	SEC_ACL *acl;
	size_t sd_size;
	
	/* basic access for Everyone */
	
	init_sec_access(&mask, SERVICE_READ_ACCESS );
	init_sec_ace(&ace[i++], &global_sid_World, SEC_ACE_TYPE_ACCESS_ALLOWED, mask, 0);
		
	init_sec_access(&mask,SERVICE_EXECUTE_ACCESS );
	init_sec_ace(&ace[i++], &global_sid_Builtin_Power_Users, SEC_ACE_TYPE_ACCESS_ALLOWED, mask, 0);
	
	init_sec_access(&mask,SERVICE_ALL_ACCESS );
	init_sec_ace(&ace[i++], &global_sid_Builtin_Server_Operators, SEC_ACE_TYPE_ACCESS_ALLOWED, mask, 0);
	init_sec_ace(&ace[i++], &global_sid_Builtin_Administrators, SEC_ACE_TYPE_ACCESS_ALLOWED, mask, 0);
	
	/* create the security descriptor */
	
	if ( !(acl = make_sec_acl(ctx, NT4_ACL_REVISION, i, ace)) )
		return NULL;

	if ( !(sd = make_sec_desc(ctx, SEC_DESC_REVISION, SEC_DESC_SELF_RELATIVE, NULL, NULL, NULL, acl, &sd_size)) )
		return NULL;

	return sd;
}

/********************************************************************
 This is where we do the dirty work of filling in things like the
 Display name, Description, etc...
********************************************************************/

static char *get_common_service_dispname( const char *servicename )
{
	static fstring dispname;
	int i;
	
	for ( i=0; common_unix_svcs[i].servicename; i++ ) {
		if ( strequal( servicename, common_unix_svcs[i].servicename ) ) {
			fstr_sprintf( dispname, "%s (%s)", 
				common_unix_svcs[i].dispname,
				common_unix_svcs[i].servicename );
				
			return dispname;
		}
	} 
	
	fstrcpy( dispname, servicename );
	
	return dispname;
}

/********************************************************************
********************************************************************/

static char* cleanup_string( const char *string )
{
	static pstring clean;
	char *begin, *end;

	pstrcpy( clean, string );
	begin = clean;
	
	/* trim any beginning whilespace */
	
	while ( isspace(*begin) )
		begin++;

	if ( *begin == '\0' )
		return NULL;
			
	/* trim any trailing whitespace or carriage returns.
	   Start at the end and move backwards */
			
	end = begin + strlen(begin) - 1;
			
	while ( isspace(*end) || *end=='\n' || *end=='\r' ) {
		*end = '\0';
		end--;
	}

	return begin;
}

/********************************************************************
********************************************************************/

static BOOL read_init_file( const char *servicename, struct rcinit_file_information **service_info )
{
	struct rcinit_file_information *info;
	pstring filepath, str;
	XFILE *f;
	char *p;
		
	if ( !(info = TALLOC_ZERO_P( NULL, struct rcinit_file_information ) ) )
		return False;
	
	/* attempt the file open */
		
	pstr_sprintf( filepath, "%s/%s/%s", dyn_LIBDIR, SVCCTL_SCRIPT_DIR, servicename );
	if ( !(f = x_fopen( filepath, O_RDONLY, 0 )) ) {
		DEBUG(0,("read_init_file: failed to open [%s]\n", filepath));
		TALLOC_FREE(info);
		return False;
	}
	
	while ( (x_fgets( str, sizeof(str)-1, f )) != NULL ) {
		/* ignore everything that is not a full line 
		   comment starting with a '#' */
		   
		if ( str[0] != '#' )
			continue;
		
		/* Look for a line like '^#.*Description:' */
		
		if ( (p = strstr( str, "Description:" )) != NULL ) {
			char *desc;

			p += strlen( "Description:" ) + 1;
			if ( !p ) 
				break;
				
			if ( (desc = cleanup_string(p)) != NULL )
				info->description = talloc_strdup( info, desc );
		}
	}
	
	x_fclose( f );
	
	if ( !info->description )
		info->description = talloc_strdup( info, "External Unix Service" );
	
	*service_info = info;
	
	return True;
}

/********************************************************************
 This is where we do the dirty work of filling in things like the
 Display name, Description, etc...
********************************************************************/

static void fill_service_values( const char *name, REGVAL_CTR *values )
{
	UNISTR2 data, dname, ipath, description;
	uint32 dword;
	pstring pstr;
	int i;
	
	/* These values are hardcoded in all QueryServiceConfig() replies.
	   I'm just storing them here for cosmetic purposes */
	
	dword = SVCCTL_AUTO_START;
	regval_ctr_addvalue( values, "Start", REG_DWORD, (char*)&dword, sizeof(uint32));
	
	dword = SVCCTL_WIN32_OWN_PROC;
	regval_ctr_addvalue( values, "Type", REG_DWORD, (char*)&dword, sizeof(uint32));

	dword = SVCCTL_SVC_ERROR_NORMAL;
	regval_ctr_addvalue( values, "ErrorControl", REG_DWORD, (char*)&dword, sizeof(uint32));
	
	/* everything runs as LocalSystem */
	
	init_unistr2( &data, "LocalSystem", UNI_STR_TERMINATE );
	regval_ctr_addvalue( values, "ObjectName", REG_SZ, (char*)data.buffer, data.uni_str_len*2);
	
	/* special considerations for internal services and the DisplayName value */
	
	for ( i=0; builtin_svcs[i].servicename; i++ ) {
		if ( strequal( name, builtin_svcs[i].servicename ) ) {
			pstr_sprintf( pstr, "%s/%s/%s",dyn_LIBDIR, SVCCTL_SCRIPT_DIR, builtin_svcs[i].daemon );
			init_unistr2( &ipath, pstr, UNI_STR_TERMINATE );
			init_unistr2( &description, builtin_svcs[i].description, UNI_STR_TERMINATE );
			init_unistr2( &dname, builtin_svcs[i].dispname, UNI_STR_TERMINATE );
			break;
		}
	} 
	
	/* default to an external service if we haven't found a match */
	
	if ( builtin_svcs[i].servicename == NULL ) {
		struct rcinit_file_information *init_info = NULL;

		pstr_sprintf( pstr, "%s/%s/%s",dyn_LIBDIR, SVCCTL_SCRIPT_DIR, name );
		init_unistr2( &ipath, pstr, UNI_STR_TERMINATE );
		
		/* lookup common unix display names */
		init_unistr2( &dname, get_common_service_dispname( name ), UNI_STR_TERMINATE );

		/* get info from init file itself */		
		if ( read_init_file( name, &init_info ) ) {
			init_unistr2( &description, init_info->description, UNI_STR_TERMINATE );
			TALLOC_FREE( init_info );
		}
		else {
			init_unistr2( &description, "External Unix Service", UNI_STR_TERMINATE );
		}
	}
	
	/* add the new values */
	
	regval_ctr_addvalue( values, "DisplayName", REG_SZ, (char*)dname.buffer, dname.uni_str_len*2);
	regval_ctr_addvalue( values, "ImagePath", REG_SZ, (char*)ipath.buffer, ipath.uni_str_len*2);
	regval_ctr_addvalue( values, "Description", REG_SZ, (char*)description.buffer, description.uni_str_len*2);
	
	return;
}

/********************************************************************
********************************************************************/

static void add_new_svc_name( REGISTRY_KEY *key_parent, REGSUBKEY_CTR *subkeys, 
                              const char *name )
{
	REGISTRY_KEY *key_service, *key_secdesc;
	WERROR wresult;
	pstring path;
	REGVAL_CTR *values;
	REGSUBKEY_CTR *svc_subkeys;
	SEC_DESC *sd;
	prs_struct ps;

	/* add to the list and create the subkey path */

	regsubkey_ctr_addkey( subkeys, name );
	store_reg_keys( key_parent, subkeys );

	/* open the new service key */

	pstr_sprintf( path, "%s\\%s", KEY_SERVICES, name );
	wresult = regkey_open_internal( &key_service, path, get_root_nt_token(), 
		REG_KEY_ALL );
	if ( !W_ERROR_IS_OK(wresult) ) {
		DEBUG(0,("add_new_svc_name: key lookup failed! [%s] (%s)\n", 
			path, dos_errstr(wresult)));
		return;
	}
	
	/* add the 'Security' key */

	if ( !(svc_subkeys = TALLOC_ZERO_P( key_service, REGSUBKEY_CTR )) ) {
		DEBUG(0,("add_new_svc_name: talloc() failed!\n"));
		regkey_close_internal( key_service );
		return;
	}
	
	fetch_reg_keys( key_service, svc_subkeys );
	regsubkey_ctr_addkey( svc_subkeys, "Security" );
	store_reg_keys( key_service, svc_subkeys );

	/* now for the service values */
	
	if ( !(values = TALLOC_ZERO_P( key_service, REGVAL_CTR )) ) {
		DEBUG(0,("add_new_svc_name: talloc() failed!\n"));
		regkey_close_internal( key_service );
		return;
	}

	fill_service_values( name, values );
	store_reg_values( key_service, values );

	/* cleanup the service key*/

	regkey_close_internal( key_service );

	/* now add the security descriptor */

	pstr_sprintf( path, "%s\\%s\\%s", KEY_SERVICES, name, "Security" );
	wresult = regkey_open_internal( &key_secdesc, path, get_root_nt_token(), 
		REG_KEY_ALL );
	if ( !W_ERROR_IS_OK(wresult) ) {
		DEBUG(0,("add_new_svc_name: key lookup failed! [%s] (%s)\n", 
			path, dos_errstr(wresult)));
		regkey_close_internal( key_secdesc );
		return;
	}

	if ( !(values = TALLOC_ZERO_P( key_secdesc, REGVAL_CTR )) ) {
		DEBUG(0,("add_new_svc_name: talloc() failed!\n"));
		regkey_close_internal( key_secdesc );
		return;
	}

	if ( !(sd = construct_service_sd(key_secdesc)) ) {
		DEBUG(0,("add_new_svc_name: Failed to create default sec_desc!\n"));
		regkey_close_internal( key_secdesc );
		return;
	}
	
	/* stream the printer security descriptor */
	
	prs_init( &ps, RPC_MAX_PDU_FRAG_LEN, key_secdesc, MARSHALL);
	
	if ( sec_io_desc("sec_desc", &sd, &ps, 0 ) ) {
		uint32 offset = prs_offset( &ps );
		regval_ctr_addvalue( values, "Security", REG_BINARY, prs_data_p(&ps), offset );
		store_reg_values( key_secdesc, values );
	}
	
	/* finally cleanup the Security key */
	
	prs_mem_free( &ps );
	regkey_close_internal( key_secdesc );

	return;
}

/********************************************************************
********************************************************************/

void svcctl_init_keys( void )
{
	const char **service_list = lp_svcctl_list();
	int i;
	REGSUBKEY_CTR *subkeys;
	REGISTRY_KEY *key = NULL;
	WERROR wresult;
	
	/* bad mojo here if the lookup failed.  Should not happen */
	
	wresult = regkey_open_internal( &key, KEY_SERVICES, get_root_nt_token(), 
		REG_KEY_ALL );

	if ( !W_ERROR_IS_OK(wresult) ) {
		DEBUG(0,("init_services_keys: key lookup failed! (%s)\n", 
			dos_errstr(wresult)));
		return;
	}
	
	/* lookup the available subkeys */	
	
	if ( !(subkeys = TALLOC_ZERO_P( key, REGSUBKEY_CTR )) ) {
		DEBUG(0,("init_services_keys: talloc() failed!\n"));
		regkey_close_internal( key );
		return;
	}
	
	fetch_reg_keys( key, subkeys );
	
	/* the builting services exist */
	
	for ( i=0; builtin_svcs[i].servicename; i++ )
		add_new_svc_name( key, subkeys, builtin_svcs[i].servicename );
		
	for ( i=0; service_list && service_list[i]; i++ ) {
	
		/* only add new services */
		if ( regsubkey_ctr_key_exists( subkeys, service_list[i] ) )
			continue;

		/* Add the new service key and initialize the appropriate values */

		add_new_svc_name( key, subkeys, service_list[i] );
	}

	regkey_close_internal( key );

	/* initialize the control hooks */

	init_service_op_table();

	return;
}

/********************************************************************
 This is where we do the dirty work of filling in things like the
 Display name, Description, etc...Always return a default secdesc 
 in case of any failure.
********************************************************************/

SEC_DESC* svcctl_get_secdesc( TALLOC_CTX *ctx, const char *name, NT_USER_TOKEN *token )
{
	REGISTRY_KEY *key;
	prs_struct ps;
	REGVAL_CTR *values;
	REGISTRY_VALUE *val;
	SEC_DESC *sd = NULL;
	SEC_DESC *ret_sd = NULL;
	pstring path;
	WERROR wresult;
	
	/* now add the security descriptor */

	pstr_sprintf( path, "%s\\%s\\%s", KEY_SERVICES, name, "Security" );
	wresult = regkey_open_internal( &key, path, token, REG_KEY_ALL );
	if ( !W_ERROR_IS_OK(wresult) ) {
		DEBUG(0,("svcctl_get_secdesc: key lookup failed! [%s] (%s)\n", 
			path, dos_errstr(wresult)));
		return NULL;
	}

	if ( !(values = TALLOC_ZERO_P( key, REGVAL_CTR )) ) {
		DEBUG(0,("add_new_svc_name: talloc() failed!\n"));
		regkey_close_internal( key );
		return NULL;
	}

	fetch_reg_values( key, values );
	
	if ( !(val = regval_ctr_getvalue( values, "Security" )) ) {
		DEBUG(6,("svcctl_get_secdesc: constructing default secdesc for service [%s]\n", 
			name));
		regkey_close_internal( key );
		return construct_service_sd( ctx );
	}
	

	/* stream the printer security descriptor */
	
	prs_init( &ps, 0, key, UNMARSHALL);
	prs_give_memory( &ps, (char *)regval_data_p(val), regval_size(val), False );
	
	if ( !sec_io_desc("sec_desc", &sd, &ps, 0 ) ) {
		regkey_close_internal( key );
		return construct_service_sd( ctx );
	}
	
	ret_sd = dup_sec_desc( ctx, sd );
	
	/* finally cleanup the Security key */
	
	prs_mem_free( &ps );
	regkey_close_internal( key );

	return ret_sd;
}

/********************************************************************
 Wrapper to make storing a Service sd easier
********************************************************************/

BOOL svcctl_set_secdesc( TALLOC_CTX *ctx, const char *name, SEC_DESC *sec_desc, NT_USER_TOKEN *token )
{
	REGISTRY_KEY *key;
	WERROR wresult;
	pstring path;
	REGVAL_CTR *values;
	prs_struct ps;
	BOOL ret = False;
	
	/* now add the security descriptor */

	pstr_sprintf( path, "%s\\%s\\%s", KEY_SERVICES, name, "Security" );
	wresult = regkey_open_internal( &key, path, token, REG_KEY_ALL );
	if ( !W_ERROR_IS_OK(wresult) ) {
		DEBUG(0,("svcctl_get_secdesc: key lookup failed! [%s] (%s)\n", 
			path, dos_errstr(wresult)));
		return False;
	}

	if ( !(values = TALLOC_ZERO_P( key, REGVAL_CTR )) ) {
		DEBUG(0,("add_new_svc_name: talloc() failed!\n"));
		regkey_close_internal( key );
		return False;
	}
	
	/* stream the printer security descriptor */
	
	prs_init( &ps, RPC_MAX_PDU_FRAG_LEN, key, MARSHALL);
	
	if ( sec_io_desc("sec_desc", &sec_desc, &ps, 0 ) ) {
		uint32 offset = prs_offset( &ps );
		regval_ctr_addvalue( values, "Security", REG_BINARY, prs_data_p(&ps), offset );
		ret = store_reg_values( key, values );
	}
	
	/* cleanup */
	
	prs_mem_free( &ps );
	regkey_close_internal( key);

	return ret;
}

/********************************************************************
********************************************************************/

char* svcctl_lookup_dispname( const char *name, NT_USER_TOKEN *token )
{
	static fstring display_name;
	REGISTRY_KEY *key;
	REGVAL_CTR *values;
	REGISTRY_VALUE *val;
	pstring path;
	WERROR wresult;
	
	/* now add the security descriptor */

	pstr_sprintf( path, "%s\\%s", KEY_SERVICES, name );
	wresult = regkey_open_internal( &key, path, token, REG_KEY_READ );
	if ( !W_ERROR_IS_OK(wresult) ) {
		DEBUG(0,("svcctl_lookup_dispname: key lookup failed! [%s] (%s)\n", 
			path, dos_errstr(wresult)));
		goto fail;
	}

	if ( !(values = TALLOC_ZERO_P( key, REGVAL_CTR )) ) {
		DEBUG(0,("svcctl_lookup_dispname: talloc() failed!\n"));
		regkey_close_internal( key );
		goto fail;
	}

	fetch_reg_values( key, values );
	
	if ( !(val = regval_ctr_getvalue( values, "DisplayName" )) )
		goto fail;

	rpcstr_pull( display_name, regval_data_p(val), sizeof(display_name), regval_size(val), 0 );

	regkey_close_internal( key );
	
	return display_name;

fail:
	/* default to returning the service name */
	regkey_close_internal( key );
	fstrcpy( display_name, name );
	return display_name;
}

/********************************************************************
********************************************************************/

char* svcctl_lookup_description( const char *name, NT_USER_TOKEN *token )
{
	static fstring description;
	REGISTRY_KEY *key;
	REGVAL_CTR *values;
	REGISTRY_VALUE *val;
	pstring path;
	WERROR wresult;
	
	/* now add the security descriptor */

	pstr_sprintf( path, "%s\\%s", KEY_SERVICES, name );
	wresult = regkey_open_internal( &key, path, token, REG_KEY_READ );
	if ( !W_ERROR_IS_OK(wresult) ) {
		DEBUG(0,("svcctl_lookup_dispname: key lookup failed! [%s] (%s)\n", 
			path, dos_errstr(wresult)));
		return NULL;
	}

	if ( !(values = TALLOC_ZERO_P( key, REGVAL_CTR )) ) {
		DEBUG(0,("svcctl_lookup_dispname: talloc() failed!\n"));
		regkey_close_internal( key );
		return NULL;
	}

	fetch_reg_values( key, values );
	
	if ( !(val = regval_ctr_getvalue( values, "Description" )) )
		fstrcpy( description, "Unix Service");
	else
		rpcstr_pull( description, regval_data_p(val), sizeof(description), regval_size(val), 0 );

	regkey_close_internal( key );
	
	return description;
}


/********************************************************************
********************************************************************/

REGVAL_CTR* svcctl_fetch_regvalues( const char *name, NT_USER_TOKEN *token )
{
	REGISTRY_KEY *key;
	REGVAL_CTR *values;
	pstring path;
	WERROR wresult;
	
	/* now add the security descriptor */

	pstr_sprintf( path, "%s\\%s", KEY_SERVICES, name );
	wresult = regkey_open_internal( &key, path, token, REG_KEY_READ );
	if ( !W_ERROR_IS_OK(wresult) ) {
		DEBUG(0,("svcctl_fetch_regvalues: key lookup failed! [%s] (%s)\n", 
			path, dos_errstr(wresult)));
		return NULL;
	}

	if ( !(values = TALLOC_ZERO_P( NULL, REGVAL_CTR )) ) {
		DEBUG(0,("svcctl_fetch_regvalues: talloc() failed!\n"));
		regkey_close_internal( key );
		return NULL;
	}
	
	fetch_reg_values( key, values );

	regkey_close_internal( key );
	
	return values;
}

