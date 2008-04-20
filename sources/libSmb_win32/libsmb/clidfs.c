/* 
   Unix SMB/CIFS implementation.
   client connect/disconnect routines
   Copyright (C) Andrew Tridgell                  1994-1998
   Copyright (C) Gerald (Jerry) Carter            2004
      
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


struct client_connection {
	struct client_connection *prev, *next;
	struct cli_state *cli;
	pstring mount;
};

/* global state....globals reek! */

static pstring username;
static pstring password;
static BOOL use_kerberos;
static BOOL got_pass;
static int signing_state;
int max_protocol = PROTOCOL_NT1;

static int port;
static int name_type = 0x20;
static BOOL have_ip;
static struct in_addr dest_ip;

static struct client_connection *connections;

/********************************************************************
 Return a connection to a server.
********************************************************************/

static struct cli_state *do_connect( const char *server, const char *share,
                                     BOOL show_sessetup )
{
	struct cli_state *c;
	struct nmb_name called, calling;
	const char *server_n;
	struct in_addr ip;
	pstring servicename;
	char *sharename;
	fstring newserver, newshare;
	
	/* make a copy so we don't modify the global string 'service' */
	pstrcpy(servicename, share);
	sharename = servicename;
	if (*sharename == '\\') {
		server = sharename+2;
		sharename = strchr_m(server,'\\');
		if (!sharename) return NULL;
		*sharename = 0;
		sharename++;
	}

	server_n = server;
	
	zero_ip(&ip);

	make_nmb_name(&calling, global_myname(), 0x0);
	make_nmb_name(&called , server, name_type);

 again:
	zero_ip(&ip);
	if (have_ip) 
		ip = dest_ip;

	/* have to open a new connection */
	if (!(c=cli_initialise(NULL)) || (cli_set_port(c, port) != port) ||
	    !cli_connect(c, server_n, &ip)) {
		d_printf("Connection to %s failed\n", server_n);
		return NULL;
	}

	c->protocol = max_protocol;
	c->use_kerberos = use_kerberos;
	cli_setup_signing_state(c, signing_state);
		

	if (!cli_session_request(c, &calling, &called)) {
		char *p;
		d_printf("session request to %s failed (%s)\n", 
			 called.name, cli_errstr(c));
		cli_shutdown(c);
		if ((p=strchr_m(called.name, '.'))) {
			*p = 0;
			goto again;
		}
		if (strcmp(called.name, "*SMBSERVER")) {
			make_nmb_name(&called , "*SMBSERVER", 0x20);
			goto again;
		}
		return NULL;
	}

	DEBUG(4,(" session request ok\n"));

	if (!cli_negprot(c)) {
		d_printf("protocol negotiation failed\n");
		cli_shutdown(c);
		return NULL;
	}

	if (!got_pass) {
		char *pass = getpass("Password: ");
		if (pass) {
			pstrcpy(password, pass);
			got_pass = 1;
		}
	}

	if (!cli_session_setup(c, username, 
			       password, strlen(password),
			       password, strlen(password),
			       lp_workgroup())) {
		/* if a password was not supplied then try again with a null username */
		if (password[0] || !username[0] || use_kerberos ||
		    !cli_session_setup(c, "", "", 0, "", 0, lp_workgroup())) { 
			d_printf("session setup failed: %s\n", cli_errstr(c));
			if (NT_STATUS_V(cli_nt_error(c)) == 
			    NT_STATUS_V(NT_STATUS_MORE_PROCESSING_REQUIRED))
				d_printf("did you forget to run kinit?\n");
			cli_shutdown(c);
			return NULL;
		}
		d_printf("Anonymous login successful\n");
	}

	if ( show_sessetup ) {
		if (*c->server_domain) {
			DEBUG(0,("Domain=[%s] OS=[%s] Server=[%s]\n",
				c->server_domain,c->server_os,c->server_type));
		} else if (*c->server_os || *c->server_type){
			DEBUG(0,("OS=[%s] Server=[%s]\n",
				 c->server_os,c->server_type));
		}		
	}
	DEBUG(4,(" session setup ok\n"));

	/* here's the fun part....to support 'msdfs proxy' shares
	   (on Samba or windows) we have to issues a TRANS_GET_DFS_REFERRAL 
	   here before trying to connect to the original share.
	   check_dfs_proxy() will fail if it is a normal share. */

	if ( (c->capabilities & CAP_DFS) && cli_check_msdfs_proxy( c, sharename, newserver, newshare ) ) {
		cli_shutdown(c);
		return do_connect( newserver, newshare, False );
	}

	/* must be a normal share */

	if (!cli_send_tconX(c, sharename, "?????", password, strlen(password)+1)) {
		d_printf("tree connect failed: %s\n", cli_errstr(c));
		cli_shutdown(c);
		return NULL;
	}

	DEBUG(4,(" tconx ok\n"));

	return c;
}

