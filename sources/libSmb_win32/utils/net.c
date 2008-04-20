/* 
   Samba Unix/Linux SMB client library 
   Distributed SMB/CIFS Server Management Utility 
   Copyright (C) 2001 Steve French  (sfrench@us.ibm.com)
   Copyright (C) 2001 Jim McDonough (jmcd@us.ibm.com)
   Copyright (C) 2001 Andrew Tridgell (tridge@samba.org)
   Copyright (C) 2001 Andrew Bartlett (abartlet@samba.org)

   Originally written by Steve and Jim. Largely rewritten by tridge in
   November 2001.

   Reworked again by abartlet in December 2001

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
 
/*****************************************************/
/*                                                   */
/*   Distributed SMB/CIFS Server Management Utility  */
/*                                                   */
/*   The intent was to make the syntax similar       */
/*   to the NET utility (first developed in DOS      */
/*   with additional interesting & useful functions  */
/*   added in later SMB server network operating     */
/*   systems).                                       */
/*                                                   */
/*****************************************************/

#include "includes.h"
#include "utils/net.h"

/***********************************************************************/
/* Beginning of internationalization section.  Translatable constants  */
/* should be kept in this area and referenced in the rest of the code. */
/*                                                                     */
/* No functions, outside of Samba or LSB (Linux Standards Base) should */
/* be used (if possible).                                              */
/***********************************************************************/

#define YES_STRING              "Yes"
#define NO_STRING               "No"

/************************************************************************************/
/*                       end of internationalization section                        */
/************************************************************************************/

/* Yes, these buggers are globals.... */
const char *opt_requester_name = NULL;
const char *opt_host = NULL; 
const char *opt_password = NULL;
const char *opt_user_name = NULL;
BOOL opt_user_specified = False;
const char *opt_workgroup = NULL;
int opt_long_list_entries = 0;
int opt_reboot = 0;
int opt_force = 0;
int opt_stdin = 0;
int opt_port = 0;
int opt_verbose = 0;
int opt_maxusers = -1;
const char *opt_comment = "";
const char *opt_container = NULL;
int opt_flags = -1;
int opt_timeout = 0;
const char *opt_target_workgroup = NULL;
int opt_machine_pass = 0;
BOOL opt_localgroup = False;
BOOL opt_domaingroup = False;
const char *opt_newntname = "";
int opt_rid = 0;
int opt_acls = 0;
int opt_attrs = 0;
int opt_timestamps = 0;
const char *opt_exclude = NULL;
const char *opt_destination = NULL;

BOOL opt_have_ip = False;
struct in_addr opt_dest_ip;

extern struct in_addr loopback_ip;
extern BOOL AllowDebugChange;

uint32 get_sec_channel_type(const char *param) 
{
	if (!(param && *param)) {
		return get_default_sec_channel();
	} else {
		if (strequal(param, "PDC")) {
			return SEC_CHAN_BDC;
		} else if (strequal(param, "BDC")) {
			return SEC_CHAN_BDC;
		} else if (strequal(param, "MEMBER")) {
			return SEC_CHAN_WKSTA;
#if 0			
		} else if (strequal(param, "DOMAIN")) {
			return SEC_CHAN_DOMAIN;
#endif
		} else {
			return get_default_sec_channel();
		}
	}
}

/*
  run a function from a function table. If not found then
  call the specified usage function 
*/
int net_run_function(int argc, const char **argv, struct functable *table, 
		     int (*usage_fn)(int argc, const char **argv))
{
	int i;
	
	if (argc < 1) {
		d_printf("\nUsage: \n");
		return usage_fn(argc, argv);
	}
	for (i=0; table[i].funcname; i++) {
		if (StrCaseCmp(argv[0], table[i].funcname) == 0)
			return table[i].fn(argc-1, argv+1);
	}
	d_fprintf(stderr, "No command: %s\n", argv[0]);
	return usage_fn(argc, argv);
}