/****************************************************************************
****************************************************************************/

static void cli_cm_set_mntpoint( struct cli_state *c, const char *mnt )
{
	struct client_connection *p;
	int i;

	for ( p=connections,i=0; p; p=p->next,i++ ) {
		if ( strequal(p->cli->desthost, c->desthost) && strequal(p->cli->share, c->share) )
			break;
	}
	
	if ( p ) {
		pstrcpy( p->mount, mnt );
		dos_clean_name( p->mount );
	}
}

/****************************************************************************
****************************************************************************/

const char * cli_cm_get_mntpoint( struct cli_state *c )
{
	struct client_connection *p;
	int i;

	for ( p=connections,i=0; p; p=p->next,i++ ) {
		if ( strequal(p->cli->desthost, c->desthost) && strequal(p->cli->share, c->share) )
			break;
	}
	
	if ( p )
		return p->mount;
		
	return NULL;
}

/********************************************************************
 Add a new connection to the list
********************************************************************/

static struct cli_state* cli_cm_connect( const char *server, const char *share,
                                         BOOL show_hdr )
{
	struct client_connection *node;
	
	node = SMB_XMALLOC_P( struct client_connection );
	
	node->cli = do_connect( server, share, show_hdr );

	if ( !node->cli ) {
		SAFE_FREE( node );
		return NULL;
	}

	DLIST_ADD( connections, node );

	cli_cm_set_mntpoint( node->cli, "" );

	return node->cli;

}

/********************************************************************
 Return a connection to a server.
********************************************************************/

static struct cli_state* cli_cm_find( const char *server, const char *share )
{
	struct client_connection *p;

	for ( p=connections; p; p=p->next ) {
		if ( strequal(server, p->cli->desthost) && strequal(share,p->cli->share) )
			return p->cli;
	}

	return NULL;
}

/****************************************************************************
 open a client connection to a \\server\share.  Set's the current *cli 
 global variable as a side-effect (but only if the connection is successful).
****************************************************************************/

struct cli_state* cli_cm_open( const char *server, const char *share, BOOL show_hdr )
{
	struct cli_state *c;
	
	/* try to reuse an existing connection */

	c = cli_cm_find( server, share );
	
	if ( !c )
		c = cli_cm_connect( server, share, show_hdr );

	return c;
}

/****************************************************************************
****************************************************************************/

void cli_cm_shutdown( void )
{

	struct client_connection *p, *x;

	for ( p=connections; p; ) {
		cli_shutdown( p->cli );
		x = p;
		p = p->next;

		SAFE_FREE( x );
	}

	connections = NULL;

	return;
}

/****************************************************************************
****************************************************************************/

void cli_cm_display(void)
{
	struct client_connection *p;
	int i;

	for ( p=connections,i=0; p; p=p->next,i++ ) {
		d_printf("%d:\tserver=%s, share=%s\n", 
			i, p->cli->desthost, p->cli->share );
	}
}

/****************************************************************************
****************************************************************************/

void cli_cm_set_credentials( struct user_auth_info *user )
{
	pstrcpy( username, user->username );
	
	if ( user->got_pass ) {
		pstrcpy( password, user->password );
		got_pass = True;
	}
	
	use_kerberos = user->use_kerberos;	
	signing_state = user->signing_state;
}

/****************************************************************************
****************************************************************************/

void cli_cm_set_port( int port_number )
{
	port = port_number;
}

/****************************************************************************
****************************************************************************/

void cli_cm_set_dest_name_type( int type )
{
	name_type = type;
}

/****************************************************************************
****************************************************************************/

void cli_cm_set_dest_ip(struct in_addr ip )
{
	dest_ip = ip;
	have_ip = True;
}

/********************************************************************
 split a dfs path into the server and share name components
********************************************************************/

static void split_dfs_path( const char *nodepath, fstring server, fstring share )
{
	char *p;
	pstring path;

	pstrcpy( path, nodepath );

	if ( path[0] != '\\' )
		return;

	p = strrchr_m( path, '\\' );

	if ( !p )
		return;

	*p = '\0';
	p++;

	fstrcpy( share, p );
	fstrcpy( server, &path[1] );
}

/****************************************************************************
 return the original path truncated at the first wildcard character
 (also strips trailing \'s).  Trust the caller to provide a NULL 
 terminated string
****************************************************************************/

static void clean_path( pstring clean, const char *path )
{
	int len;
	char *p;
	pstring newpath;
		
	pstrcpy( newpath, path );
	p = newpath;
	
	while ( p ) {
		/* first check for '*' */
		
		p = strrchr_m( newpath, '*' );
		if ( p ) {
			*p = '\0';
			p = newpath;
			continue;
		}
	
		/* first check for '?' */
		
		p = strrchr_m( newpath, '?' );
		if ( p ) {
			*p = '\0';
			p = newpath;
		}
	}
	
	/* strip a trailing backslash */
	
	len = strlen( newpath );
	if ( (len > 0) && (newpath[len-1] == '\\') )
		newpath[len-1] = '\0';
		
	pstrcpy( clean, newpath );
}