/*
 * run a function from a function table.
 */
int net_run_function2(int argc, const char **argv, const char *whoami,
		      struct functable2 *table)
{
	int i;

	if (argc != 0) {
		for (i=0; table[i].funcname; i++) {
			if (StrCaseCmp(argv[0], table[i].funcname) == 0)
				return table[i].fn(argc-1, argv+1);
		}
	}

	for (i=0; table[i].funcname != NULL; i++) {
		d_printf("%s %-15s %s\n", whoami, table[i].funcname,
			 table[i].helptext);
	}

	return -1;
}

/****************************************************************************
connect to \\server\service 
****************************************************************************/
NTSTATUS connect_to_service(struct cli_state **c, struct in_addr *server_ip,
					const char *server_name, 
					const char *service_name, 
					const char *service_type)
{
	NTSTATUS nt_status;

	if (!opt_password && !opt_machine_pass) {
		char *pass = getpass("Password:");
		if (pass) {
			opt_password = SMB_STRDUP(pass);
		}
	}
	
	nt_status = cli_full_connection(c, NULL, server_name, 
					server_ip, opt_port,
					service_name, service_type,  
					opt_user_name, opt_workgroup,
					opt_password, 0, Undefined, NULL);
	
	if (NT_STATUS_IS_OK(nt_status)) {
		return nt_status;
	} else {
		d_fprintf(stderr, "Could not connect to server %s\n", server_name);

		/* Display a nicer message depending on the result */

		if (NT_STATUS_V(nt_status) == 
		    NT_STATUS_V(NT_STATUS_LOGON_FAILURE))
			d_fprintf(stderr, "The username or password was not correct.\n");

		if (NT_STATUS_V(nt_status) == 
		    NT_STATUS_V(NT_STATUS_ACCOUNT_LOCKED_OUT))
			d_fprintf(stderr, "The account was locked out.\n");

		if (NT_STATUS_V(nt_status) == 
		    NT_STATUS_V(NT_STATUS_ACCOUNT_DISABLED))
			d_fprintf(stderr, "The account was disabled.\n");

		return nt_status;
	}
}


/****************************************************************************
connect to \\server\ipc$  
****************************************************************************/
NTSTATUS connect_to_ipc(struct cli_state **c, struct in_addr *server_ip,
					const char *server_name)
{
	return connect_to_service(c, server_ip, server_name, "IPC$", "IPC");
}

/****************************************************************************
connect to \\server\ipc$ anonymously
****************************************************************************/
NTSTATUS connect_to_ipc_anonymous(struct cli_state **c,
			struct in_addr *server_ip, const char *server_name)
{
	NTSTATUS nt_status;

	nt_status = cli_full_connection(c, opt_requester_name, server_name, 
					server_ip, opt_port,
					"IPC$", "IPC",  
					"", "",
					"", 0, Undefined, NULL);
	
	if (NT_STATUS_IS_OK(nt_status)) {
		return nt_status;
	} else {
		DEBUG(1,("Cannot connect to server (anonymously).  Error was %s\n", nt_errstr(nt_status)));
		return nt_status;
	}
}

/****************************************************************************
connect to \\server\ipc$ using KRB5
****************************************************************************/
NTSTATUS connect_to_ipc_krb5(struct cli_state **c,
			struct in_addr *server_ip, const char *server_name)
{
	NTSTATUS nt_status;

	nt_status = cli_full_connection(c, NULL, server_name, 
					server_ip, opt_port,
					"IPC$", "IPC",  
					opt_user_name, opt_workgroup,
					opt_password, CLI_FULL_CONNECTION_USE_KERBEROS, 
					Undefined, NULL);
	
	if (NT_STATUS_IS_OK(nt_status)) {
		return nt_status;
	} else {
		DEBUG(1,("Cannot connect to server using kerberos.  Error was %s\n", nt_errstr(nt_status)));
		return nt_status;
	}
}

/**
 * Connect a server and open a given pipe
 *
 * @param cli_dst		A cli_state 
 * @param pipe			The pipe to open
 * @param got_pipe		boolean that stores if we got a pipe
 *
 * @return Normal NTSTATUS return.
 **/
NTSTATUS connect_dst_pipe(struct cli_state **cli_dst, struct rpc_pipe_client **pp_pipe_hnd, int pipe_num)
{
	NTSTATUS nt_status;
	char *server_name = SMB_STRDUP("127.0.0.1");
	struct cli_state *cli_tmp = NULL;
	struct rpc_pipe_client *pipe_hnd = NULL;

	if (server_name == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	if (opt_destination) {
		SAFE_FREE(server_name);
		if ((server_name = SMB_STRDUP(opt_destination)) == NULL) {
			return NT_STATUS_NO_MEMORY;
		}
	}

	/* make a connection to a named pipe */
	nt_status = connect_to_ipc(&cli_tmp, NULL, server_name);
	if (!NT_STATUS_IS_OK(nt_status)) {
		SAFE_FREE(server_name);
		return nt_status;
	}

	pipe_hnd = cli_rpc_pipe_open_noauth(cli_tmp, pipe_num, &nt_status);
	if (!pipe_hnd) {
		DEBUG(0, ("couldn't not initialize pipe\n"));
		cli_shutdown(cli_tmp);
		SAFE_FREE(server_name);
		return nt_status;
	}

	*cli_dst = cli_tmp;
	*pp_pipe_hnd = pipe_hnd;
	SAFE_FREE(server_name);

	return nt_status;
}

/****************************************************************************
 Use the local machine's password for this session.
****************************************************************************/

int net_use_machine_password(void) 
{
	char *user_name = NULL;

	if (!secrets_init()) {
		d_fprintf(stderr, "ERROR: Unable to open secrets database\n");
		exit(1);
	}

	user_name = NULL;
	opt_password = secrets_fetch_machine_password(opt_target_workgroup, NULL, NULL);
	if (asprintf(&user_name, "%s$@%s", global_myname(), lp_realm()) == -1) {
		return -1;
	}
	opt_user_name = user_name;
	return 0;
}

BOOL net_find_server(const char *domain, unsigned flags, struct in_addr *server_ip, char **server_name)
{
	const char *d = domain ? domain : opt_target_workgroup;

	if (opt_host) {
		*server_name = SMB_STRDUP(opt_host);
	}		

	if (opt_have_ip) {
		*server_ip = opt_dest_ip;
		if (!*server_name) {
			*server_name = SMB_STRDUP(inet_ntoa(opt_dest_ip));
		}
	} else if (*server_name) {
		/* resolve the IP address */
		if (!resolve_name(*server_name, server_ip, 0x20))  {
			DEBUG(1,("Unable to resolve server name\n"));
			return False;
		}
	} else if (flags & NET_FLAGS_PDC) {
		struct in_addr pdc_ip;

		if (get_pdc_ip(d, &pdc_ip)) {
			fstring dc_name;
			
			if (is_zero_ip(pdc_ip))
				return False;
			
			if ( !name_status_find(d, 0x1b, 0x20, pdc_ip, dc_name) )
				return False;
				
			*server_name = SMB_STRDUP(dc_name);
			*server_ip = pdc_ip;
		}
	} else if (flags & NET_FLAGS_DMB) {
		struct in_addr msbrow_ip;
		/*  if (!resolve_name(MSBROWSE, &msbrow_ip, 1)) */
		if (!resolve_name(d, &msbrow_ip, 0x1B))  {
			DEBUG(1,("Unable to resolve domain browser via name lookup\n"));
			return False;
		} else {
			*server_ip = msbrow_ip;
		}
		*server_name = SMB_STRDUP(inet_ntoa(opt_dest_ip));
	} else if (flags & NET_FLAGS_MASTER) {
		struct in_addr brow_ips;
		if (!resolve_name(d, &brow_ips, 0x1D))  {
				/* go looking for workgroups */
			DEBUG(1,("Unable to resolve master browser via name lookup\n"));
			return False;
		} else {
			*server_ip = brow_ips;
		}
		*server_name = SMB_STRDUP(inet_ntoa(opt_dest_ip));
	} else if (!(flags & NET_FLAGS_LOCALHOST_DEFAULT_INSANE)) {
		*server_ip = loopback_ip;
		*server_name = SMB_STRDUP("127.0.0.1");
	}

	if (!server_name || !*server_name) {
		DEBUG(1,("no server to connect to\n"));
		return False;
	}

	return True;
}


BOOL net_find_pdc(struct in_addr *server_ip, fstring server_name, const char *domain_name)
{
	if (get_pdc_ip(domain_name, server_ip)) {
		if (is_zero_ip(*server_ip))
			return False;
		
		if (!name_status_find(domain_name, 0x1b, 0x20, *server_ip, server_name))
			return False;
			
		return True;	
	} 
	else
		return False;
}

struct cli_state *net_make_ipc_connection( unsigned flags )
{
	return net_make_ipc_connection_ex( NULL, NULL, NULL, flags );
}

struct cli_state *net_make_ipc_connection_ex( const char *domain, const char *server,
                                              struct in_addr *ip, unsigned flags)
{
	char *server_name = NULL;
	struct in_addr server_ip;
	struct cli_state *cli = NULL;
	NTSTATUS nt_status;

	if ( !server || !ip ) {
		if (!net_find_server(domain, flags, &server_ip, &server_name)) {
			d_fprintf(stderr, "Unable to find a suitable server\n");
			return NULL;
		}
	} else {
		server_name = SMB_STRDUP( server );
		server_ip = *ip;
	}

	if (flags & NET_FLAGS_ANONYMOUS) {
		nt_status = connect_to_ipc_anonymous(&cli, &server_ip, server_name);
	} else {
		nt_status = connect_to_ipc(&cli, &server_ip, server_name);
	}

	/* store the server in the affinity cache if it was a PDC */

	if ( (flags & NET_FLAGS_PDC) && NT_STATUS_IS_OK(nt_status) )
		saf_store( cli->server_domain, cli->desthost );

	SAFE_FREE(server_name);
	if (NT_STATUS_IS_OK(nt_status)) {
		return cli;
	} else {
		d_fprintf(stderr, "Connection failed: %s\n",
			  nt_errstr(nt_status));
		return NULL;
	}
}

static int net_user(int argc, const char **argv)
{
	if (net_ads_check() == 0)
		return net_ads_user(argc, argv);

	/* if server is not specified, default to PDC? */
	if (net_rpc_check(NET_FLAGS_PDC))
		return net_rpc_user(argc, argv);

	return net_rap_user(argc, argv);
}

static int net_group(int argc, const char **argv)
{
	if (net_ads_check() == 0)
		return net_ads_group(argc, argv);

	if (argc == 0 && net_rpc_check(NET_FLAGS_PDC))
		return net_rpc_group(argc, argv);

	return net_rap_group(argc, argv);
}

static int net_join(int argc, const char **argv)
{
	if (net_ads_check() == 0) {
		if (net_ads_join(argc, argv) == 0)
			return 0;
		else
			d_fprintf(stderr, "ADS join did not work, falling back to RPC...\n");
	}
	return net_rpc_join(argc, argv);
}

static int net_changetrustpw(int argc, const char **argv)
{
	if (net_ads_check() == 0)
		return net_ads_changetrustpw(argc, argv);

	return net_rpc_changetrustpw(argc, argv);
}

static void set_line_buffering(FILE *f)
{
	setvbuf(f, NULL, _IOLBF, 0);
}

static int net_changesecretpw(int argc, const char **argv)
{
        char *trust_pw;
        uint32 sec_channel_type = SEC_CHAN_WKSTA;

	if(opt_force) {
		if (opt_stdin) {
			set_line_buffering(stdin);
			set_line_buffering(stdout);
			set_line_buffering(stderr);
		}
		
		trust_pw = get_pass("Enter machine password: ", opt_stdin);

		if (!secrets_store_machine_password(trust_pw, lp_workgroup(), sec_channel_type)) {
			    d_fprintf(stderr, "Unable to write the machine account password in the secrets database");
			    return 1;
		}
		else {
		    d_printf("Modified trust account password in secrets database\n");
		}
	}
	else {
		d_printf("Machine account password change requires the -f flag.\n");
		d_printf("Do NOT use this function unless you know what it does!\n");
		d_printf("This function will change the ADS Domain member machine account password in the secrets.tdb file!\n");
	}

        return 0;
}

static int net_share(int argc, const char **argv)
{
	if (net_rpc_check(0))
		return net_rpc_share(argc, argv);
	return net_rap_share(argc, argv);
}

static int net_file(int argc, const char **argv)
{
	if (net_rpc_check(0))
		return net_rpc_file(argc, argv);
	return net_rap_file(argc, argv);
}

/*
 Retrieve our local SID or the SID for the specified name
 */
static int net_getlocalsid(int argc, const char **argv)
{
        DOM_SID sid;
	const char *name;
	fstring sid_str;

	if (argc >= 1) {
		name = argv[0];
        }
	else {
		name = global_myname();
	}

	if(!initialize_password_db(False)) {
		DEBUG(0, ("WARNING: Could not open passdb - local sid may not reflect passdb\n"
			  "backend knowlege (such as the sid stored in LDAP)\n"));
	}

	/* first check to see if we can even access secrets, so we don't
	   panic when we can't. */

	if (!secrets_init()) {
		d_fprintf(stderr, "Unable to open secrets.tdb.  Can't fetch domain SID for name: %s\n", name);
		return 1;
	}

	/* Generate one, if it doesn't exist */
	get_global_sam_sid();

	if (!secrets_fetch_domain_sid(name, &sid)) {
		DEBUG(0, ("Can't fetch domain SID for name: %s\n", name));
		return 1;
	}
	sid_to_string(sid_str, &sid);
	d_printf("SID for domain %s is: %s\n", name, sid_str);
	return 0;
}

static int net_setlocalsid(int argc, const char **argv)
{
	DOM_SID sid;

	if ( (argc != 1)
	     || (strncmp(argv[0], "S-1-5-21-", strlen("S-1-5-21-")) != 0)
	     || (!string_to_sid(&sid, argv[0]))
	     || (sid.num_auths != 4)) {
		d_printf("usage: net setlocalsid S-1-5-21-x-y-z\n");
		return 1;
	}

	if (!secrets_store_domain_sid(global_myname(), &sid)) {
		DEBUG(0,("Can't store domain SID as a pdc/bdc.\n"));
		return 1;
	}

	return 0;
}

static int net_setdomainsid(int argc, const char **argv)
{
	DOM_SID sid;

	if ( (argc != 1)
	     || (strncmp(argv[0], "S-1-5-21-", strlen("S-1-5-21-")) != 0)
	     || (!string_to_sid(&sid, argv[0]))
	     || (sid.num_auths != 4)) {
		d_printf("usage: net setdomainsid S-1-5-21-x-y-z\n");
		return 1;
	}

	if (!secrets_store_domain_sid(lp_workgroup(), &sid)) {
		DEBUG(0,("Can't store domain SID.\n"));
		return 1;
	}

	return 0;
}

static int net_getdomainsid(int argc, const char **argv)
{
	DOM_SID domain_sid;
	fstring sid_str;

	if(!initialize_password_db(False)) {
		DEBUG(0, ("WARNING: Could not open passdb - domain sid may not reflect passdb\n"
			  "backend knowlege (such as the sid stored in LDAP)\n"));
	}

	/* Generate one, if it doesn't exist */
	get_global_sam_sid();

	if (!secrets_fetch_domain_sid(global_myname(), &domain_sid)) {
		d_fprintf(stderr, "Could not fetch local SID\n");
		return 1;
	}
	sid_to_string(sid_str, &domain_sid);
	d_printf("SID for domain %s is: %s\n", global_myname(), sid_str);

	if (!secrets_fetch_domain_sid(opt_workgroup, &domain_sid)) {
		d_fprintf(stderr, "Could not fetch domain SID\n");
		return 1;
	}

	sid_to_string(sid_str, &domain_sid);
	d_printf("SID for domain %s is: %s\n", opt_workgroup, sid_str);

	return 0;
}

#ifdef WITH_FAKE_KASERVER

int net_help_afs(int argc, const char **argv)
{
	d_printf("  net afs key filename\n"
		 "\tImports a OpenAFS KeyFile into our secrets.tdb\n\n");
	d_printf("  net afs impersonate <user> <cell>\n"
		 "\tCreates a token for user@cell\n\n");
	return -1;
}

static int net_afs_key(int argc, const char **argv)
{
	int fd;
	struct afs_keyfile keyfile;

	if (argc != 2) {
		d_printf("usage: 'net afs key <keyfile> cell'\n");
		return -1;
	}

	if (!secrets_init()) {
		d_fprintf(stderr, "Could not open secrets.tdb\n");
		return -1;
	}

	if ((fd = open(argv[0], O_RDONLY, 0)) < 0) {
		d_fprintf(stderr, "Could not open %s\n", argv[0]);
		return -1;
	}

	if (read(fd, &keyfile, sizeof(keyfile)) != sizeof(keyfile)) {
		d_fprintf(stderr, "Could not read keyfile\n");
		return -1;
	}

	if (!secrets_store_afs_keyfile(argv[1], &keyfile)) {
		d_fprintf(stderr, "Could not write keyfile to secrets.tdb\n");
		return -1;
	}

	return 0;
}

static int net_afs_impersonate(int argc, const char **argv)
{
	char *token;

	if (argc != 2) {
		fprintf(stderr, "Usage: net afs impersonate <user> <cell>\n");
	        exit(1);
	}

	token = afs_createtoken_str(argv[0], argv[1]);

	if (token == NULL) {
		fprintf(stderr, "Could not create token\n");
	        exit(1);
	}

	if (!afs_settoken_str(token)) {
		fprintf(stderr, "Could not set token into kernel\n");
	        exit(1);
	}

	printf("Success: %s@%s\n", argv[0], argv[1]);
	return 0;
}

static int net_afs(int argc, const char **argv)
{
	struct functable func[] = {
		{"key", net_afs_key},
		{"impersonate", net_afs_impersonate},
		{"help", net_help_afs},
		{NULL, NULL}
	};
	return net_run_function(argc, argv, func, net_help_afs);
}

#endif /* WITH_FAKE_KASERVER */

static BOOL search_maxrid(struct pdb_search *search, const char *type,
			  uint32 *max_rid)
{
	struct samr_displayentry *entries;
	uint32 i, num_entries;

	if (search == NULL) {
		d_fprintf(stderr, "get_maxrid: Could not search %s\n", type);
		return False;
	}

	num_entries = pdb_search_entries(search, 0, 0xffffffff, &entries);
	for (i=0; i<num_entries; i++)
		*max_rid = MAX(*max_rid, entries[i].rid);
	pdb_search_destroy(search);
	return True;
}

static uint32 get_maxrid(void)
{
	uint32 max_rid = 0;

	if (!search_maxrid(pdb_search_users(0), "users", &max_rid))
		return 0;

	if (!search_maxrid(pdb_search_groups(), "groups", &max_rid))
		return 0;

	if (!search_maxrid(pdb_search_aliases(get_global_sam_sid()),
			   "aliases", &max_rid))
		return 0;
	
	return max_rid;
}

static int net_maxrid(int argc, const char **argv)
{
	uint32 rid;

	if (argc != 0) {
	        DEBUG(0, ("usage: net maxrid\n"));
		return 1;
	}

	if ((rid = get_maxrid()) == 0) {
		DEBUG(0, ("can't get current maximum rid\n"));
		return 1;
	}

	d_printf("Currently used maximum rid: %d\n", rid);

	return 0;
}

/* main function table */
static struct functable net_func[] = {
	{"RPC", net_rpc},
	{"RAP", net_rap},
	{"ADS", net_ads},

	/* eventually these should auto-choose the transport ... */
	{"FILE", net_file},
	{"SHARE", net_share},
	{"SESSION", net_rap_session},
	{"SERVER", net_rap_server},
	{"DOMAIN", net_rap_domain},
	{"PRINTQ", net_rap_printq},
	{"USER", net_user},
	{"GROUP", net_group},
	{"GROUPMAP", net_groupmap},
	{"SAM", net_sam},
	{"VALIDATE", net_rap_validate},
	{"GROUPMEMBER", net_rap_groupmember},
	{"ADMIN", net_rap_admin},
	{"SERVICE", net_rap_service},	
	{"PASSWORD", net_rap_password},
	{"CHANGETRUSTPW", net_changetrustpw},
	{"CHANGESECRETPW", net_changesecretpw},
	{"TIME", net_time},
	{"LOOKUP", net_lookup},
	{"JOIN", net_join},
	{"CACHE", net_cache},
	{"GETLOCALSID", net_getlocalsid},
	{"SETLOCALSID", net_setlocalsid},
	{"SETDOMAINSID", net_setdomainsid},
	{"GETDOMAINSID", net_getdomainsid},
	{"MAXRID", net_maxrid},
	{"IDMAP", net_idmap},
	{"STATUS", net_status},
	{"USERSHARE", net_usershare},
	{"USERSIDLIST", net_usersidlist},
#ifdef WITH_FAKE_KASERVER
	{"AFS", net_afs},
#endif

	{"HELP", net_help},
	{NULL, NULL}
};


/****************************************************************************
  main program
****************************************************************************/
 int main(int argc, const char **argv)
{
	int opt,i;
	char *p;
	int rc = 0;
	int argc_new = 0;
	const char ** argv_new;
	poptContext pc;

	struct poptOption long_options[] = {
		{"help",	'h', POPT_ARG_NONE,   0, 'h'},
		{"workgroup",	'w', POPT_ARG_STRING, &opt_target_workgroup},
		{"user",	'U', POPT_ARG_STRING, &opt_user_name, 'U'},
		{"ipaddress",	'I', POPT_ARG_STRING, 0,'I'},
		{"port",	'p', POPT_ARG_INT,    &opt_port},
		{"myname",	'n', POPT_ARG_STRING, &opt_requester_name},
		{"server",	'S', POPT_ARG_STRING, &opt_host},
		{"container",	'c', POPT_ARG_STRING, &opt_container},
		{"comment",	'C', POPT_ARG_STRING, &opt_comment},
		{"maxusers",	'M', POPT_ARG_INT,    &opt_maxusers},
		{"flags",	'F', POPT_ARG_INT,    &opt_flags},
		{"long",	'l', POPT_ARG_NONE,   &opt_long_list_entries},
		{"reboot",	'r', POPT_ARG_NONE,   &opt_reboot},
		{"force",	'f', POPT_ARG_NONE,   &opt_force},
		{"stdin",	'i', POPT_ARG_NONE,   &opt_stdin},
		{"timeout",	't', POPT_ARG_INT,    &opt_timeout},
		{"machine-pass",'P', POPT_ARG_NONE,   &opt_machine_pass},
		{"myworkgroup", 'W', POPT_ARG_STRING, &opt_workgroup},
		{"verbose",	'v', POPT_ARG_NONE,   &opt_verbose},
		/* Options for 'net groupmap set' */
		{"local",       'L', POPT_ARG_NONE,   &opt_localgroup},
		{"domain",      'D', POPT_ARG_NONE,   &opt_domaingroup},
		{"ntname",      'N', POPT_ARG_STRING, &opt_newntname},
		{"rid",         'R', POPT_ARG_INT,    &opt_rid},
		/* Options for 'net rpc share migrate' */
		{"acls",	0, POPT_ARG_NONE,     &opt_acls},
		{"attrs",	0, POPT_ARG_NONE,     &opt_attrs},
		{"timestamps",	0, POPT_ARG_NONE,     &opt_timestamps},
		{"exclude",	'e', POPT_ARG_STRING, &opt_exclude},
		{"destination",	0, POPT_ARG_STRING,   &opt_destination},

		POPT_COMMON_SAMBA
		{ 0, 0, 0, 0}
	};

	zero_ip(&opt_dest_ip);

	load_case_tables();

	/* set default debug level to 0 regardless of what smb.conf sets */
	DEBUGLEVEL_CLASS[DBGC_ALL] = 0;
	dbf = x_stderr;
	
	pc = poptGetContext(NULL, argc, (const char **) argv, long_options, 
			    POPT_CONTEXT_KEEP_FIRST);
	
	while((opt = poptGetNextOpt(pc)) != -1) {
		switch (opt) {
		case 'h':
			net_help(argc, argv);
			exit(0);
			break;
		case 'I':
			opt_dest_ip = *interpret_addr2(poptGetOptArg(pc));
			if (is_zero_ip(opt_dest_ip))
				d_fprintf(stderr, "\nInvalid ip address specified\n");
			else
				opt_have_ip = True;
			break;
		case 'U':
			opt_user_specified = True;
			opt_user_name = SMB_STRDUP(opt_user_name);
			p = strchr(opt_user_name,'%');
			if (p) {
				*p = 0;
				opt_password = p+1;
			}
			break;
		default:
			d_fprintf(stderr, "\nInvalid option %s: %s\n", 
				 poptBadOption(pc, 0), poptStrerror(opt));
			net_help(argc, argv);
			exit(1);
		}
	}
	
	/*
	 * Don't load debug level from smb.conf. It should be
	 * set by cmdline arg or remain default (0)
	 */
	AllowDebugChange = False;
	lp_load(dyn_CONFIGFILE,True,False,False,True);
	
 	argv_new = (const char **)poptGetArgs(pc);

	argc_new = argc;
	for (i=0; i<argc; i++) {
		if (argv_new[i] == NULL) {
			argc_new = i;
			break;
		}
	}

	if (opt_requester_name) {
		set_global_myname(opt_requester_name);
	}

	if (!opt_user_name && getenv("LOGNAME")) {
		opt_user_name = getenv("LOGNAME");
	}

	if (!opt_workgroup) {
		opt_workgroup = smb_xstrdup(lp_workgroup());
	}
	
	if (!opt_target_workgroup) {
		opt_target_workgroup = smb_xstrdup(lp_workgroup());
	}
	
	if (!init_names())
		exit(1);

	load_interfaces();
	
	/* this makes sure that when we do things like call scripts, 
	   that it won't assert becouse we are not root */
	sec_init();

	if (opt_machine_pass) {
		/* it is very useful to be able to make ads queries as the
		   machine account for testing purposes and for domain leave */

		net_use_machine_password();
	}

	if (!opt_password) {
		opt_password = getenv("PASSWD");
	}
  	 
	rc = net_run_function(argc_new-1, argv_new+1, net_func, net_help);
	
	DEBUG(2,("return code = %d\n", rc));
	return rc;
}