/****************************************************************************
****************************************************************************/

BOOL cli_dfs_make_full_path( pstring path, const char *server, const char *share,
                            const char *dir )
{
	pstring servicename;
	char *sharename;
	const char *directory;

	
	/* make a copy so we don't modify the global string 'service' */
	
	pstrcpy(servicename, share);
	sharename = servicename;
	
	if (*sharename == '\\') {
	
		server = sharename+2;
		sharename = strchr_m(server,'\\');
		
		if (!sharename) 
			return False;
			
		*sharename = 0;
		sharename++;
	}

	directory = dir;
	if ( *directory == '\\' )
		directory++;
	
	pstr_sprintf( path, "\\%s\\%s\\%s", server, sharename, directory );

	return True;
}

/********************************************************************
 check for dfs referral
********************************************************************/

static BOOL cli_dfs_check_error( struct cli_state *cli, NTSTATUS status )
{
	uint32 flgs2 = SVAL(cli->inbuf,smb_flg2);

	/* only deal with DS when we negotiated NT_STATUS codes and UNICODE */

	if ( !( (flgs2&FLAGS2_32_BIT_ERROR_CODES) && (flgs2&FLAGS2_UNICODE_STRINGS) ) )
		return False;

	if ( NT_STATUS_EQUAL( status, NT_STATUS(IVAL(cli->inbuf,smb_rcls)) ) )
		return True;

	return False;
}

/********************************************************************
 get the dfs referral link
********************************************************************/

BOOL cli_dfs_get_referral( struct cli_state *cli, const char *path, 
                           CLIENT_DFS_REFERRAL**refs, size_t *num_refs,
			   uint16 *consumed)
{
	unsigned int data_len = 0;
	unsigned int param_len = 0;
	uint16 setup = TRANSACT2_GET_DFS_REFERRAL;
	char param[sizeof(pstring)+2];
	pstring data;
	char *rparam=NULL, *rdata=NULL;
	char *p;
	size_t pathlen = 2*(strlen(path)+1);
	uint16 num_referrals;
	CLIENT_DFS_REFERRAL *referrals = NULL;
	
	memset(param, 0, sizeof(param));
	SSVAL(param, 0, 0x03);	/* max referral level */
	p = &param[2];

	p += clistr_push(cli, p, path, MIN(pathlen, sizeof(param)-2), STR_TERMINATE);
	param_len = PTR_DIFF(p, param);

	if (!cli_send_trans(cli, SMBtrans2,
		NULL,                        /* name */
		-1, 0,                          /* fid, flags */
		&setup, 1, 0,                   /* setup, length, max */
		param, param_len, 2,            /* param, length, max */
		(char *)&data,  data_len, cli->max_xmit /* data, length, max */
		)) {
			return False;
	}

	if (!cli_receive_trans(cli, SMBtrans2,
		&rparam, &param_len,
		&rdata, &data_len)) {
			return False;
	}
	
	*consumed     = SVAL( rdata, 0 );
	num_referrals = SVAL( rdata, 2 );
	
	if ( num_referrals != 0 ) {
		uint16 ref_version;
		uint16 ref_size;
		int i;
		uint16 node_offset;
		
		
		referrals = SMB_XMALLOC_ARRAY( CLIENT_DFS_REFERRAL, num_referrals );
	
		/* start at the referrals array */
	
		p = rdata+8;
		for ( i=0; i<num_referrals; i++ ) {
			ref_version = SVAL( p, 0 );
			ref_size    = SVAL( p, 2 );
			node_offset = SVAL( p, 16 );
			
			if ( ref_version != 3 ) {
				p += ref_size;
				continue;
			}
			
			referrals[i].proximity = SVAL( p, 8 );
			referrals[i].ttl       = SVAL( p, 10 );

			clistr_pull( cli, referrals[i].dfspath, p+node_offset, 
				sizeof(referrals[i].dfspath), -1, STR_TERMINATE|STR_UNICODE );

			p += ref_size;
		}
	
	}
	
	*num_refs = num_referrals;
	*refs = referrals;

	SAFE_FREE(rdata);
	SAFE_FREE(rparam);

	return True;
}

/********************************************************************
********************************************************************/

BOOL cli_resolve_path( const char *mountpt, struct cli_state *rootcli, const char *path,
                       struct cli_state **targetcli, pstring targetpath )
{
	CLIENT_DFS_REFERRAL *refs = NULL;
	size_t num_refs;
	uint16 consumed;
	struct cli_state *cli_ipc;
	pstring fullpath, cleanpath;
	int pathlen;
	fstring server, share;
	struct cli_state *newcli;
	pstring newpath;
	pstring newmount;
	char *ppath;
	
	SMB_STRUCT_STAT sbuf;
	uint32 attributes;
	
	if ( !rootcli || !path || !targetcli )
		return False;
		
	*targetcli = NULL;
	
	/* send a trans2_query_path_info to check for a referral */
	
	clean_path( cleanpath, 	path );
	cli_dfs_make_full_path( fullpath, rootcli->desthost, rootcli->share, cleanpath );

	/* don't bother continuing if this is not a dfs root */
	
	if ( !rootcli->dfsroot || cli_qpathinfo_basic( rootcli, cleanpath, &sbuf, &attributes ) ) {
		*targetcli = rootcli;
		pstrcpy( targetpath, path );
		return True;
	}

	/* special case where client asked for a path that does not exist */

	if ( cli_dfs_check_error(rootcli, NT_STATUS_OBJECT_NAME_NOT_FOUND) ) {
		*targetcli = rootcli;
		pstrcpy( targetpath, path );
		return True;
	}

	/* we got an error, check for DFS referral */
			
	if ( !cli_dfs_check_error(rootcli, NT_STATUS_PATH_NOT_COVERED) ) 
		return False;

	/* check for the referral */

	if ( !(cli_ipc = cli_cm_open( rootcli->desthost, "IPC$", False )) )
		return False;
	
	if ( !cli_dfs_get_referral(cli_ipc, fullpath, &refs, &num_refs, &consumed) 
		|| !num_refs )
	{
		return False;
	}
	
	/* just store the first referral for now
	   Make sure to recreate the original string including any wildcards */
	
	cli_dfs_make_full_path( fullpath, rootcli->desthost, rootcli->share, path );
	pathlen = strlen( fullpath )*2;
	consumed = MIN(pathlen, consumed );
	pstrcpy( targetpath, &fullpath[consumed/2] );

	split_dfs_path( refs[0].dfspath, server, share );
	SAFE_FREE( refs );
	
	/* open the connection to the target path */
	
	if ( (*targetcli = cli_cm_open(server, share, False)) == NULL ) {
		d_printf("Unable to follow dfs referral [//%s/%s]\n",
			server, share );
			
		return False;
	}
	
	/* parse out the consumed mount path */
	/* trim off the \server\share\ */

	fullpath[consumed/2] = '\0';
	dos_clean_name( fullpath );
	if ((ppath = strchr_m( fullpath, '\\' )) == NULL)
		return False;
	if ((ppath = strchr_m( ppath+1, '\\' )) == NULL)
		return False;
	if ((ppath = strchr_m( ppath+1, '\\' )) == NULL)
		return False;
	ppath++;
	
	pstr_sprintf( newmount, "%s\\%s", mountpt, ppath );
	cli_cm_set_mntpoint( *targetcli, newmount );

	/* check for another dfs referral, note that we are not 
	   checking for loops here */

	if ( !strequal( targetpath, "\\" ) ) {
		if ( cli_resolve_path( newmount, *targetcli, targetpath, &newcli, newpath ) ) {
			*targetcli = newcli;
			pstrcpy( targetpath, newpath );
		}
	}

	return True;
}

/********************************************************************
********************************************************************/

BOOL cli_check_msdfs_proxy( struct cli_state *cli, const char *sharename,
                            fstring newserver, fstring newshare )
{
	CLIENT_DFS_REFERRAL *refs = NULL;
	size_t num_refs;
	uint16 consumed;
	pstring fullpath;
	BOOL res;
	uint16 cnum;
	
	if ( !cli || !sharename )
		return False;

	cnum = cli->cnum;

	/* special case.  never check for a referral on the IPC$ share */

	if ( strequal( sharename, "IPC$" ) )
		return False;
		
	/* send a trans2_query_path_info to check for a referral */
	
	pstr_sprintf( fullpath, "\\%s\\%s", cli->desthost, sharename );

	/* check for the referral */

	if (!cli_send_tconX(cli, "IPC$", "IPC", NULL, 0)) {
		return False;
	}

	res = cli_dfs_get_referral(cli, fullpath, &refs, &num_refs, &consumed);

	if (!cli_tdis(cli)) {
		SAFE_FREE( refs );
		return False;
	}

	cli->cnum = cnum;
		
	if (!res || !num_refs ) {
		SAFE_FREE( refs );
		return False;
	}
	
	split_dfs_path( refs[0].dfspath, newserver, newshare );

	/* check that this is not a self-referral */

	if ( strequal( cli->desthost, newserver ) && strequal( sharename, newshare ) ) {
		SAFE_FREE( refs );
		return False;
	}
	
	SAFE_FREE( refs );
	
	return True;
}
