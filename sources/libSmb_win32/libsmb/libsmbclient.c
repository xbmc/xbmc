/* 
   Unix SMB/Netbios implementation.
   SMB client library implementation
   Copyright (C) Andrew Tridgell 1998
   Copyright (C) Richard Sharpe 2000, 2002
   Copyright (C) John Terpstra 2000
   Copyright (C) Tom Jansen (Ninja ISD) 2002 
   Copyright (C) Derrell Lipman 2003, 2004
   
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

#include "include/libsmb_internal.h"


/*
 * DOS Attribute values (used internally)
 */
typedef struct DOS_ATTR_DESC {
	int mode;
	SMB_OFF_T size;
	time_t a_time;
	time_t c_time;
	time_t m_time;
	SMB_INO_T inode;
} DOS_ATTR_DESC;


/*
 * Internal flags for extended attributes
 */

/* internal mode values */
#define SMBC_XATTR_MODE_ADD          1
#define SMBC_XATTR_MODE_REMOVE       2
#define SMBC_XATTR_MODE_REMOVE_ALL   3
#define SMBC_XATTR_MODE_SET          4
#define SMBC_XATTR_MODE_CHOWN        5
#define SMBC_XATTR_MODE_CHGRP        6

#define CREATE_ACCESS_READ      READ_CONTROL_ACCESS

/*We should test for this in configure ... */
#ifndef ENOTSUP
#define ENOTSUP EOPNOTSUPP
#endif

/*
 * Functions exported by libsmb_cache.c that we need here
 */
int smbc_default_cache_functions(SMBCCTX *context);

/* 
 * check if an element is part of the list. 
 * FIXME: Does not belong here !  
 * Can anyone put this in a macro in dlinklist.h ?
 * -- Tom
 */
static int DLIST_CONTAINS(SMBCFILE * list, SMBCFILE *p) {
	if (!p || !list) return False;
	do {
		if (p == list) return True;
		list = list->next;
	} while (list);
	return False;
}

/*
 * Find an lsa pipe handle associated with a cli struct.
 */
static struct rpc_pipe_client *
find_lsa_pipe_hnd(struct cli_state *ipc_cli)
{
	struct rpc_pipe_client *pipe_hnd;

	for (pipe_hnd = ipc_cli->pipe_list;
             pipe_hnd;
             pipe_hnd = pipe_hnd->next) {
            
		if (pipe_hnd->pipe_idx == PI_LSARPC) {
			return pipe_hnd;
		}
	}

	return NULL;
}

static int
smbc_close_ctx(SMBCCTX *context,
               SMBCFILE *file);
static SMB_OFF_T
smbc_lseek_ctx(SMBCCTX *context,
               SMBCFILE *file,
               SMB_OFF_T offset,
               int whence);

extern BOOL in_client;

/*
 * Is the logging working / configfile read ? 
 */
static int smbc_initialized = 0;

static int 
hex2int( unsigned int _char )
{
    if ( _char >= 'A' && _char <='F')
	return _char - 'A' + 10;
    if ( _char >= 'a' && _char <='f')
	return _char - 'a' + 10;
    if ( _char >= '0' && _char <='9')
	return _char - '0';
    return -1;
}

/*
 * smbc_urldecode()
 *
 * Convert strings of %xx to their single character equivalent.  Each 'x' must
 * be a valid hexadecimal digit, or that % sequence is left undecoded.
 *
 * dest may, but need not be, the same pointer as src.
 *
 * Returns the number of % sequences which could not be converted due to lack
 * of two following hexadecimal digits.
 */
int
smbc_urldecode(char *dest, char * src, size_t max_dest_len)
{
        int old_length = strlen(src);
        int i = 0;
        int err_count = 0;
        pstring temp;
        char * p;

        if ( old_length == 0 ) {
                return 0;
        }

        p = temp;
        while ( i < old_length ) {
                unsigned char character = src[ i++ ];

                if (character == '%') {
                        int a = i+1 < old_length ? hex2int( src[i] ) : -1;
                        int b = i+1 < old_length ? hex2int( src[i+1] ) : -1;

                        /* Replace valid sequence */
                        if (a != -1 && b != -1) {

                                /* Replace valid %xx sequence with %dd */
                                character = (a * 16) + b;

                                if (character == '\0') {
                                        break; /* Stop at %00 */
                                }

                                i += 2;
                        } else {

                                err_count++;
                        }
                }

                *p++ = character;
        }

        *p = '\0';

        strncpy(dest, temp, max_dest_len - 1);
        dest[max_dest_len - 1] = '\0';

        return err_count;
}

/*
 * smbc_urlencode()
 *
 * Convert any characters not specifically allowed in a URL into their %xx
 * equivalent.
 *
 * Returns the remaining buffer length.
 */
int
smbc_urlencode(char * dest, char * src, int max_dest_len)
{
        char hex[] = "0123456789ABCDEF";

        for (; *src != '\0' && max_dest_len >= 3; src++) {

                if ((*src < '0' &&
                     *src != '-' &&
                     *src != '.') ||
                    (*src > '9' &&
                     *src < 'A') ||
                    (*src > 'Z' &&
                     *src < 'a' &&
                     *src != '_') ||
                    (*src > 'z')) {
                        *dest++ = '%';
                        *dest++ = hex[(*src >> 4) & 0x0f];
                        *dest++ = hex[*src & 0x0f];
                        max_dest_len -= 3;
                } else {
                        *dest++ = *src;
                        max_dest_len--;
                }
        }

        *dest++ = '\0';
        max_dest_len--;
        
        return max_dest_len;
}

/*
 * Function to parse a path and turn it into components
 *
 * The general format of an SMB URI is explain in Christopher Hertel's CIFS
 * book, at http://ubiqx.org/cifs/Appendix-D.html.  We accept a subset of the
 * general format ("smb:" only; we do not look for "cifs:").
 *
 *
 * We accept:
 *  smb://[[[domain;]user[:password@]]server[/share[/path[/file]]]][?options]
 *
 * Meaning of URLs:
 *
 * smb://           Show all workgroups.
 *
 *                  The method of locating the list of workgroups varies
 *                  depending upon the setting of the context variable
 *                  context->options.browse_max_lmb_count.  This value
 *                  determine the maximum number of local master browsers to
 *                  query for the list of workgroups.  In order to ensure that
 *                  a complete list of workgroups is obtained, all master
 *                  browsers must be queried, but if there are many
 *                  workgroups, the time spent querying can begin to add up.
 *                  For small networks (not many workgroups), it is suggested
 *                  that this variable be set to 0, indicating query all local
 *                  master browsers.  When the network has many workgroups, a
 *                  reasonable setting for this variable might be around 3.
 *
 * smb://name/      if name<1D> or name<1B> exists, list servers in
 *                  workgroup, else, if name<20> exists, list all shares
 *                  for server ...
 *
 * If "options" are provided, this function returns the entire option list as a
 * string, for later parsing by the caller.  Note that currently, no options
 * are supported.
 */

static const char *smbc_prefix = "smb:";

static int
smbc_parse_path(SMBCCTX *context,
                const char *fname,
                char *workgroup, int workgroup_len,
                char *server, int server_len,
                char *share, int share_len,
                char *path, int path_len,
		char *user, int user_len,
                char *password, int password_len,
                char *options, int options_len)
{
	static pstring s;
	pstring userinfo;
	const char *p;
	char *q, *r;
	int len;

	server[0] = share[0] = path[0] = user[0] = password[0] = (char)0;

        /*
         * Assume we wont find an authentication domain to parse, so default
         * to the workgroup in the provided context.
         */
        if (workgroup != NULL) {
                strncpy(workgroup, context->workgroup, workgroup_len - 1);
                workgroup[workgroup_len - 1] = '\0';
        }

        if (options != NULL && options_len > 0) {
                options[0] = (char)0;
        }
	pstrcpy(s, fname);

	/* see if it has the right prefix */
	len = strlen(smbc_prefix);
	if (strncmp(s,smbc_prefix,len) || (s[len] != '/' && s[len] != 0)) {
                return -1; /* What about no smb: ? */
        }

	p = s + len;

	/* Watch the test below, we are testing to see if we should exit */

	if (strncmp(p, "//", 2) && strncmp(p, "\\\\", 2)) {

                DEBUG(1, ("Invalid path (does not begin with smb://"));
		return -1;

	}

	p += 2;  /* Skip the double slash */

        /* See if any options were specified */
#ifdef _XBOX
				/* check that '?' occurs after '@', if '@' exists at all */
				r = strrchr_m(p, '@');
        q = strrchr_m(p, '?');
        if (q && (!r || q > r)) {
#else
        if ((q = strrchr(p, '?')) != NULL ) {
#endif
                /* There are options.  Null terminate here and point to them */
                *q++ = '\0';
                
                DEBUG(4, ("Found options '%s'", q));

                /* Copy the options */
                if (options != NULL && options_len > 0) {
                        safe_strcpy(options, q, options_len - 1);
                }
        }

	if (*p == (char)0)
	    goto decoding;

	if (*p == '/') {

		strncpy(server, context->workgroup, 
			((strlen(context->workgroup) < 16)
                         ? strlen(context->workgroup)
                         : 16));
                server[server_len - 1] = '\0';
		return 0;
		
	}

	/*
	 * ok, its for us. Now parse out the server, share etc. 
	 *
	 * However, we want to parse out [[domain;]user[:password]@] if it
	 * exists ...
	 */

	/* check that '@' occurs before '/', if '/' exists at all */
	q = strchr_m(p, '@');
	r = strchr_m(p, '/');
	if (q && (!r || q < r)) {
		pstring username, passwd, domain;
		const char *u = userinfo;

		next_token(&p, userinfo, "@", sizeof(fstring));

		username[0] = passwd[0] = domain[0] = 0;

		if (strchr_m(u, ';')) {
      
			next_token(&u, domain, ";", sizeof(fstring));

		}

		if (strchr_m(u, ':')) {

			next_token(&u, username, ":", sizeof(fstring));

			pstrcpy(passwd, u);

		}
		else {

			pstrcpy(username, u);

		}

                if (domain[0] && workgroup) {
                        strncpy(workgroup, domain, workgroup_len - 1);
                        workgroup[workgroup_len - 1] = '\0';
                }

		if (username[0]) {
			strncpy(user, username, user_len - 1);
                        user[user_len - 1] = '\0';
                }

		if (passwd[0]) {
			strncpy(password, passwd, password_len - 1);
                        password[password_len - 1] = '\0';
                }

	}

	if (!next_token(&p, server, "/", sizeof(fstring))) {

		return -1;

	}

	if (*p == (char)0) goto decoding;  /* That's it ... */
  
	if (!next_token(&p, share, "/", sizeof(fstring))) {

		return -1;

	}

        /*
         * Prepend a leading slash if there's a file path, as required by
         * NetApp filers.
         */
        *path = '\0';
        if (*p != '\0') {
                *path = '/';
                safe_strcpy(path + 1, p, path_len - 2);
        }

	all_string_sub(path, "/", "\\", 0);

 decoding:
	(void) smbc_urldecode(path, path, path_len);
	(void) smbc_urldecode(server, server, server_len);
	(void) smbc_urldecode(share, share, share_len);
	(void) smbc_urldecode(user, user, user_len);
	(void) smbc_urldecode(password, password, password_len);

	return 0;
}

/*
 * Verify that the options specified in a URL are valid
 */
static int
smbc_check_options(char *server,
                   char *share,
                   char *path,
                   char *options)
{
        DEBUG(4, ("smbc_check_options(): server='%s' share='%s' "
                  "path='%s' options='%s'\n",
                  server, share, path, options));

        /* No options at all is always ok */
        if (! *options) return 0;

        /* Currently, we don't support any options. */
        return -1;
}

/*
 * Convert an SMB error into a UNIX error ...
 */
static int
smbc_errno(SMBCCTX *context,
           struct cli_state *c)
{
	int ret = cli_errno(c);
	
        if (cli_is_dos_error(c)) {
                uint8 eclass;
                uint32 ecode;

                cli_dos_error(c, &eclass, &ecode);
                
                DEBUG(3,("smbc_error %d %d (0x%x) -> %d\n", 
                         (int)eclass, (int)ecode, (int)ecode, ret));
        } else {
                NTSTATUS status;

                status = cli_nt_error(c);

                DEBUG(3,("smbc errno %s -> %d\n",
                         nt_errstr(status), ret));
        }

	return ret;
}

/* 
 * Check a server for being alive and well.
 * returns 0 if the server is in shape. Returns 1 on error 
 * 
 * Also useable outside libsmbclient to enable external cache
 * to do some checks too.
 */
static int
smbc_check_server(SMBCCTX * context,
                  SMBCSRV * server) 
{
#ifdef _XBOX
  struct timeval now, access;
  int error;

  if(cli_is_error(&server->cli))
  {
    error = cli_errno(&server->cli);
    if(  error == EINVAL 
      || error == EFAULT 
      || error == EBADF 
      || error == ENOSYS 
      || error == ENETUNREACH 
      || error == ECONNABORTED
      || error == ECONNREFUSED 
      || error == ENOPROTOOPT
      || error == ETIMEDOUT )
    {
			DEBUG(3, ("smbc_check_server: connection to [%s/%s] had error on last call, disconnecting it", 
						server->cli.desthost, server->cli.share));
      return 1;
    }
  }
  
	access = server->access;
	gettimeofday(&now, NULL);
	server->access = now;
	
	/* if server has been idle, check it, otherwise assume good */
	if( access.tv_sec && (now.tv_sec > access.tv_sec + 10) ) {
		if(!cli_echo(&server->cli,"",0) ) {
			DEBUG(3, ("smbc_check_server: server [%s] disconnected session on share [%s]", 
						server->cli.desthost, server->cli.share));
			return 1;
		}
	}
  else
  {
    /* just check so we still can write to socket */
    if( sys_write(server->cli.fd,"",0) != 0 )
      return 1;
  }
#else

	if ( send_keepalive(server->cli.fd) == False )
		return 1;
#endif

	/* connection is ok */
	return 0;
}

/* 
 * Remove a server from the cached server list it's unused.
 * On success, 0 is returned. 1 is returned if the server could not be removed.
 * 
 * Also useable outside libsmbclient
 */
int
smbc_remove_unused_server(SMBCCTX * context,
                          SMBCSRV * srv)
{
	SMBCFILE * file;

	/* are we being fooled ? */
	if (!context || !context->internal ||
	    !context->internal->_initialized || !srv) return 1;

	
	/* Check all open files/directories for a relation with this server */
	for (file = context->internal->_files; file; file=file->next) {
		if (file->srv == srv) {
			/* Still used */
			DEBUG(3, ("smbc_remove_usused_server: "
                                  "%p still used by %p.\n", 
				  srv, file));
			return 1;
		}
	}

	DLIST_REMOVE(context->internal->_servers, srv);

#ifdef _XBOX // try cleanly first
  if( !cli_tdis(&srv->cli) )
#endif
	cli_shutdown(&srv->cli);


	DEBUG(3, ("smbc_remove_usused_server: %p removed.\n", srv));

	context->callbacks.remove_cached_srv_fn(context, srv);

        SAFE_FREE(srv);
	
	return 0;
}

static SMBCSRV *
find_server(SMBCCTX *context,
            const char *server,
            const char *share,
            fstring workgroup,
            fstring username,
            fstring password)
{
        SMBCSRV *srv;
        int auth_called = 0;
        
 check_server_cache:

	srv = context->callbacks.get_cached_srv_fn(context, server, share, 
						   workgroup, username);

	if (!auth_called && !srv && (!username[0] || !password[0])) {
                if (context->internal->_auth_fn_with_context != NULL) {
                         context->internal->_auth_fn_with_context(
                                context,
                                server, share,
                                workgroup, sizeof(fstring),
                                username, sizeof(fstring),
                                password, sizeof(fstring));
                } else {
                        context->callbacks.auth_fn(
                                server, share,
                                workgroup, sizeof(fstring),
                                username, sizeof(fstring),
                                password, sizeof(fstring));
                }

		/*
                 * However, smbc_auth_fn may have picked up info relating to
                 * an existing connection, so try for an existing connection
                 * again ...
                 */
		auth_called = 1;
		goto check_server_cache;
		
	}
	
	if (srv) {
		if (context->callbacks.check_server_fn(context, srv)) {
			/*
                         * This server is no good anymore 
                         * Try to remove it and check for more possible
                         * servers in the cache
                         */
			if (context->callbacks.remove_unused_server_fn(context,
                                                                       srv)) { 
                                /*
                                 * We could not remove the server completely,
                                 * remove it from the cache so we will not get
                                 * it again. It will be removed when the last
                                 * file/dir is closed.
                                 */
				context->callbacks.remove_cached_srv_fn(context,
                                                                        srv);
			}
			
			/*
                         * Maybe there are more cached connections to this
                         * server
                         */
			goto check_server_cache; 
		}

		return srv;
 	}

        return NULL;
}

/*
 * Connect to a server, possibly on an existing connection
 *
 * Here, what we want to do is: If the server and username
 * match an existing connection, reuse that, otherwise, establish a 
 * new connection.
 *
 * If we have to create a new connection, call the auth_fn to get the
 * info we need, unless the username and password were passed in.
 */

static SMBCSRV *
smbc_server(SMBCCTX *context,
            BOOL connect_if_not_found,
            const char *server,
            const char *share, 
            fstring workgroup,
            fstring username, 
            fstring password)
{
	SMBCSRV *srv=NULL;
	struct cli_state c;
	struct nmb_name called, calling;
	const char *server_n = server;
	pstring ipenv;
	struct in_addr ip;
	int tried_reverse = 0;
        int port_try_first;
        int port_try_next;
        const char *username_used;
  
	zero_ip(&ip);
	ZERO_STRUCT(c);

	if (server[0] == 0) {
		errno = EPERM;
		return NULL;
	}

        /* Look for a cached connection */
        srv = find_server(context, server, share,
                          workgroup, username, password);
        
        /*
         * If we found a connection and we're only allowed one share per
         * server...
         */
        if (srv && *share != '\0' && context->options.one_share_per_server) {

                /*
                 * ... then if there's no current connection to the share,
                 * connect to it.  find_server(), or rather the function
                 * pointed to by context->callbacks.get_cached_srv_fn which
                 * was called by find_server(), will have issued a tree
                 * disconnect if the requested share is not the same as the
                 * one that was already connected.
                 */
                if (srv->cli.cnum == (uint16) -1) {
                        /* Ensure we have accurate auth info */
                        if (context->internal->_auth_fn_with_context != NULL) {
                                context->internal->_auth_fn_with_context(
                                        context,
                                        server, share,
                                        workgroup, sizeof(fstring),
                                        username, sizeof(fstring),
                                        password, sizeof(fstring));
                        } else {
                                context->callbacks.auth_fn(
                                        server, share,
                                        workgroup, sizeof(fstring),
                                        username, sizeof(fstring),
                                        password, sizeof(fstring));
                        }

                        if (! cli_send_tconX(&srv->cli, share, "?????",
                                             password, strlen(password)+1)) {
                        
                                errno = smbc_errno(context, &srv->cli);
                                cli_shutdown(&srv->cli);
                                context->callbacks.remove_cached_srv_fn(context,
                                                                        srv);
                                srv = NULL;
                        }

                        /*
                         * Regenerate the dev value since it's based on both
                         * server and share
                         */
                        if (srv) {
                                srv->dev = (dev_t)(str_checksum(server) ^
                                                   str_checksum(share));
                        }
                }
        }
        
        /* If we have a connection... */
        if (srv) {

                /* ... then we're done here.  Give 'em what they came for. */
                return srv;
        }

        /* If we're not asked to connect when a connection doesn't exist... */
        if (! connect_if_not_found) {
                /* ... then we're done here. */
                return NULL;
        }

	make_nmb_name(&calling, context->netbios_name, 0x0);
	make_nmb_name(&called , server, 0x20);

	DEBUG(4,("smbc_server: server_n=[%s] server=[%s]\n", server_n, server));
  
	DEBUG(4,(" -> server_n=[%s] server=[%s]\n", server_n, server));

 again:
	slprintf(ipenv,sizeof(ipenv)-1,"HOST_%s", server_n);

	zero_ip(&ip);

	/* have to open a new connection */
	if (!cli_initialise(&c)) {
		errno = ENOMEM;
		return NULL;
	}

	if (context->flags & SMB_CTX_FLAG_USE_KERBEROS) {
		c.use_kerberos = True;
	}
	if (context->flags & SMB_CTX_FLAG_FALLBACK_AFTER_KERBEROS) {
		c.fallback_after_kerberos = True;
	}

	c.timeout = context->timeout;

        /*
         * Force use of port 139 for first try if share is $IPC, empty, or
         * null, so browse lists can work
         */
        if (share == NULL || *share == '\0' || strcmp(share, "IPC$") == 0) {
                port_try_first = 139;
                port_try_next = 445;
        } else {
                port_try_first = 445;
                port_try_next = 139;
        }

        c.port = port_try_first;

	if (!cli_connect(&c, server_n, &ip)) {

                /* First connection attempt failed.  Try alternate port. */
                c.port = port_try_next;

                if (!cli_connect(&c, server_n, &ip)) {
                        cli_shutdown(&c);
                        errno = ETIMEDOUT;
                        return NULL;
                }
 	}

	if (!cli_session_request(&c, &calling, &called)) {
		cli_shutdown(&c);
		if (strcmp(called.name, "*SMBSERVER")) {
			make_nmb_name(&called , "*SMBSERVER", 0x20);
			goto again;
		}
		else {  /* Try one more time, but ensure we don't loop */

		  /* Only try this if server is an IP address ... */

		  if (is_ipaddress(server) && !tried_reverse) {
		    fstring remote_name;
		    struct in_addr rem_ip;

		    if ((rem_ip.s_addr=inet_addr(server)) == INADDR_NONE) {
		      DEBUG(4, ("Could not convert IP address %s to struct in_addr\n", server));
		      errno = ETIMEDOUT;
		      return NULL;
		    }

		    tried_reverse++; /* Yuck */

		    if (name_status_find("*", 0, 0, rem_ip, remote_name)) {
		      make_nmb_name(&called, remote_name, 0x20);
		      goto again;
		    }


		  }
		}
		errno = ETIMEDOUT;
		return NULL;
	}
  
	DEBUG(4,(" session request ok\n"));
  
	if (!cli_negprot(&c)) {
		cli_shutdown(&c);
		errno = ETIMEDOUT;
		return NULL;
	}

        username_used = username;

	if (!cli_session_setup(&c, username_used, 
			       password, strlen(password),
			       password, strlen(password),
			       workgroup)) {
                
                /* Failed.  Try an anonymous login, if allowed by flags. */
                username_used = "";

                if ((context->flags & SMBCCTX_FLAG_NO_AUTO_ANONYMOUS_LOGON) ||
                     !cli_session_setup(&c, username_used,
                                        password, 1,
                                        password, 0,
                                        workgroup)) {

                        cli_shutdown(&c);
                        errno = EPERM;
                        return NULL;
                }
	}

	DEBUG(4,(" session setup ok\n"));

	if (!cli_send_tconX(&c, share, "?????",
			    password, strlen(password)+1)) {
		errno = smbc_errno(context, &c);
		cli_shutdown(&c);
		return NULL;
	}
  
	DEBUG(4,(" tconx ok\n"));
  
	/*
	 * Ok, we have got a nice connection
	 * Let's allocate a server structure.
	 */

	srv = SMB_MALLOC_P(SMBCSRV);
	if (!srv) {
		errno = ENOMEM;
		goto failed;
	}

	ZERO_STRUCTP(srv);
	srv->cli = c;
        srv->cli.allocated = False;
	srv->dev = (dev_t)(str_checksum(server) ^ str_checksum(share));
        srv->no_pathinfo = False;
        srv->no_pathinfo2 = False;
        srv->no_nt_session = False;
#ifdef _XBOX
		gettimeofday(&srv->access, NULL);
#endif
	/* now add it to the cache (internal or external)  */
	/* Let the cache function set errno if it wants to */
	errno = 0;
	if (context->callbacks.add_cached_srv_fn(context, srv, server, share, workgroup, username)) {
		int saved_errno = errno;
		DEBUG(3, (" Failed to add server to cache\n"));
		errno = saved_errno;
		if (errno == 0) {
			errno = ENOMEM;
		}
		goto failed;
	}
	
	DEBUG(2, ("Server connect ok: //%s/%s: %p\n", 
		  server, share, srv));

	DLIST_ADD(context->internal->_servers, srv);
	return srv;

 failed:
	cli_shutdown(&c);
	if (!srv) return NULL;
  
	SAFE_FREE(srv);
	return NULL;
}

/*
 * Connect to a server for getting/setting attributes, possibly on an existing
 * connection.  This works similarly to smbc_server().
 */
static SMBCSRV *
smbc_attr_server(SMBCCTX *context,
                 const char *server,
                 const char *share, 
                 fstring workgroup,
                 fstring username,
                 fstring password,
                 POLICY_HND *pol)
{
        struct in_addr ip;
	struct cli_state *ipc_cli;
	struct rpc_pipe_client *pipe_hnd;
        NTSTATUS nt_status;
	SMBCSRV *ipc_srv=NULL;

        /*
         * See if we've already created this special connection.  Reference
         * our "special" share name '*IPC$', which is an impossible real share
         * name due to the leading asterisk.
         */
        ipc_srv = find_server(context, server, "*IPC$",
                              workgroup, username, password);
        if (!ipc_srv) {

                /* We didn't find a cached connection.  Get the password */
                if (*password == '\0') {
                        /* ... then retrieve it now. */
                        if (context->internal->_auth_fn_with_context != NULL) {
                                context->internal->_auth_fn_with_context(
                                        context,
                                        server, share,
                                        workgroup, sizeof(fstring),
                                        username, sizeof(fstring),
                                        password, sizeof(fstring));
                        } else {
                                context->callbacks.auth_fn(
                                        server, share,
                                        workgroup, sizeof(fstring),
                                        username, sizeof(fstring),
                                        password, sizeof(fstring));
                        }
                }
        
                zero_ip(&ip);
                nt_status = cli_full_connection(&ipc_cli,
                                                global_myname(), server, 
                                                &ip, 0, "IPC$", "?????",  
                                                username, workgroup,
                                                password, 0,
                                                Undefined, NULL);
                if (! NT_STATUS_IS_OK(nt_status)) {
                        DEBUG(1,("cli_full_connection failed! (%s)\n",
                                 nt_errstr(nt_status)));
                        errno = ENOTSUP;
                        return NULL;
                }

                ipc_srv = SMB_MALLOC_P(SMBCSRV);
                if (!ipc_srv) {
                        errno = ENOMEM;
                        cli_shutdown(ipc_cli);
                        return NULL;
                }

                ZERO_STRUCTP(ipc_srv);
                ipc_srv->cli = *ipc_cli;
                ipc_srv->cli.allocated = False;

                free(ipc_cli);

                if (pol) {
                        pipe_hnd = cli_rpc_pipe_open_noauth(&ipc_srv->cli,
                                                            PI_LSARPC,
                                                            &nt_status);
                        if (!pipe_hnd) {
                                DEBUG(1, ("cli_nt_session_open fail!\n"));
                                errno = ENOTSUP;
                                cli_shutdown(&ipc_srv->cli);
                                free(ipc_srv);
                                return NULL;
                        }

                        /*
                         * Some systems don't support
                         * SEC_RIGHTS_MAXIMUM_ALLOWED, but NT sends 0x2000000
                         * so we might as well do it too.
                         */
        
                        nt_status = rpccli_lsa_open_policy(
                                pipe_hnd,
                                ipc_srv->cli.mem_ctx,
                                True, 
                                GENERIC_EXECUTE_ACCESS,
                                pol);
        
                        if (!NT_STATUS_IS_OK(nt_status)) {
                                errno = smbc_errno(context, &ipc_srv->cli);
                                cli_shutdown(&ipc_srv->cli);
                                return NULL;
                        }
                }

                /* now add it to the cache (internal or external) */

                errno = 0;      /* let cache function set errno if it likes */
                if (context->callbacks.add_cached_srv_fn(context, ipc_srv,
                                                         server,
                                                         "*IPC$",
                                                         workgroup,
                                                         username)) {
                        DEBUG(3, (" Failed to add server to cache\n"));
                        if (errno == 0) {
                                errno = ENOMEM;
                        }
                        cli_shutdown(&ipc_srv->cli);
                        free(ipc_srv);
                        return NULL;
                }

                DLIST_ADD(context->internal->_servers, ipc_srv);
        }

        return ipc_srv;
}

/*
 * Routine to open() a file ...
 */

static SMBCFILE *
smbc_open_ctx(SMBCCTX *context,
              const char *fname,
              int flags,
              mode_t mode)
{
	fstring server, share, user, password, workgroup;
	pstring path;
        pstring targetpath;
	struct cli_state *targetcli;
	SMBCSRV *srv   = NULL;
	SMBCFILE *file = NULL;
	int fd;

	if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;  /* Best I can think of ... */
		return NULL;

	}

	if (!fname) {

		errno = EINVAL;
		return NULL;

	}

	if (smbc_parse_path(context, fname,
                            workgroup, sizeof(workgroup),
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            NULL, 0)) {
                errno = EINVAL;
                return NULL;
        }

	if (user[0] == (char)0) fstrcpy(user, context->user);

	srv = smbc_server(context, True,
                          server, share, workgroup, user, password);

	if (!srv) {

		if (errno == EPERM) errno = EACCES;
		return NULL;  /* smbc_server sets errno */
    
	}

	/* Hmmm, the test for a directory is suspect here ... FIXME */

	if (strlen(path) > 0 && path[strlen(path) - 1] == '\\') {
    
		fd = -1;

	}
	else {
	  
		file = SMB_MALLOC_P(SMBCFILE);

		if (!file) {

			errno = ENOMEM;
			return NULL;

		}

		ZERO_STRUCTP(file);

		/*d_printf(">>>open: resolving %s\n", path);*/
		if (!cli_resolve_path( "", &srv->cli, path, &targetcli, targetpath))
		{
			d_printf("Could not resolve %s\n", path);
			SAFE_FREE(file);
			return NULL;
		}
		/*d_printf(">>>open: resolved %s as %s\n", path, targetpath);*/
		
		if ( targetcli->dfsroot )
		{
			pstring temppath;
			pstrcpy(temppath, targetpath);
			cli_dfs_make_full_path( targetpath, targetcli->desthost, targetcli->share, temppath);
		}
		
		if ((fd = cli_open(targetcli, targetpath, flags, DENY_NONE)) < 0) {

			/* Handle the error ... */

			SAFE_FREE(file);
			errno = smbc_errno(context, targetcli);
			return NULL;

		}

		/* Fill in file struct */

		file->cli_fd  = fd;
		file->fname   = SMB_STRDUP(fname);
		file->srv     = srv;
		file->offset  = 0;
		file->file    = True;

		DLIST_ADD(context->internal->_files, file);

                /*
                 * If the file was opened in O_APPEND mode, all write
                 * operations should be appended to the file.  To do that,
                 * though, using this protocol, would require a getattrE()
                 * call for each and every write, to determine where the end
                 * of the file is. (There does not appear to be an append flag
                 * in the protocol.)  Rather than add all of that overhead of
                 * retrieving the current end-of-file offset prior to each
                 * write operation, we'll assume that most append operations
                 * will continuously write, so we'll just set the offset to
                 * the end of the file now and hope that's adequate.
                 *
                 * Note to self: If this proves inadequate, and O_APPEND
                 * should, in some cases, be forced for each write, add a
                 * field in the context options structure, for
                 * "strict_append_mode" which would select between the current
                 * behavior (if FALSE) or issuing a getattrE() prior to each
                 * write and forcing the write to the end of the file (if
                 * TRUE).  Adding that capability will likely require adding
                 * an "append" flag into the _SMBCFILE structure to track
                 * whether a file was opened in O_APPEND mode.  -- djl
                 */
                if (flags & O_APPEND) {
                        if (smbc_lseek_ctx(context, file, 0, SEEK_END) < 0) {
                                (void) smbc_close_ctx(context, file);
                                errno = ENXIO;
                                return NULL;
                        }
                }

		return file;

	}

	/* Check if opendir needed ... */

	if (fd == -1) {
		int eno = 0;

		eno = smbc_errno(context, &srv->cli);
		file = context->opendir(context, fname);
		if (!file) errno = eno;
		return file;

	}

	errno = EINVAL; /* FIXME, correct errno ? */
	return NULL;

}

/*
 * Routine to create a file 
 */

static int creat_bits = O_WRONLY | O_CREAT | O_TRUNC; /* FIXME: Do we need this */

static SMBCFILE *
smbc_creat_ctx(SMBCCTX *context,
               const char *path,
               mode_t mode)
{

	if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;
		return NULL;

	}

	return smbc_open_ctx(context, path, creat_bits, mode);
}

/*
 * Routine to read() a file ...
 */

static ssize_t
smbc_read_ctx(SMBCCTX *context,
              SMBCFILE *file,
              void *buf,
              size_t count)
{
	int ret;
	fstring server, share, user, password;
	pstring path, targetpath;
	struct cli_state *targetcli;

        /*
         * offset:
         *
         * Compiler bug (possibly) -- gcc (GCC) 3.3.5 (Debian 1:3.3.5-2) --
         * appears to pass file->offset (which is type off_t) differently than
         * a local variable of type off_t.  Using local variable "offset" in
         * the call to cli_read() instead of file->offset fixes a problem
         * retrieving data at an offset greater than 4GB.
         */
        SMB_OFF_T offset;

	if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;
		return -1;

	}

	DEBUG(4, ("smbc_read(%p, %d)\n", file, (int)count));

	if (!file || !DLIST_CONTAINS(context->internal->_files, file)) {

		errno = EBADF;
		return -1;

	}

	offset = file->offset;

	/* Check that the buffer exists ... */

	if (buf == NULL) {

		errno = EINVAL;
		return -1;

	}

	/*d_printf(">>>read: parsing %s\n", file->fname);*/
	if (smbc_parse_path(context, file->fname,
                            NULL, 0,
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            NULL, 0)) {
                errno = EINVAL;
                return -1;
        }
	
	/*d_printf(">>>read: resolving %s\n", path);*/
	if (!cli_resolve_path("", &file->srv->cli, path,
                              &targetcli, targetpath))
	{
		d_printf("Could not resolve %s\n", path);
		return -1;
	}
	/*d_printf(">>>fstat: resolved path as %s\n", targetpath);*/
	
	ret = cli_read(targetcli, file->cli_fd, buf, offset, count);

	if (ret < 0) {

		errno = smbc_errno(context, targetcli);
		return -1;

	}

	file->offset += ret;

	DEBUG(4, ("  --> %d\n", ret));

	return ret;  /* Success, ret bytes of data ... */

}

/*
 * Routine to write() a file ...
 */

static ssize_t
smbc_write_ctx(SMBCCTX *context,
               SMBCFILE *file,
               void *buf,
               size_t count)
{
	int ret;
        SMB_OFF_T offset;
	fstring server, share, user, password;
	pstring path, targetpath;
	struct cli_state *targetcli;

	/* First check all pointers before dereferencing them */
	
	if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;
		return -1;

	}

	if (!file || !DLIST_CONTAINS(context->internal->_files, file)) {

		errno = EBADF;
		return -1;
    
	}

	/* Check that the buffer exists ... */

	if (buf == NULL) {

		errno = EINVAL;
		return -1;

	}

        offset = file->offset; /* See "offset" comment in smbc_read_ctx() */

	/*d_printf(">>>write: parsing %s\n", file->fname);*/
	if (smbc_parse_path(context, file->fname,
                            NULL, 0,
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            NULL, 0)) {
                errno = EINVAL;
                return -1;
        }
	
	/*d_printf(">>>write: resolving %s\n", path);*/
	if (!cli_resolve_path("", &file->srv->cli, path,
                              &targetcli, targetpath))
	{
		d_printf("Could not resolve %s\n", path);
		return -1;
	}
	/*d_printf(">>>write: resolved path as %s\n", targetpath);*/


	ret = cli_write(targetcli, file->cli_fd, 0, buf, offset, count);

	if (ret <= 0) {

		errno = smbc_errno(context, targetcli);
		return -1;

	}

	file->offset += ret;

	return ret;  /* Success, 0 bytes of data ... */
}
 
/*
 * Routine to close() a file ...
 */

static int
smbc_close_ctx(SMBCCTX *context,
               SMBCFILE *file)
{
        SMBCSRV *srv; 
	fstring server, share, user, password;
	pstring path, targetpath;
	struct cli_state *targetcli;

	if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;
		return -1;

	}

	if (!file || !DLIST_CONTAINS(context->internal->_files, file)) {
   
		errno = EBADF;
		return -1;

	}

	/* IS a dir ... */
	if (!file->file) {
		
		return context->closedir(context, file);

	}

	/*d_printf(">>>close: parsing %s\n", file->fname);*/
	if (smbc_parse_path(context, file->fname,
                            NULL, 0,
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            NULL, 0)) {
                errno = EINVAL;
                return -1;
        }
	
	/*d_printf(">>>close: resolving %s\n", path);*/
	if (!cli_resolve_path("", &file->srv->cli, path,
                              &targetcli, targetpath))
	{
		d_printf("Could not resolve %s\n", path);
		return -1;
	}
	/*d_printf(">>>close: resolved path as %s\n", targetpath);*/

	if (!cli_close(targetcli, file->cli_fd)) {

		DEBUG(3, ("cli_close failed on %s. purging server.\n", 
			  file->fname));
		/* Deallocate slot and remove the server 
		 * from the server cache if unused */
		errno = smbc_errno(context, targetcli);
		srv = file->srv;
		DLIST_REMOVE(context->internal->_files, file);
		SAFE_FREE(file->fname);
		SAFE_FREE(file);
		context->callbacks.remove_unused_server_fn(context, srv);

		return -1;

	}

	DLIST_REMOVE(context->internal->_files, file);
	SAFE_FREE(file->fname);
	SAFE_FREE(file);

	return 0;
}

/*
 * Get info from an SMB server on a file. Use a qpathinfo call first
 * and if that fails, use getatr, as Win95 sometimes refuses qpathinfo
 */
static BOOL
smbc_getatr(SMBCCTX * context,
            SMBCSRV *srv,
            char *path, 
            uint16 *mode,
            SMB_OFF_T *size, 
            time_t *c_time,
            time_t *a_time,
            time_t *m_time,
            SMB_INO_T *ino)
{
	pstring fixedpath;
	pstring targetpath;
	struct cli_state *targetcli;

	if (!context || !context->internal ||
	    !context->internal->_initialized) {
 
		errno = EINVAL;
 		return -1;
 
 	}

	/* path fixup for . and .. */
	if (strequal(path, ".") || strequal(path, ".."))
		pstrcpy(fixedpath, "\\");
	else
	{
		pstrcpy(fixedpath, path);
		trim_string(fixedpath, NULL, "\\..");
		trim_string(fixedpath, NULL, "\\.");
	}
	DEBUG(4,("smbc_getatr: sending qpathinfo\n"));
  
	if (!cli_resolve_path( "", &srv->cli, fixedpath, &targetcli, targetpath))
	{
		d_printf("Couldn't resolve %s\n", path);
		return False;
	}
	
	if ( targetcli->dfsroot )
	{
		pstring temppath;
		pstrcpy(temppath, targetpath);
		cli_dfs_make_full_path(targetpath, targetcli->desthost,
                                       targetcli->share, temppath);
	}
  
	if (!srv->no_pathinfo2 &&
            cli_qpathinfo2(targetcli, targetpath,
                           NULL, a_time, m_time, c_time, size, mode, ino)) {
            return True;
        }

	/* if this is NT then don't bother with the getatr */
	if (targetcli->capabilities & CAP_NT_SMBS) {
                errno = EPERM;
                return False;
        }

	if (cli_getatr(targetcli, targetpath, mode, size, m_time)) {
                if (m_time != NULL) {
                        if (a_time != NULL) *a_time = *m_time;
                        if (c_time != NULL) *c_time = *m_time;
                }
		srv->no_pathinfo2 = True;
		return True;
	}

        errno = EPERM;
	return False;

}

/*
 * Set file info on an SMB server.  Use setpathinfo call first.  If that
 * fails, use setattrE..
 *
 * Access and modification time parameters are always used and must be
 * provided.  Create time, if zero, will be determined from the actual create
 * time of the file.  If non-zero, the create time will be set as well.
 *
 * "mode" (attributes) parameter may be set to -1 if it is not to be set.
 */
static BOOL
smbc_setatr(SMBCCTX * context, SMBCSRV *srv, char *path, 
            time_t c_time, time_t a_time, time_t m_time,
            uint16 mode)
{
        int fd;
        int ret;

        /*
         * First, try setpathinfo (if qpathinfo succeeded), for it is the
         * modern function for "new code" to be using, and it works given a
         * filename rather than requiring that the file be opened to have its
         * attributes manipulated.
         */
        if (srv->no_pathinfo ||
            ! cli_setpathinfo(&srv->cli, path, c_time, a_time, m_time, mode)) {

                /*
                 * setpathinfo is not supported; go to plan B. 
                 *
                 * cli_setatr() does not work on win98, and it also doesn't
                 * support setting the access time (only the modification
                 * time), so in all cases, we open the specified file and use
                 * cli_setattrE() which should work on all OS versions, and
                 * supports both times.
                 */

                /* Don't try {q,set}pathinfo() again, with this server */
                srv->no_pathinfo = True;

                /* Open the file */
                if ((fd = cli_open(&srv->cli, path, O_RDWR, DENY_NONE)) < 0) {

                        errno = smbc_errno(context, &srv->cli);
                        return -1;
                }

                /*
                 * Get the creat time of the file (if it wasn't provided).
                 * We'll need it in the set call
                 */
                if (c_time == 0) {
                        ret = cli_getattrE(&srv->cli, fd,
                                           NULL, NULL,
                                           &c_time, NULL, NULL);
                } else {
                        ret = True;
                }
                    
                /* If we got create time, set times */
                if (ret) {
                        /* Some OS versions don't support create time */
                        if (c_time == 0 || c_time == -1) {
                                c_time = time(NULL);
                        }

                        /*
                         * For sanity sake, since there is no POSIX function
                         * to set the create time of a file, if the existing
                         * create time is greater than either of access time
                         * or modification time, set create time to the
                         * smallest of those.  This ensure that the create
                         * time of a file is never greater than its last
                         * access or modification time.
                         */
                        if (c_time > a_time) c_time = a_time;
                        if (c_time > m_time) c_time = m_time;
                        
                        /* Set the new attributes */
                        ret = cli_setattrE(&srv->cli, fd,
                                           c_time, a_time, m_time);
                        cli_close(&srv->cli, fd);
                }

                /*
                 * Unfortunately, setattrE() doesn't have a provision for
                 * setting the access mode (attributes).  We'll have to try
                 * cli_setatr() for that, and with only this parameter, it
                 * seems to work on win98.
                 */
                if (ret && mode != (uint16) -1) {
                        ret = cli_setatr(&srv->cli, path, mode, 0);
                }

                if (! ret) {
                        errno = smbc_errno(context, &srv->cli);
                        return False;
                }
        }

        return True;
}

 /*
  * Routine to unlink() a file
  */

static int
smbc_unlink_ctx(SMBCCTX *context,
                const char *fname)
{
	fstring server, share, user, password, workgroup;
	pstring path, targetpath;
	struct cli_state *targetcli;
	SMBCSRV *srv = NULL;

	if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;  /* Best I can think of ... */
		return -1;

	}

	if (!fname) {

		errno = EINVAL;
		return -1;

	}

	if (smbc_parse_path(context, fname,
                            workgroup, sizeof(workgroup),
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            NULL, 0)) {
                errno = EINVAL;
                return -1;
        }

	if (user[0] == (char)0) fstrcpy(user, context->user);

	srv = smbc_server(context, True,
                          server, share, workgroup, user, password);

	if (!srv) {

		return -1;  /* smbc_server sets errno */

	}

	/*d_printf(">>>unlink: resolving %s\n", path);*/
	if (!cli_resolve_path( "", &srv->cli, path, &targetcli, targetpath))
	{
		d_printf("Could not resolve %s\n", path);
		return -1;
	}
	/*d_printf(">>>unlink: resolved path as %s\n", targetpath);*/

	if (!cli_unlink(targetcli, targetpath)) {

		errno = smbc_errno(context, targetcli);

		if (errno == EACCES) { /* Check if the file is a directory */

			int saverr = errno;
			SMB_OFF_T size = 0;
			uint16 mode = 0;
			time_t m_time = 0, a_time = 0, c_time = 0;
			SMB_INO_T ino = 0;

			if (!smbc_getatr(context, srv, path, &mode, &size,
					 &c_time, &a_time, &m_time, &ino)) {

				/* Hmmm, bad error ... What? */

				errno = smbc_errno(context, targetcli);
				return -1;

			}
			else {

				if (IS_DOS_DIR(mode))
					errno = EISDIR;
				else
					errno = saverr;  /* Restore this */

			}
		}

		return -1;

	}

	return 0;  /* Success ... */

}

/*
 * Routine to rename() a file
 */

static int
smbc_rename_ctx(SMBCCTX *ocontext,
                const char *oname, 
                SMBCCTX *ncontext,
                const char *nname)
{
	fstring server1;
        fstring share1;
        fstring server2;
        fstring share2;
        fstring user1;
        fstring user2;
        fstring password1;
        fstring password2;
        fstring workgroup;
	pstring path1;
        pstring path2;
        pstring targetpath1;
        pstring targetpath2;
	struct cli_state *targetcli1;
        struct cli_state *targetcli2;
	SMBCSRV *srv = NULL;

	if (!ocontext || !ncontext || 
	    !ocontext->internal || !ncontext->internal ||
	    !ocontext->internal->_initialized || 
	    !ncontext->internal->_initialized) {

		errno = EINVAL;  /* Best I can think of ... */
		return -1;

	}
	
	if (!oname || !nname) {

		errno = EINVAL;
		return -1;

	}
	
	DEBUG(4, ("smbc_rename(%s,%s)\n", oname, nname));

	smbc_parse_path(ocontext, oname,
                        workgroup, sizeof(workgroup),
                        server1, sizeof(server1),
                        share1, sizeof(share1),
                        path1, sizeof(path1),
                        user1, sizeof(user1),
                        password1, sizeof(password1),
                        NULL, 0);

	if (user1[0] == (char)0) fstrcpy(user1, ocontext->user);

	smbc_parse_path(ncontext, nname,
                        NULL, 0,
                        server2, sizeof(server2),
                        share2, sizeof(share2),
                        path2, sizeof(path2),
                        user2, sizeof(user2),
                        password2, sizeof(password2),
                        NULL, 0);

	if (user2[0] == (char)0) fstrcpy(user2, ncontext->user);

	if (strcmp(server1, server2) || strcmp(share1, share2) ||
	    strcmp(user1, user2)) {

		/* Can't rename across file systems, or users?? */

		errno = EXDEV;
		return -1;

	}

	srv = smbc_server(ocontext, True,
                          server1, share1, workgroup, user1, password1);
	if (!srv) {

		return -1;

	}

	/*d_printf(">>>rename: resolving %s\n", path1);*/
	if (!cli_resolve_path( "", &srv->cli, path1, &targetcli1, targetpath1))
	{
		d_printf("Could not resolve %s\n", path1);
		return -1;
	}
	/*d_printf(">>>rename: resolved path as %s\n", targetpath1);*/
	/*d_printf(">>>rename: resolving %s\n", path2);*/
	if (!cli_resolve_path( "", &srv->cli, path2, &targetcli2, targetpath2))
	{
		d_printf("Could not resolve %s\n", path2);
		return -1;
	}
	/*d_printf(">>>rename: resolved path as %s\n", targetpath2);*/
	
	if (strcmp(targetcli1->desthost, targetcli2->desthost) ||
            strcmp(targetcli1->share, targetcli2->share))
	{
		/* can't rename across file systems */
		
		errno = EXDEV;
		return -1;
	}

	if (!cli_rename(targetcli1, targetpath1, targetpath2)) {
		int eno = smbc_errno(ocontext, targetcli1);

		if (eno != EEXIST ||
		    !cli_unlink(targetcli1, targetpath2) ||
		    !cli_rename(targetcli1, targetpath1, targetpath2)) {

			errno = eno;
			return -1;

		}
	}

	return 0; /* Success */

}

/*
 * A routine to lseek() a file
 */

static SMB_OFF_T
smbc_lseek_ctx(SMBCCTX *context,
               SMBCFILE *file,
               SMB_OFF_T offset,
               int whence)
{
	SMB_OFF_T size;
	fstring server, share, user, password;
	pstring path, targetpath;
	struct cli_state *targetcli;

	if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;
		return -1;
		
	}

	if (!file || !DLIST_CONTAINS(context->internal->_files, file)) {

		errno = EBADF;
		return -1;

	}

	if (!file->file) {

		errno = EINVAL;
		return -1;      /* Can't lseek a dir ... */

	}

	switch (whence) {
	case SEEK_SET:
		file->offset = offset;
		break;

	case SEEK_CUR:
		file->offset += offset;
		break;

	case SEEK_END:
		/*d_printf(">>>lseek: parsing %s\n", file->fname);*/
		if (smbc_parse_path(context, file->fname,
                                    NULL, 0,
                                    server, sizeof(server),
                                    share, sizeof(share),
                                    path, sizeof(path),
                                    user, sizeof(user),
                                    password, sizeof(password),
                                    NULL, 0)) {
                        
					errno = EINVAL;
					return -1;
			}
		
		/*d_printf(">>>lseek: resolving %s\n", path);*/
		if (!cli_resolve_path("", &file->srv->cli, path,
                                      &targetcli, targetpath))
		{
			d_printf("Could not resolve %s\n", path);
			return -1;
		}
		/*d_printf(">>>lseek: resolved path as %s\n", targetpath);*/
		
		if (!cli_qfileinfo(targetcli, file->cli_fd, NULL,
                                   &size, NULL, NULL, NULL, NULL, NULL)) 
		{
		    SMB_OFF_T b_size = size;
			if (!cli_getattrE(targetcli, file->cli_fd,
                                          NULL, &b_size, NULL, NULL, NULL)) 
		    {
			errno = EINVAL;
			return -1;
		    } else
			size = b_size;
		}
		file->offset = size + offset;
		break;

	default:
		errno = EINVAL;
		break;

	}

	return file->offset;

}

/* 
 * Generate an inode number from file name for those things that need it
 */

static ino_t
smbc_inode(SMBCCTX *context,
           const char *name)
{

	if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;
		return -1;

	}

	if (!*name) return 2; /* FIXME, why 2 ??? */
	return (ino_t)str_checksum(name);

}

/*
 * Routine to put basic stat info into a stat structure ... Used by stat and
 * fstat below.
 */

static int
smbc_setup_stat(SMBCCTX *context,
                SMB_STRUCT_STAT *st,
                char *fname,
                SMB_OFF_T size,
                int mode)
{
	
	st->st_mode = 0;

#ifdef _XBOX /* otherwise we can't check for hidden later */
	if (IS_DOS_DIR(mode))
    st->st_mode = S_IFDIR;
	else
		st->st_mode = S_IFREG;
#else
	if (IS_DOS_DIR(mode)) {
		st->st_mode = SMBC_DIR_MODE;
	} else {
		st->st_mode = SMBC_FILE_MODE;
	}
#endif

	if (IS_DOS_ARCHIVE(mode)) st->st_mode |= S_IXUSR;
	if (IS_DOS_SYSTEM(mode)) st->st_mode |= S_IXGRP;
	if (IS_DOS_HIDDEN(mode)) st->st_mode |= S_IXOTH;
	if (!IS_DOS_READONLY(mode)) st->st_mode |= S_IWUSR;

	st->st_size = size;
#ifdef HAVE_STAT_ST_BLKSIZE
	st->st_blksize = 512;
#endif
#ifdef HAVE_STAT_ST_BLOCKS
	st->st_blocks = (size+511)/512;
#endif
	st->st_uid = getuid();
	st->st_gid = getgid();

	if (IS_DOS_DIR(mode)) {
		st->st_nlink = 2;
	} else {
		st->st_nlink = 1;
	}

	if (st->st_ino == 0) {
		st->st_ino = smbc_inode(context, fname);
	}
	
	return True;  /* FIXME: Is this needed ? */

}

/*
 * Routine to stat a file given a name
 */

static int
smbc_stat_ctx(SMBCCTX *context,
              const char *fname,
              SMB_STRUCT_STAT *st)
{
	SMBCSRV *srv;
	fstring server;
        fstring share;
        fstring user;
        fstring password;
        fstring workgroup;
	pstring path;
	time_t m_time = 0;
        time_t a_time = 0;
        time_t c_time = 0;
	SMB_OFF_T size = 0;
	uint16 mode = 0;
	SMB_INO_T ino = 0;

	if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;  /* Best I can think of ... */
		return -1;
    
	}

	if (!fname) {

		errno = EINVAL;
		return -1;

	}
  
	DEBUG(4, ("smbc_stat(%s)\n", fname));

	if (smbc_parse_path(context, fname,
                            workgroup, sizeof(workgroup),
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            NULL, 0)) {
                errno = EINVAL;
                return -1;
        }

	if (user[0] == (char)0) fstrcpy(user, context->user);

	srv = smbc_server(context, True,
                          server, share, workgroup, user, password);

	if (!srv) {
		return -1;  /* errno set by smbc_server */
	}

	if (!smbc_getatr(context, srv, path, &mode, &size, 
			 &c_time, &a_time, &m_time, &ino)) {

		errno = smbc_errno(context, &srv->cli);
		return -1;
		
	}

	st->st_ino = ino;

	smbc_setup_stat(context, st, path, size, mode);

	st->st_atime = a_time;
	st->st_ctime = c_time;
	st->st_mtime = m_time;
	st->st_dev   = srv->dev;

	return 0;

}

/*
 * Routine to stat a file given an fd
 */

static int
smbc_fstat_ctx(SMBCCTX *context,
               SMBCFILE *file,
               SMB_STRUCT_STAT *st)
{
	time_t c_time;
        time_t a_time;
        time_t m_time;
	SMB_OFF_T size;
	uint16 mode;
	fstring server;
        fstring share;
        fstring user;
        fstring password;
	pstring path;
        pstring targetpath;
	struct cli_state *targetcli;
	SMB_INO_T ino = 0;

	if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;
		return -1;

	}

	if (!file || !DLIST_CONTAINS(context->internal->_files, file)) {

		errno = EBADF;
		return -1;

	}

	if (!file->file) {

		return context->fstatdir(context, file, st);

	}

	/*d_printf(">>>fstat: parsing %s\n", file->fname);*/
	if (smbc_parse_path(context, file->fname,
                            NULL, 0,
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            NULL, 0)) {
                errno = EINVAL;
                return -1;
        }
	
	/*d_printf(">>>fstat: resolving %s\n", path);*/
	if (!cli_resolve_path("", &file->srv->cli, path,
                              &targetcli, targetpath))
	{
		d_printf("Could not resolve %s\n", path);
		return -1;
	}
	/*d_printf(">>>fstat: resolved path as %s\n", targetpath);*/

	if (!cli_qfileinfo(targetcli, file->cli_fd, &mode, &size,
                           NULL, &a_time, &m_time, &c_time, &ino)) {
	    if (!cli_getattrE(targetcli, file->cli_fd, &mode, &size,
                              &c_time, &a_time, &m_time)) {

		errno = EINVAL;
		return -1;
	    }
	}

	st->st_ino = ino;

	smbc_setup_stat(context, st, file->fname, size, mode);

	st->st_atime = a_time;
	st->st_ctime = c_time;
	st->st_mtime = m_time;
	st->st_dev = file->srv->dev;

	return 0;

}

/*
 * Routine to open a directory
 * We accept the URL syntax explained in smbc_parse_path(), above.
 */

static void
smbc_remove_dir(SMBCFILE *dir)
{
	struct smbc_dir_list *d,*f;

	d = dir->dir_list;
	while (d) {

		f = d; d = d->next;

		SAFE_FREE(f->dirent);
		SAFE_FREE(f);

	}

	dir->dir_list = dir->dir_end = dir->dir_next = NULL;

}

static int
add_dirent(SMBCFILE *dir,
           const char *name,
           const char *comment,
           uint32 type)
{
	struct smbc_dirent *dirent;
	int size;
        int name_length = (name == NULL ? 0 : strlen(name));
        int comment_len = (comment == NULL ? 0 : strlen(comment));

	/*
	 * Allocate space for the dirent, which must be increased by the 
	 * size of the name and the comment and 1 each for the null terminator.
	 */

	size = sizeof(struct smbc_dirent) + name_length + comment_len + 2;
    
	dirent = SMB_MALLOC(size);

	if (!dirent) {

		dir->dir_error = ENOMEM;
		return -1;

	}

	ZERO_STRUCTP(dirent);

	if (dir->dir_list == NULL) {

		dir->dir_list = SMB_MALLOC_P(struct smbc_dir_list);
		if (!dir->dir_list) {

			SAFE_FREE(dirent);
			dir->dir_error = ENOMEM;
			return -1;

		}
		ZERO_STRUCTP(dir->dir_list);

		dir->dir_end = dir->dir_next = dir->dir_list;
	}
	else {

		dir->dir_end->next = SMB_MALLOC_P(struct smbc_dir_list);
		
		if (!dir->dir_end->next) {
			
			SAFE_FREE(dirent);
			dir->dir_error = ENOMEM;
			return -1;

		}
		ZERO_STRUCTP(dir->dir_end->next);

		dir->dir_end = dir->dir_end->next;
	}

	dir->dir_end->next = NULL;
	dir->dir_end->dirent = dirent;
	
	dirent->smbc_type = type;
	dirent->namelen = name_length;
	dirent->commentlen = comment_len;
	dirent->dirlen = size;
  
        /*
         * dirent->namelen + 1 includes the null (no null termination needed)
         * Ditto for dirent->commentlen.
         * The space for the two null bytes was allocated.
         */
	strncpy(dirent->name, (name?name:""), dirent->namelen + 1);
	dirent->comment = (char *)(&dirent->name + dirent->namelen + 1);
	strncpy(dirent->comment, (comment?comment:""), dirent->commentlen + 1);
	
	return 0;

}

static void
list_unique_wg_fn(const char *name,
                  uint32 type,
                  const char *comment,
                  void *state)
{
	SMBCFILE *dir = (SMBCFILE *)state;
        struct smbc_dir_list *dir_list;
        struct smbc_dirent *dirent;
	int dirent_type;
        int do_remove = 0;

	dirent_type = dir->dir_type;

	if (add_dirent(dir, name, comment, dirent_type) < 0) {

		/* An error occurred, what do we do? */
		/* FIXME: Add some code here */
	}

        /* Point to the one just added */
        dirent = dir->dir_end->dirent;

        /* See if this was a duplicate */
        for (dir_list = dir->dir_list;
             dir_list != dir->dir_end;
             dir_list = dir_list->next) {
                if (! do_remove &&
                    strcmp(dir_list->dirent->name, dirent->name) == 0) {
                        /* Duplicate.  End end of list need to be removed. */
                        do_remove = 1;
                }

                if (do_remove && dir_list->next == dir->dir_end) {
                        /* Found the end of the list.  Remove it. */
                        dir->dir_end = dir_list;
                        free(dir_list->next);
                        free(dirent);
                        dir_list->next = NULL;
                        break;
                }
        }
}

static void
list_fn(const char *name,
        uint32 type,
        const char *comment,
        void *state)
{
	SMBCFILE *dir = (SMBCFILE *)state;
	int dirent_type;

	/*
         * We need to process the type a little ...
         *
         * Disk share     = 0x00000000
         * Print share    = 0x00000001
         * Comms share    = 0x00000002 (obsolete?)
         * IPC$ share     = 0x00000003 
         *
         * administrative shares:
         * ADMIN$, IPC$, C$, D$, E$ ...  are type |= 0x80000000
         */
        
	if (dir->dir_type == SMBC_FILE_SHARE) {
		
		switch (type) {
                case 0 | 0x80000000:
		case 0:
			dirent_type = SMBC_FILE_SHARE;
			break;

		case 1:
			dirent_type = SMBC_PRINTER_SHARE;
			break;

		case 2:
			dirent_type = SMBC_COMMS_SHARE;
			break;

                case 3 | 0x80000000:
		case 3:
			dirent_type = SMBC_IPC_SHARE;
			break;

		default:
			dirent_type = SMBC_FILE_SHARE; /* FIXME, error? */
			break;
		}
	}
	else {
                dirent_type = dir->dir_type;
        }

	if (add_dirent(dir, name, comment, dirent_type) < 0) {

		/* An error occurred, what do we do? */
		/* FIXME: Add some code here */

	}
}

static void
dir_list_fn(const char *mnt,
            file_info *finfo,
            const char *mask,
            void *state)
{

	if (add_dirent((SMBCFILE *)state, finfo->name, "", 
		       (finfo->mode&aDIR?SMBC_DIR:SMBC_FILE)) < 0) {

		/* Handle an error ... */

		/* FIXME: Add some code ... */

	} 

}

static int
net_share_enum_rpc(struct cli_state *cli,
                   void (*fn)(const char *name,
                              uint32 type,
                              const char *comment,
                              void *state),
                   void *state)
{
        int i;
	WERROR result;
	ENUM_HND enum_hnd;
        uint32 info_level = 1;
	uint32 preferred_len = 0xffffffff;
        uint32 type;
	SRV_SHARE_INFO_CTR ctr;
	fstring name = "";
        fstring comment = "";
        void *mem_ctx;
	struct rpc_pipe_client *pipe_hnd;
        NTSTATUS nt_status;

        /* Open the server service pipe */
        pipe_hnd = cli_rpc_pipe_open_noauth(cli, PI_SRVSVC, &nt_status);
        if (!pipe_hnd) {
                DEBUG(1, ("net_share_enum_rpc pipe open fail!\n"));
                return -1;
        }

        /* Allocate a context for parsing and for the entries in "ctr" */
        mem_ctx = talloc_init("libsmbclient: net_share_enum_rpc");
        if (mem_ctx == NULL) {
                DEBUG(0, ("out of memory for net_share_enum_rpc!\n"));
                cli_rpc_pipe_close(pipe_hnd);
                return -1; 
        }

        /* Issue the NetShareEnum RPC call and retrieve the response */
	init_enum_hnd(&enum_hnd, 0);
	result = rpccli_srvsvc_net_share_enum(pipe_hnd,
                                              mem_ctx,
                                              info_level,
                                              &ctr,
                                              preferred_len,
                                              &enum_hnd);

        /* Was it successful? */
	if (!W_ERROR_IS_OK(result) || ctr.num_entries == 0) {
                /*  Nope.  Go clean up. */
		goto done;
        }

        /* For each returned entry... */
        for (i = 0; i < ctr.num_entries; i++) {

                /* pull out the share name */
                rpcstr_pull_unistr2_fstring(
                        name, &ctr.share.info1[i].info_1_str.uni_netname);

                /* pull out the share's comment */
                rpcstr_pull_unistr2_fstring(
                        comment, &ctr.share.info1[i].info_1_str.uni_remark);

                /* Get the type value */
                type = ctr.share.info1[i].info_1.type;

                /* Add this share to the list */
                (*fn)(name, type, comment, state);
        }

done:
        /* Close the server service pipe */
        cli_rpc_pipe_close(pipe_hnd);

        /* Free all memory which was allocated for this request */
        TALLOC_FREE(mem_ctx);

        /* Tell 'em if it worked */
        return W_ERROR_IS_OK(result) ? 0 : -1;
}



static SMBCFILE *
smbc_opendir_ctx(SMBCCTX *context,
                 const char *fname)
{
	fstring server, share, user, password, options;
	pstring workgroup;
	pstring path;
        uint16 mode;
        char *p;
	SMBCSRV *srv  = NULL;
	SMBCFILE *dir = NULL;
	struct in_addr rem_ip;

	if (!context || !context->internal ||
	    !context->internal->_initialized) {
	        DEBUG(4, ("no valid context\n"));
		errno = EINVAL + 8192;
		return NULL;

	}

	if (!fname) {
		DEBUG(4, ("no valid fname\n"));
		errno = EINVAL + 8193;
		return NULL;
	}

	if (smbc_parse_path(context, fname,
                            workgroup, sizeof(workgroup),
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            options, sizeof(options))) {
	        DEBUG(4, ("no valid path\n"));
		errno = EINVAL + 8194;
		return NULL;
	}

	DEBUG(4, ("parsed path: fname='%s' server='%s' share='%s' "
                  "path='%s' options='%s'\n",
                  fname, server, share, path, options));

        /* Ensure the options are valid */
        if (smbc_check_options(server, share, path, options)) {
                DEBUG(4, ("unacceptable options (%s)\n", options));
                errno = EINVAL + 8195;
                return NULL;
        }

	if (user[0] == (char)0) fstrcpy(user, context->user);

	dir = SMB_MALLOC_P(SMBCFILE);

	if (!dir) {

		errno = ENOMEM;
		return NULL;

	}

	ZERO_STRUCTP(dir);

	dir->cli_fd   = 0;
	dir->fname    = SMB_STRDUP(fname);
	dir->srv      = NULL;
	dir->offset   = 0;
	dir->file     = False;
	dir->dir_list = dir->dir_next = dir->dir_end = NULL;

	if (server[0] == (char)0) {

                int i;
                int count;
                int max_lmb_count;
                struct ip_service *ip_list;
                struct ip_service server_addr;
                struct user_auth_info u_info;
                struct cli_state *cli;

		if (share[0] != (char)0 || path[0] != (char)0) {

			errno = EINVAL + 8196;
			if (dir) {
				SAFE_FREE(dir->fname);
				SAFE_FREE(dir);
			}
			return NULL;
		}

                /* Determine how many local master browsers to query */
                max_lmb_count = (context->options.browse_max_lmb_count == 0
                                 ? INT_MAX
                                 : context->options.browse_max_lmb_count);

                pstrcpy(u_info.username, user);
                pstrcpy(u_info.password, password);

		/*
                 * We have server and share and path empty but options
                 * requesting that we scan all master browsers for their list
                 * of workgroups/domains.  This implies that we must first try
                 * broadcast queries to find all master browsers, and if that
                 * doesn't work, then try our other methods which return only
                 * a single master browser.
                 */

                ip_list = NULL;
                if (!name_resolve_bcast(MSBROWSE, 1, &ip_list, &count)) {

                        SAFE_FREE(ip_list);

                        if (!find_master_ip(workgroup, &server_addr.ip)) {

				if (dir) {
					SAFE_FREE(dir->fname);
					SAFE_FREE(dir);
				}
                                errno = ENOENT;
                                return NULL;
                        }

                        ip_list = &server_addr;
                        count = 1;
                }

                for (i = 0; i < count && i < max_lmb_count; i++) {
                        DEBUG(99, ("Found master browser %d of %d: %s\n",
                                   i+1, MAX(count, max_lmb_count),
                                   inet_ntoa(ip_list[i].ip)));
                        
                        cli = get_ipc_connect_master_ip(&ip_list[i],
                                                        workgroup, &u_info);
			/* cli == NULL is the master browser refused to talk or 
			   could not be found */
			if ( !cli )
				continue;

                        fstrcpy(server, cli->desthost);
                        cli_shutdown(cli);

                        DEBUG(4, ("using workgroup %s %s\n",
                                  workgroup, server));

                        /*
                         * For each returned master browser IP address, get a
                         * connection to IPC$ on the server if we do not
                         * already have one, and determine the
                         * workgroups/domains that it knows about.
                         */
                
                        srv = smbc_server(context, True, server, "IPC$",
                                          workgroup, user, password);
                        if (!srv) {
                                continue;
                        }
                
                        dir->srv = srv;
                        dir->dir_type = SMBC_WORKGROUP;

                        /* Now, list the stuff ... */
                        
                        if (!cli_NetServerEnum(&srv->cli,
                                               workgroup,
                                               SV_TYPE_DOMAIN_ENUM,
                                               list_unique_wg_fn,
                                               (void *)dir)) {
                                continue;
                        }
                }

                SAFE_FREE(ip_list);
        } else { 
                /*
                 * Server not an empty string ... Check the rest and see what
                 * gives
                 */
		if (*share == '\0') {
			if (*path != '\0') {

                                /* Should not have empty share with path */
				errno = EINVAL + 8197;
				if (dir) {
					SAFE_FREE(dir->fname);
					SAFE_FREE(dir);
				}
				return NULL;
	
			}

			/*
                         * We don't know if <server> is really a server name
                         * or is a workgroup/domain name.  If we already have
                         * a server structure for it, we'll use it.
                         * Otherwise, check to see if <server><1D>,
                         * <server><1B>, or <server><20> translates.  We check
                         * to see if <server> is an IP address first.
                         */

                        /*
                         * See if we have an existing server.  Do not
                         * establish a connection if one does not already
                         * exist.
                         */
                        srv = smbc_server(context, False, server, "IPC$",
                                          workgroup, user, password);

                        /*
                         * If no existing server and not an IP addr, look for
                         * LMB or DMB
                         */
			if (!srv &&
                            !is_ipaddress(server) &&
			    (resolve_name(server, &rem_ip, 0x1d) ||   /* LMB */
                             resolve_name(server, &rem_ip, 0x1b) )) { /* DMB */

				fstring buserver;

				dir->dir_type = SMBC_SERVER;

				/*
				 * Get the backup list ...
				 */
				if (!name_status_find(server, 0, 0,
                                                      rem_ip, buserver)) {

                                        DEBUG(0, ("Could not get name of "
                                                  "local/domain master browser "
                                                  "for server %s\n", server));
					if (dir) {
						SAFE_FREE(dir->fname);
						SAFE_FREE(dir);
					}
					errno = EPERM;
					return NULL;

				}

				/*
                                 * Get a connection to IPC$ on the server if
                                 * we do not already have one
                                 */
				srv = smbc_server(context, True,
                                                  buserver, "IPC$",
                                                  workgroup, user, password);
				if (!srv) {
				        DEBUG(0, ("got no contact to IPC$\n"));
					if (dir) {
						SAFE_FREE(dir->fname);
						SAFE_FREE(dir);
					}
					return NULL;

				}

				dir->srv = srv;

				/* Now, list the servers ... */
				if (!cli_NetServerEnum(&srv->cli, server,
                                                       0x0000FFFE, list_fn,
						       (void *)dir)) {

					if (dir) {
						SAFE_FREE(dir->fname);
						SAFE_FREE(dir);
					}
					return NULL;
				}
			} else if (srv ||
                                   (resolve_name(server, &rem_ip, 0x20))) {
                                
                                /* If we hadn't found the server, get one now */
                                if (!srv) {
                                        srv = smbc_server(context, True,
                                                          server, "IPC$",
                                                          workgroup,
                                                          user, password);
                                }

                                if (!srv) {
                                        if (dir) {
                                                SAFE_FREE(dir->fname);
                                                SAFE_FREE(dir);
                                        }
                                        return NULL;

                                }

                                dir->dir_type = SMBC_FILE_SHARE;
                                dir->srv = srv;

                                /* List the shares ... */

                                if (net_share_enum_rpc(
                                            &srv->cli,
                                            list_fn,
                                            (void *) dir) < 0 &&
                                    cli_RNetShareEnum(
                                            &srv->cli,
                                            list_fn, 
                                            (void *)dir) < 0) {
                                                
                                        errno = cli_errno(&srv->cli);
                                        if (dir) {
                                                SAFE_FREE(dir->fname);
                                                SAFE_FREE(dir);
                                        }
                                        return NULL;

                                }
                        } else {
                                /* Neither the workgroup nor server exists */
                                errno = ECONNREFUSED;   
                                if (dir) {
                                        SAFE_FREE(dir->fname);
                                        SAFE_FREE(dir);
                                }
                                return NULL;
			}

		}
		else {
                        /*
                         * The server and share are specified ... work from
                         * there ...
                         */
			pstring targetpath;
			struct cli_state *targetcli;

			/* We connect to the server and list the directory */
			dir->dir_type = SMBC_FILE_SHARE;

			srv = smbc_server(context, True, server, share,
                                          workgroup, user, password);

			if (!srv) {

				if (dir) {
					SAFE_FREE(dir->fname);
					SAFE_FREE(dir);
				}
				return NULL;

			}

			dir->srv = srv;

			/* Now, list the files ... */

                        p = path + strlen(path);
			pstrcat(path, "\\*");

			if (!cli_resolve_path("", &srv->cli, path,
                                              &targetcli, targetpath))
			{
				d_printf("Could not resolve %s\n", path);
				if (dir) {
					SAFE_FREE(dir->fname);
					SAFE_FREE(dir);
				}
				return NULL;
			}
			
			if (cli_list(targetcli, targetpath,
                                     aDIR | aSYSTEM | aHIDDEN,
                                     dir_list_fn, (void *)dir) < 0) {

				if (dir) {
					SAFE_FREE(dir->fname);
					SAFE_FREE(dir);
				}
				errno = smbc_errno(context, targetcli);

                                if (errno == EINVAL) {
                                    /*
                                     * See if they asked to opendir something
                                     * other than a directory.  If so, the
                                     * converted error value we got would have
                                     * been EINVAL rather than ENOTDIR.
                                     */
                                    *p = '\0'; /* restore original path */

                                    if (smbc_getatr(context, srv, path,
                                                    &mode, NULL,
                                                    NULL, NULL, NULL,
                                                    NULL) &&
                                        ! IS_DOS_DIR(mode)) {

                                        /* It is.  Correct the error value */
                                        errno = ENOTDIR;
                                    }
                                }

				return NULL;

			}
		}

	}

	DLIST_ADD(context->internal->_files, dir);
	return dir;

}

/*
 * Routine to close a directory
 */

static int
smbc_closedir_ctx(SMBCCTX *context,
                  SMBCFILE *dir)
{

        if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;
		return -1;

	}

	if (!dir || !DLIST_CONTAINS(context->internal->_files, dir)) {

		errno = EBADF;
		return -1;
    
	}

	smbc_remove_dir(dir); /* Clean it up */

	DLIST_REMOVE(context->internal->_files, dir);

	if (dir) {

		SAFE_FREE(dir->fname);
		SAFE_FREE(dir);    /* Free the space too */
	}

	return 0;

}

static void
smbc_readdir_internal(SMBCCTX * context,
                      struct smbc_dirent *dest,
                      struct smbc_dirent *src,
                      int max_namebuf_len)
{
        if (context->options.urlencode_readdir_entries) {

                /* url-encode the name.  get back remaining buffer space */
                max_namebuf_len =
                        smbc_urlencode(dest->name, src->name, max_namebuf_len);

                /* We now know the name length */
                dest->namelen = strlen(dest->name);

                /* Save the pointer to the beginning of the comment */
                dest->comment = dest->name + dest->namelen + 1;

                /* Copy the comment */
                strncpy(dest->comment, src->comment, max_namebuf_len - 1);
                dest->comment[max_namebuf_len - 1] = '\0';

                /* Save other fields */
                dest->smbc_type = src->smbc_type;
                dest->commentlen = strlen(dest->comment);
                dest->dirlen = ((dest->comment + dest->commentlen + 1) -
                                (char *) dest);
        } else {

                /* No encoding.  Just copy the entry as is. */
                memcpy(dest, src, src->dirlen);
                dest->comment = (char *)(&dest->name + src->namelen + 1);
        }
        
}

/*
 * Routine to get a directory entry
 */

struct smbc_dirent *
smbc_readdir_ctx(SMBCCTX *context,
                 SMBCFILE *dir)
{
        int maxlen;
	struct smbc_dirent *dirp, *dirent;

	/* Check that all is ok first ... */

	if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;
                DEBUG(0, ("Invalid context in smbc_readdir_ctx()\n"));
		return NULL;

	}

	if (!dir || !DLIST_CONTAINS(context->internal->_files, dir)) {

		errno = EBADF;
                DEBUG(0, ("Invalid dir in smbc_readdir_ctx()\n"));
		return NULL;

	}

	if (dir->file != False) { /* FIXME, should be dir, perhaps */

		errno = ENOTDIR;
                DEBUG(0, ("Found file vs directory in smbc_readdir_ctx()\n"));
		return NULL;

	}

	if (!dir->dir_next) {
		return NULL;
        }

        dirent = dir->dir_next->dirent;
        if (!dirent) {

                errno = ENOENT;
                return NULL;

        }

        dirp = (struct smbc_dirent *)context->internal->_dirent;
        maxlen = (sizeof(context->internal->_dirent) -
                  sizeof(struct smbc_dirent));

        smbc_readdir_internal(context, dirp, dirent, maxlen);

        dir->dir_next = dir->dir_next->next;

        return dirp;
}

/*
 * Routine to get directory entries
 */

static int
smbc_getdents_ctx(SMBCCTX *context,
                  SMBCFILE *dir,
                  struct smbc_dirent *dirp,
                  int count)
{
	int rem = count;
        int reqd;
        int maxlen;
	char *ndir = (char *)dirp;
	struct smbc_dir_list *dirlist;

	/* Check that all is ok first ... */

	if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;
		return -1;

	}

	if (!dir || !DLIST_CONTAINS(context->internal->_files, dir)) {

		errno = EBADF;
		return -1;
    
	}

	if (dir->file != False) { /* FIXME, should be dir, perhaps */

		errno = ENOTDIR;
		return -1;

	}

	/* 
	 * Now, retrieve the number of entries that will fit in what was passed
	 * We have to figure out if the info is in the list, or we need to 
	 * send a request to the server to get the info.
	 */

	while ((dirlist = dir->dir_next)) {
		struct smbc_dirent *dirent;

		if (!dirlist->dirent) {

			errno = ENOENT;  /* Bad error */
			return -1;

		}

                /* Do urlencoding of next entry, if so selected */
                dirent = (struct smbc_dirent *)context->internal->_dirent;
                maxlen = (sizeof(context->internal->_dirent) -
                          sizeof(struct smbc_dirent));
                smbc_readdir_internal(context, dirent, dirlist->dirent, maxlen);

                reqd = dirent->dirlen;

		if (rem < reqd) {

			if (rem < count) { /* We managed to copy something */

				errno = 0;
				return count - rem;

			}
			else { /* Nothing copied ... */

				errno = EINVAL;  /* Not enough space ... */
				return -1;

			}

		}

		memcpy(ndir, dirent, reqd); /* Copy the data in ... */
    
		((struct smbc_dirent *)ndir)->comment = 
			(char *)(&((struct smbc_dirent *)ndir)->name +
                                 dirent->namelen +
                                 1);

		ndir += reqd;

		rem -= reqd;

		dir->dir_next = dirlist = dirlist -> next;
	}

	if (rem == count)
		return 0;
	else 
		return count - rem;

}

/*
 * Routine to create a directory ...
 */

static int
smbc_mkdir_ctx(SMBCCTX *context,
               const char *fname,
               mode_t mode)
{
	SMBCSRV *srv;
	fstring server;
        fstring share;
        fstring user;
        fstring password;
        fstring workgroup;
	pstring path, targetpath;
	struct cli_state *targetcli;

	if (!context || !context->internal || 
	    !context->internal->_initialized) {

		errno = EINVAL;
		return -1;

	}

	if (!fname) {

		errno = EINVAL;
		return -1;

	}
  
	DEBUG(4, ("smbc_mkdir(%s)\n", fname));

	if (smbc_parse_path(context, fname,
                            workgroup, sizeof(workgroup),
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            NULL, 0)) {
                errno = EINVAL;
                return -1;
        }

	if (user[0] == (char)0) fstrcpy(user, context->user);

	srv = smbc_server(context, True,
                          server, share, workgroup, user, password);

	if (!srv) {

		return -1;  /* errno set by smbc_server */

	}

	/*d_printf(">>>mkdir: resolving %s\n", path);*/
	if (!cli_resolve_path( "", &srv->cli, path, &targetcli, targetpath))
	{
		d_printf("Could not resolve %s\n", path);
		return -1;
	}
	/*d_printf(">>>mkdir: resolved path as %s\n", targetpath);*/

	if (!cli_mkdir(targetcli, targetpath)) {

		errno = smbc_errno(context, targetcli);
		return -1;

	} 

	return 0;

}

/*
 * Our list function simply checks to see if a directory is not empty
 */

static int smbc_rmdir_dirempty = True;

static void
rmdir_list_fn(const char *mnt,
              file_info *finfo,
              const char *mask,
              void *state)
{
	if (strncmp(finfo->name, ".", 1) != 0 &&
            strncmp(finfo->name, "..", 2) != 0) {
                
		smbc_rmdir_dirempty = False;
        }
}

/*
 * Routine to remove a directory
 */

static int
smbc_rmdir_ctx(SMBCCTX *context,
               const char *fname)
{
	SMBCSRV *srv;
	fstring server;
        fstring share;
        fstring user;
        fstring password;
        fstring workgroup;
	pstring path;
        pstring targetpath;
	struct cli_state *targetcli;

	if (!context || !context->internal || 
	    !context->internal->_initialized) {

		errno = EINVAL;
		return -1;

	}

	if (!fname) {

		errno = EINVAL;
		return -1;

	}
  
	DEBUG(4, ("smbc_rmdir(%s)\n", fname));

	if (smbc_parse_path(context, fname,
                            workgroup, sizeof(workgroup),
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            NULL, 0))
        {
                errno = EINVAL;
                return -1;
        }

	if (user[0] == (char)0) fstrcpy(user, context->user);

	srv = smbc_server(context, True,
                          server, share, workgroup, user, password);

	if (!srv) {

		return -1;  /* errno set by smbc_server */

	}

	/*d_printf(">>>rmdir: resolving %s\n", path);*/
	if (!cli_resolve_path( "", &srv->cli, path, &targetcli, targetpath))
	{
		d_printf("Could not resolve %s\n", path);
		return -1;
	}
	/*d_printf(">>>rmdir: resolved path as %s\n", targetpath);*/


	if (!cli_rmdir(targetcli, targetpath)) {

		errno = smbc_errno(context, targetcli);

		if (errno == EACCES) {  /* Check if the dir empty or not */

                        /* Local storage to avoid buffer overflows */
			pstring lpath; 

			smbc_rmdir_dirempty = True;  /* Make this so ... */

			pstrcpy(lpath, targetpath);
			pstrcat(lpath, "\\*");

			if (cli_list(targetcli, lpath,
                                     aDIR | aSYSTEM | aHIDDEN,
                                     rmdir_list_fn, NULL) < 0) {

				/* Fix errno to ignore latest error ... */
				DEBUG(5, ("smbc_rmdir: "
                                          "cli_list returned an error: %d\n", 
					  smbc_errno(context, targetcli)));
				errno = EACCES;

			}

			if (smbc_rmdir_dirempty)
				errno = EACCES;
			else
				errno = ENOTEMPTY;

		}

		return -1;

	} 

	return 0;

}

/*
 * Routine to return the current directory position
 */

static SMB_OFF_T
smbc_telldir_ctx(SMBCCTX *context,
                 SMBCFILE *dir)
{
	off_t ret_val; /* Squash warnings about cast */

	if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;
		return -1;

	}

	if (!dir || !DLIST_CONTAINS(context->internal->_files, dir)) {

		errno = EBADF;
		return -1;

	}

	if (dir->file != False) { /* FIXME, should be dir, perhaps */

		errno = ENOTDIR;
		return -1;

	}

	/*
	 * We return the pointer here as the offset
	 */
	ret_val = (off_t)(long)dir->dir_next;
	return ret_val;

}

/*
 * A routine to run down the list and see if the entry is OK
 */

struct smbc_dir_list *
smbc_check_dir_ent(struct smbc_dir_list *list, 
                   struct smbc_dirent *dirent)
{

	/* Run down the list looking for what we want */

	if (dirent) {

		struct smbc_dir_list *tmp = list;

		while (tmp) {

			if (tmp->dirent == dirent)
				return tmp;

			tmp = tmp->next;

		}

	}

	return NULL;  /* Not found, or an error */

}


/*
 * Routine to seek on a directory
 */

static int
smbc_lseekdir_ctx(SMBCCTX *context,
                  SMBCFILE *dir,
                  SMB_OFF_T offset)
{
	long long l_offset = offset;  /* Handle problems of size */
	struct smbc_dirent *dirent = (struct smbc_dirent *)l_offset;
	struct smbc_dir_list *list_ent = (struct smbc_dir_list *)NULL;

	if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;
		return -1;

	}

	if (dir->file != False) { /* FIXME, should be dir, perhaps */

		errno = ENOTDIR;
		return -1;

	}

	/* Now, check what we were passed and see if it is OK ... */

	if (dirent == NULL) {  /* Seek to the begining of the list */

		dir->dir_next = dir->dir_list;
		return 0;

	}

	/* Now, run down the list and make sure that the entry is OK       */
	/* This may need to be changed if we change the format of the list */

	if ((list_ent = smbc_check_dir_ent(dir->dir_list, dirent)) == NULL) {

		errno = EINVAL;   /* Bad entry */
		return -1;

	}

	dir->dir_next = list_ent;

	return 0; 

}

/*
 * Routine to fstat a dir
 */

static int
smbc_fstatdir_ctx(SMBCCTX *context,
                  SMBCFILE *dir,
                  SMB_STRUCT_STAT *st)
{

	if (!context || !context->internal || 
	    !context->internal->_initialized) {

		errno = EINVAL;
		return -1;

	}

	/* No code yet ... */

	return 0;

}

static int
smbc_chmod_ctx(SMBCCTX *context,
               const char *fname,
               mode_t newmode)
{
        SMBCSRV *srv;
	fstring server;
        fstring share;
        fstring user;
        fstring password;
        fstring workgroup;
	pstring path;
	uint16 mode;

	if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;  /* Best I can think of ... */
		return -1;
    
	}

	if (!fname) {

		errno = EINVAL;
		return -1;

	}
  
	DEBUG(4, ("smbc_chmod(%s, 0%3o)\n", fname, newmode));

	if (smbc_parse_path(context, fname,
                            workgroup, sizeof(workgroup),
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            NULL, 0)) {
                errno = EINVAL;
                return -1;
        }

	if (user[0] == (char)0) fstrcpy(user, context->user);

	srv = smbc_server(context, True,
                          server, share, workgroup, user, password);

	if (!srv) {
		return -1;  /* errno set by smbc_server */
	}

	mode = 0;

	if (!(newmode & (S_IWUSR | S_IWGRP | S_IWOTH))) mode |= aRONLY;
	if ((newmode & S_IXUSR) && lp_map_archive(-1)) mode |= aARCH;
	if ((newmode & S_IXGRP) && lp_map_system(-1)) mode |= aSYSTEM;
	if ((newmode & S_IXOTH) && lp_map_hidden(-1)) mode |= aHIDDEN;

	if (!cli_setatr(&srv->cli, path, mode, 0)) {
		errno = smbc_errno(context, &srv->cli);
		return -1;
	}
	
        return 0;
}

static int
smbc_utimes_ctx(SMBCCTX *context,
                const char *fname,
                struct timeval *tbuf)
{
        SMBCSRV *srv;
	fstring server;
        fstring share;
        fstring user;
        fstring password;
        fstring workgroup;
	pstring path;
        time_t a_time;
        time_t m_time;

	if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;  /* Best I can think of ... */
		return -1;
    
	}

	if (!fname) {

		errno = EINVAL;
		return -1;

	}
  
        if (tbuf == NULL) {
                a_time = m_time = time(NULL);
        } else {
                a_time = tbuf[0].tv_sec;
                m_time = tbuf[1].tv_sec;
        }

        if (DEBUGLVL(4)) 
        {
                char *p;
                char atimebuf[32];
                char mtimebuf[32];

                strncpy(atimebuf, ctime(&a_time), sizeof(atimebuf) - 1);
                atimebuf[sizeof(atimebuf) - 1] = '\0';
                if ((p = strchr(atimebuf, '\n')) != NULL) {
                        *p = '\0';
                }

                strncpy(mtimebuf, ctime(&m_time), sizeof(mtimebuf) - 1);
                mtimebuf[sizeof(mtimebuf) - 1] = '\0';
                if ((p = strchr(mtimebuf, '\n')) != NULL) {
                        *p = '\0';
                }

                dbgtext("smbc_utimes(%s, atime = %s mtime = %s)\n",
                        fname, atimebuf, mtimebuf);
        }

	if (smbc_parse_path(context, fname,
                            workgroup, sizeof(workgroup),
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            NULL, 0)) {
                errno = EINVAL;
                return -1;
        }

	if (user[0] == (char)0) fstrcpy(user, context->user);

	srv = smbc_server(context, True,
                          server, share, workgroup, user, password);

	if (!srv) {
		return -1;      /* errno set by smbc_server */
	}

        if (!smbc_setatr(context, srv, path, 0, a_time, m_time, 0)) {
                return -1;      /* errno set by smbc_setatr */
        }

        return 0;
}


/* The MSDN is contradictory over the ordering of ACE entries in an ACL.
   However NT4 gives a "The information may have been modified by a
   computer running Windows NT 5.0" if denied ACEs do not appear before
   allowed ACEs. */

static int
ace_compare(SEC_ACE *ace1,
            SEC_ACE *ace2)
{
	if (sec_ace_equal(ace1, ace2)) 
		return 0;

	if (ace1->type != ace2->type) 
		return ace2->type - ace1->type;

	if (sid_compare(&ace1->trustee, &ace2->trustee)) 
		return sid_compare(&ace1->trustee, &ace2->trustee);

	if (ace1->flags != ace2->flags) 
		return ace1->flags - ace2->flags;

	if (ace1->info.mask != ace2->info.mask) 
		return ace1->info.mask - ace2->info.mask;

	if (ace1->size != ace2->size) 
		return ace1->size - ace2->size;

	return memcmp(ace1, ace2, sizeof(SEC_ACE));
}


static void
sort_acl(SEC_ACL *the_acl)
{
	uint32 i;
	if (!the_acl) return;

	qsort(the_acl->ace, the_acl->num_aces, sizeof(the_acl->ace[0]),
              QSORT_CAST ace_compare);

	for (i=1;i<the_acl->num_aces;) {
		if (sec_ace_equal(&the_acl->ace[i-1], &the_acl->ace[i])) {
			int j;
			for (j=i; j<the_acl->num_aces-1; j++) {
				the_acl->ace[j] = the_acl->ace[j+1];
			}
			the_acl->num_aces--;
		} else {
			i++;
		}
	}
}

/* convert a SID to a string, either numeric or username/group */
static void
convert_sid_to_string(struct cli_state *ipc_cli,
                      POLICY_HND *pol,
                      fstring str,
                      BOOL numeric,
                      DOM_SID *sid)
{
	char **domains = NULL;
	char **names = NULL;
	uint32 *types = NULL;
	struct rpc_pipe_client *pipe_hnd = find_lsa_pipe_hnd(ipc_cli);
	sid_to_string(str, sid);

	if (numeric) {
		return;     /* no lookup desired */
	}
       
	if (!pipe_hnd) {
		return;
	}
 
	/* Ask LSA to convert the sid to a name */

	if (!NT_STATUS_IS_OK(rpccli_lsa_lookup_sids(pipe_hnd, ipc_cli->mem_ctx,  
						 pol, 1, sid, &domains, 
						 &names, &types)) ||
	    !domains || !domains[0] || !names || !names[0]) {
		return;
	}

	/* Converted OK */

	slprintf(str, sizeof(fstring) - 1, "%s%s%s",
		 domains[0], lp_winbind_separator(),
		 names[0]);
}

/* convert a string to a SID, either numeric or username/group */
static BOOL
convert_string_to_sid(struct cli_state *ipc_cli,
                      POLICY_HND *pol,
                      BOOL numeric,
                      DOM_SID *sid,
                      const char *str)
{
	uint32 *types = NULL;
	DOM_SID *sids = NULL;
	BOOL result = True;
	struct rpc_pipe_client *pipe_hnd = find_lsa_pipe_hnd(ipc_cli);

	if (!pipe_hnd) {
		return False;
	}

        if (numeric) {
                if (strncmp(str, "S-", 2) == 0) {
                        return string_to_sid(sid, str);
                }

                result = False;
                goto done;
        }

	if (!NT_STATUS_IS_OK(rpccli_lsa_lookup_names(pipe_hnd, ipc_cli->mem_ctx, 
						  pol, 1, &str, NULL, &sids, 
						  &types))) {
		result = False;
		goto done;
	}

	sid_copy(sid, &sids[0]);
 done:

	return result;
}


/* parse an ACE in the same format as print_ace() */
static BOOL
parse_ace(struct cli_state *ipc_cli,
          POLICY_HND *pol,
          SEC_ACE *ace,
          BOOL numeric,
          char *str)
{
	char *p;
	const char *cp;
	fstring tok;
	unsigned int atype;
        unsigned int aflags;
        unsigned int amask;
	DOM_SID sid;
	SEC_ACCESS mask;
	const struct perm_value *v;
        struct perm_value {
                const char *perm;
                uint32 mask;
        };

        /* These values discovered by inspection */
        static const struct perm_value special_values[] = {
                { "R", 0x00120089 },
                { "W", 0x00120116 },
                { "X", 0x001200a0 },
                { "D", 0x00010000 },
                { "P", 0x00040000 },
                { "O", 0x00080000 },
                { NULL, 0 },
        };

        static const struct perm_value standard_values[] = {
                { "READ",   0x001200a9 },
                { "CHANGE", 0x001301bf },
                { "FULL",   0x001f01ff },
                { NULL, 0 },
        };


	ZERO_STRUCTP(ace);
	p = strchr_m(str,':');
	if (!p) return False;
	*p = '\0';
	p++;
	/* Try to parse numeric form */

	if (sscanf(p, "%i/%i/%i", &atype, &aflags, &amask) == 3 &&
	    convert_string_to_sid(ipc_cli, pol, numeric, &sid, str)) {
		goto done;
	}

	/* Try to parse text form */

	if (!convert_string_to_sid(ipc_cli, pol, numeric, &sid, str)) {
		return False;
	}

	cp = p;
	if (!next_token(&cp, tok, "/", sizeof(fstring))) {
		return False;
	}

	if (StrnCaseCmp(tok, "ALLOWED", strlen("ALLOWED")) == 0) {
		atype = SEC_ACE_TYPE_ACCESS_ALLOWED;
	} else if (StrnCaseCmp(tok, "DENIED", strlen("DENIED")) == 0) {
		atype = SEC_ACE_TYPE_ACCESS_DENIED;
	} else {
		return False;
	}

	/* Only numeric form accepted for flags at present */

	if (!(next_token(&cp, tok, "/", sizeof(fstring)) &&
	      sscanf(tok, "%i", &aflags))) {
		return False;
	}

	if (!next_token(&cp, tok, "/", sizeof(fstring))) {
		return False;
	}

	if (strncmp(tok, "0x", 2) == 0) {
		if (sscanf(tok, "%i", &amask) != 1) {
			return False;
		}
		goto done;
	}

	for (v = standard_values; v->perm; v++) {
		if (strcmp(tok, v->perm) == 0) {
			amask = v->mask;
			goto done;
		}
	}

	p = tok;

	while(*p) {
		BOOL found = False;

		for (v = special_values; v->perm; v++) {
			if (v->perm[0] == *p) {
				amask |= v->mask;
				found = True;
			}
		}

		if (!found) return False;
		p++;
	}

	if (*p) {
		return False;
	}

 done:
	mask.mask = amask;
	init_sec_ace(ace, &sid, atype, mask, aflags);
	return True;
}

/* add an ACE to a list of ACEs in a SEC_ACL */
static BOOL
add_ace(SEC_ACL **the_acl,
        SEC_ACE *ace,
        TALLOC_CTX *ctx)
{
	SEC_ACL *newacl;
	SEC_ACE *aces;

	if (! *the_acl) {
		(*the_acl) = make_sec_acl(ctx, 3, 1, ace);
		return True;
	}

	if ((aces = SMB_CALLOC_ARRAY(SEC_ACE, 1+(*the_acl)->num_aces)) == NULL) {
		return False;
	}
	memcpy(aces, (*the_acl)->ace, (*the_acl)->num_aces * sizeof(SEC_ACE));
	memcpy(aces+(*the_acl)->num_aces, ace, sizeof(SEC_ACE));
	newacl = make_sec_acl(ctx, (*the_acl)->revision,
                              1+(*the_acl)->num_aces, aces);
	SAFE_FREE(aces);
	(*the_acl) = newacl;
	return True;
}


/* parse a ascii version of a security descriptor */
static SEC_DESC *
sec_desc_parse(TALLOC_CTX *ctx,
               struct cli_state *ipc_cli,
               POLICY_HND *pol,
               BOOL numeric,
               char *str)
{
	const char *p = str;
	fstring tok;
	SEC_DESC *ret = NULL;
	size_t sd_size;
	DOM_SID *grp_sid=NULL;
        DOM_SID *owner_sid=NULL;
	SEC_ACL *dacl=NULL;
	int revision=1;

	while (next_token(&p, tok, "\t,\r\n", sizeof(tok))) {

		if (StrnCaseCmp(tok,"REVISION:", 9) == 0) {
			revision = strtol(tok+9, NULL, 16);
			continue;
		}

		if (StrnCaseCmp(tok,"OWNER:", 6) == 0) {
			if (owner_sid) {
				DEBUG(5, ("OWNER specified more than once!\n"));
				goto done;
			}
			owner_sid = SMB_CALLOC_ARRAY(DOM_SID, 1);
			if (!owner_sid ||
			    !convert_string_to_sid(ipc_cli, pol,
                                                   numeric,
                                                   owner_sid, tok+6)) {
				DEBUG(5, ("Failed to parse owner sid\n"));
				goto done;
			}
			continue;
		}

		if (StrnCaseCmp(tok,"OWNER+:", 7) == 0) {
			if (owner_sid) {
				DEBUG(5, ("OWNER specified more than once!\n"));
				goto done;
			}
			owner_sid = SMB_CALLOC_ARRAY(DOM_SID, 1);
			if (!owner_sid ||
			    !convert_string_to_sid(ipc_cli, pol,
                                                   False,
                                                   owner_sid, tok+7)) {
				DEBUG(5, ("Failed to parse owner sid\n"));
				goto done;
			}
			continue;
		}

		if (StrnCaseCmp(tok,"GROUP:", 6) == 0) {
			if (grp_sid) {
				DEBUG(5, ("GROUP specified more than once!\n"));
				goto done;
			}
			grp_sid = SMB_CALLOC_ARRAY(DOM_SID, 1);
			if (!grp_sid ||
			    !convert_string_to_sid(ipc_cli, pol,
                                                   numeric,
                                                   grp_sid, tok+6)) {
				DEBUG(5, ("Failed to parse group sid\n"));
				goto done;
			}
			continue;
		}

		if (StrnCaseCmp(tok,"GROUP+:", 7) == 0) {
			if (grp_sid) {
				DEBUG(5, ("GROUP specified more than once!\n"));
				goto done;
			}
			grp_sid = SMB_CALLOC_ARRAY(DOM_SID, 1);
			if (!grp_sid ||
			    !convert_string_to_sid(ipc_cli, pol,
                                                   False,
                                                   grp_sid, tok+6)) {
				DEBUG(5, ("Failed to parse group sid\n"));
				goto done;
			}
			continue;
		}

		if (StrnCaseCmp(tok,"ACL:", 4) == 0) {
			SEC_ACE ace;
			if (!parse_ace(ipc_cli, pol, &ace, numeric, tok+4)) {
				DEBUG(5, ("Failed to parse ACL %s\n", tok));
				goto done;
			}
			if(!add_ace(&dacl, &ace, ctx)) {
				DEBUG(5, ("Failed to add ACL %s\n", tok));
				goto done;
			}
			continue;
		}

		if (StrnCaseCmp(tok,"ACL+:", 5) == 0) {
			SEC_ACE ace;
			if (!parse_ace(ipc_cli, pol, &ace, False, tok+5)) {
				DEBUG(5, ("Failed to parse ACL %s\n", tok));
				goto done;
			}
			if(!add_ace(&dacl, &ace, ctx)) {
				DEBUG(5, ("Failed to add ACL %s\n", tok));
				goto done;
			}
			continue;
		}

		DEBUG(5, ("Failed to parse security descriptor\n"));
		goto done;
	}

	ret = make_sec_desc(ctx, revision, SEC_DESC_SELF_RELATIVE, 
			    owner_sid, grp_sid, NULL, dacl, &sd_size);

  done:
	SAFE_FREE(grp_sid);
	SAFE_FREE(owner_sid);

	return ret;
}


/* Obtain the current dos attributes */
static DOS_ATTR_DESC *
dos_attr_query(SMBCCTX *context,
               TALLOC_CTX *ctx,
               const char *filename,
               SMBCSRV *srv)
{
        time_t m_time = 0, a_time = 0, c_time = 0;
        SMB_OFF_T size = 0;
        uint16 mode = 0;
	SMB_INO_T inode = 0;
        DOS_ATTR_DESC *ret;
    
        ret = TALLOC_P(ctx, DOS_ATTR_DESC);
        if (!ret) {
                errno = ENOMEM;
                return NULL;
        }

        /* Obtain the DOS attributes */
        if (!smbc_getatr(context, srv, CONST_DISCARD(char *, filename),
                         &mode, &size, 
                         &c_time, &a_time, &m_time, &inode)) {
        
                errno = smbc_errno(context, &srv->cli);
                DEBUG(5, ("dos_attr_query Failed to query old attributes\n"));
                return NULL;
        
        }
                
        ret->mode = mode;
        ret->size = size;
        ret->a_time = a_time;
        ret->c_time = c_time;
        ret->m_time = m_time;
        ret->inode = inode;

        return ret;
}


/* parse a ascii version of a security descriptor */
static void
dos_attr_parse(SMBCCTX *context,
               DOS_ATTR_DESC *dad,
               SMBCSRV *srv,
               char *str)
{
	const char *p = str;
	fstring tok;

	while (next_token(&p, tok, "\t,\r\n", sizeof(tok))) {

		if (StrnCaseCmp(tok, "MODE:", 5) == 0) {
			dad->mode = strtol(tok+5, NULL, 16);
			continue;
		}

		if (StrnCaseCmp(tok, "SIZE:", 5) == 0) {
                        dad->size = (SMB_OFF_T)atof(tok+5);
			continue;
		}

		if (StrnCaseCmp(tok, "A_TIME:", 7) == 0) {
                        dad->a_time = (time_t)strtol(tok+7, NULL, 10);
			continue;
		}

		if (StrnCaseCmp(tok, "C_TIME:", 7) == 0) {
                        dad->c_time = (time_t)strtol(tok+7, NULL, 10);
			continue;
		}

		if (StrnCaseCmp(tok, "M_TIME:", 7) == 0) {
                        dad->m_time = (time_t)strtol(tok+7, NULL, 10);
			continue;
		}

		if (StrnCaseCmp(tok, "INODE:", 6) == 0) {
                        dad->inode = (SMB_INO_T)atof(tok+6);
			continue;
		}
	}
}

/***************************************************** 
 Retrieve the acls for a file.
*******************************************************/

static int
cacl_get(SMBCCTX *context,
         TALLOC_CTX *ctx,
         SMBCSRV *srv,
         struct cli_state *ipc_cli,
         POLICY_HND *pol,
         char *filename,
         char *attr_name,
         char *buf,
         int bufsize)
{
	uint32 i;
        int n = 0;
        int n_used;
        BOOL all;
        BOOL all_nt;
        BOOL all_nt_acls;
        BOOL all_dos;
        BOOL some_nt;
        BOOL some_dos;
        BOOL exclude_nt_revision = False;
        BOOL exclude_nt_owner = False;
        BOOL exclude_nt_group = False;
        BOOL exclude_nt_acl = False;
        BOOL exclude_dos_mode = False;
        BOOL exclude_dos_size = False;
        BOOL exclude_dos_ctime = False;
        BOOL exclude_dos_atime = False;
        BOOL exclude_dos_mtime = False;
        BOOL exclude_dos_inode = False;
        BOOL numeric = True;
        BOOL determine_size = (bufsize == 0);
	int fnum = -1;
	SEC_DESC *sd;
	fstring sidstr;
        fstring name_sandbox;
        char *name;
        char *pExclude;
        char *p;
	time_t m_time = 0, a_time = 0, c_time = 0;
	SMB_OFF_T size = 0;
	uint16 mode = 0;
	SMB_INO_T ino = 0;
        struct cli_state *cli = &srv->cli;

        /* Copy name so we can strip off exclusions (if any are specified) */
        strncpy(name_sandbox, attr_name, sizeof(name_sandbox) - 1);

        /* Ensure name is null terminated */
        name_sandbox[sizeof(name_sandbox) - 1] = '\0';

        /* Play in the sandbox */
        name = name_sandbox;

        /* If there are any exclusions, point to them and mask them from name */
        if ((pExclude = strchr(name, '!')) != NULL)
        {
                *pExclude++ = '\0';
        }

        all = (StrnCaseCmp(name, "system.*", 8) == 0);
        all_nt = (StrnCaseCmp(name, "system.nt_sec_desc.*", 20) == 0);
        all_nt_acls = (StrnCaseCmp(name, "system.nt_sec_desc.acl.*", 24) == 0);
        all_dos = (StrnCaseCmp(name, "system.dos_attr.*", 17) == 0);
        some_nt = (StrnCaseCmp(name, "system.nt_sec_desc.", 19) == 0);
        some_dos = (StrnCaseCmp(name, "system.dos_attr.", 16) == 0);
        numeric = (* (name + strlen(name) - 1) != '+');

        /* Look for exclusions from "all" requests */
        if (all || all_nt || all_dos) {

                /* Exclusions are delimited by '!' */
                for (;
                     pExclude != NULL;
                     pExclude = (p == NULL ? NULL : p + 1)) {

                /* Find end of this exclusion name */
                if ((p = strchr(pExclude, '!')) != NULL)
                {
                    *p = '\0';
                }

                /* Which exclusion name is this? */
                if (StrCaseCmp(pExclude, "nt_sec_desc.revision") == 0) {
                    exclude_nt_revision = True;
                }
                else if (StrCaseCmp(pExclude, "nt_sec_desc.owner") == 0) {
                    exclude_nt_owner = True;
                }
                else if (StrCaseCmp(pExclude, "nt_sec_desc.group") == 0) {
                    exclude_nt_group = True;
                }
                else if (StrCaseCmp(pExclude, "nt_sec_desc.acl") == 0) {
                    exclude_nt_acl = True;
                }
                else if (StrCaseCmp(pExclude, "dos_attr.mode") == 0) {
                    exclude_dos_mode = True;
                }
                else if (StrCaseCmp(pExclude, "dos_attr.size") == 0) {
                    exclude_dos_size = True;
                }
                else if (StrCaseCmp(pExclude, "dos_attr.c_time") == 0) {
                    exclude_dos_ctime = True;
                }
                else if (StrCaseCmp(pExclude, "dos_attr.a_time") == 0) {
                    exclude_dos_atime = True;
                }
                else if (StrCaseCmp(pExclude, "dos_attr.m_time") == 0) {
                    exclude_dos_mtime = True;
                }
                else if (StrCaseCmp(pExclude, "dos_attr.inode") == 0) {
                    exclude_dos_inode = True;
                }
                else {
                    DEBUG(5, ("cacl_get received unknown exclusion: %s\n",
                              pExclude));
                    errno = ENOATTR;
                    return -1;
                }
            }
        }

        n_used = 0;

        /*
         * If we are (possibly) talking to an NT or new system and some NT
         * attributes have been requested...
         */
        if (ipc_cli && (all || some_nt || all_nt_acls)) {
                /* Point to the portion after "system.nt_sec_desc." */
                name += 19;     /* if (all) this will be invalid but unused */

                /* ... then obtain any NT attributes which were requested */
                fnum = cli_nt_create(cli, filename, CREATE_ACCESS_READ);

                if (fnum == -1) {
                        DEBUG(5, ("cacl_get failed to open %s: %s\n",
                                  filename, cli_errstr(cli)));
                        errno = 0;
                        return -1;
                }

                sd = cli_query_secdesc(cli, fnum, ctx);

                if (!sd) {
                        DEBUG(5,
                              ("cacl_get Failed to query old descriptor\n"));
                        errno = 0;
                        return -1;
                }

                cli_close(cli, fnum);

                if (! exclude_nt_revision) {
                        if (all || all_nt) {
                                if (determine_size) {
                                        p = talloc_asprintf(ctx,
                                                            "REVISION:%d",
                                                            sd->revision);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize,
                                                     "REVISION:%d",
                                                     sd->revision);
                                }
                        } else if (StrCaseCmp(name, "revision") == 0) {
                                if (determine_size) {
                                        p = talloc_asprintf(ctx, "%d",
                                                            sd->revision);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize, "%d",
                                                     sd->revision);
                                }
                        }
        
                        if (!determine_size && n > bufsize) {
                                errno = ERANGE;
                                return -1;
                        }
                        buf += n;
                        n_used += n;
                        bufsize -= n;
                }

                if (! exclude_nt_owner) {
                        /* Get owner and group sid */
                        if (sd->owner_sid) {
                                convert_sid_to_string(ipc_cli, pol,
                                                      sidstr,
                                                      numeric,
                                                      sd->owner_sid);
                        } else {
                                fstrcpy(sidstr, "");
                        }

                        if (all || all_nt) {
                                if (determine_size) {
                                        p = talloc_asprintf(ctx, ",OWNER:%s",
                                                            sidstr);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize,
                                                     ",OWNER:%s", sidstr);
                                }
                        } else if (StrnCaseCmp(name, "owner", 5) == 0) {
                                if (determine_size) {
                                        p = talloc_asprintf(ctx, "%s", sidstr);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize, "%s",
                                                     sidstr);
                                }
                        }

                        if (!determine_size && n > bufsize) {
                                errno = ERANGE;
                                return -1;
                        }
                        buf += n;
                        n_used += n;
                        bufsize -= n;
                }

                if (! exclude_nt_group) {
                        if (sd->grp_sid) {
                                convert_sid_to_string(ipc_cli, pol,
                                                      sidstr, numeric,
                                                      sd->grp_sid);
                        } else {
                                fstrcpy(sidstr, "");
                        }

                        if (all || all_nt) {
                                if (determine_size) {
                                        p = talloc_asprintf(ctx, ",GROUP:%s",
                                                            sidstr);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize,
                                                     ",GROUP:%s", sidstr);
                                }
                        } else if (StrnCaseCmp(name, "group", 5) == 0) {
                                if (determine_size) {
                                        p = talloc_asprintf(ctx, "%s", sidstr);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize,
                                                     "%s", sidstr);
                                }
                        }

                        if (!determine_size && n > bufsize) {
                                errno = ERANGE;
                                return -1;
                        }
                        buf += n;
                        n_used += n;
                        bufsize -= n;
                }

                if (! exclude_nt_acl) {
                        /* Add aces to value buffer  */
                        for (i = 0; sd->dacl && i < sd->dacl->num_aces; i++) {

                                SEC_ACE *ace = &sd->dacl->ace[i];
                                convert_sid_to_string(ipc_cli, pol,
                                                      sidstr, numeric,
                                                      &ace->trustee);

                                if (all || all_nt) {
                                        if (determine_size) {
                                                p = talloc_asprintf(
                                                        ctx, 
                                                        ",ACL:"
                                                        "%s:%d/%d/0x%08x", 
                                                        sidstr,
                                                        ace->type,
                                                        ace->flags,
                                                        ace->info.mask);
                                                if (!p) {
                                                        errno = ENOMEM;
                                                        return -1;
                                                }
                                                n = strlen(p);
                                        } else {
                                                n = snprintf(
                                                        buf, bufsize,
                                                        ",ACL:%s:%d/%d/0x%08x", 
                                                        sidstr,
                                                        ace->type,
                                                        ace->flags,
                                                        ace->info.mask);
                                        }
                                } else if ((StrnCaseCmp(name, "acl", 3) == 0 &&
                                            StrCaseCmp(name+3, sidstr) == 0) ||
                                           (StrnCaseCmp(name, "acl+", 4) == 0 &&
                                            StrCaseCmp(name+4, sidstr) == 0)) {
                                        if (determine_size) {
                                                p = talloc_asprintf(
                                                        ctx, 
                                                        "%d/%d/0x%08x", 
                                                        ace->type,
                                                        ace->flags,
                                                        ace->info.mask);
                                                if (!p) {
                                                        errno = ENOMEM;
                                                        return -1;
                                                }
                                                n = strlen(p);
                                        } else {
                                                n = snprintf(buf, bufsize,
                                                             "%d/%d/0x%08x", 
                                                             ace->type,
                                                             ace->flags,
                                                             ace->info.mask);
                                        }
                                } else if (all_nt_acls) {
                                        if (determine_size) {
                                                p = talloc_asprintf(
                                                        ctx, 
                                                        "%s%s:%d/%d/0x%08x",
                                                        i ? "," : "",
                                                        sidstr,
                                                        ace->type,
                                                        ace->flags,
                                                        ace->info.mask);
                                                if (!p) {
                                                        errno = ENOMEM;
                                                        return -1;
                                                }
                                                n = strlen(p);
                                        } else {
                                                n = snprintf(buf, bufsize,
                                                             "%s%s:%d/%d/0x%08x",
                                                             i ? "," : "",
                                                             sidstr,
                                                             ace->type,
                                                             ace->flags,
                                                             ace->info.mask);
                                        }
                                }
                                if (n > bufsize) {
                                        errno = ERANGE;
                                        return -1;
                                }
                                buf += n;
                                n_used += n;
                                bufsize -= n;
                        }
                }

                /* Restore name pointer to its original value */
                name -= 19;
        }

        if (all || some_dos) {
                /* Point to the portion after "system.dos_attr." */
                name += 16;     /* if (all) this will be invalid but unused */

                /* Obtain the DOS attributes */
                if (!smbc_getatr(context, srv, filename, &mode, &size, 
                                 &c_time, &a_time, &m_time, &ino)) {
                        
                        errno = smbc_errno(context, &srv->cli);
                        return -1;
                        
                }
                
                if (! exclude_dos_mode) {
                        if (all || all_dos) {
                                if (determine_size) {
                                        p = talloc_asprintf(ctx,
                                                            "%sMODE:0x%x",
                                                            (ipc_cli &&
                                                             (all || some_nt)
                                                             ? ","
                                                             : ""),
                                                            mode);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize,
                                                     "%sMODE:0x%x",
                                                     (ipc_cli &&
                                                      (all || some_nt)
                                                      ? ","
                                                      : ""),
                                                     mode);
                                }
                        } else if (StrCaseCmp(name, "mode") == 0) {
                                if (determine_size) {
                                        p = talloc_asprintf(ctx, "0x%x", mode);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize,
                                                     "0x%x", mode);
                                }
                        }
        
                        if (!determine_size && n > bufsize) {
                                errno = ERANGE;
                                return -1;
                        }
                        buf += n;
                        n_used += n;
                        bufsize -= n;
                }

                if (! exclude_dos_size) {
                        if (all || all_dos) {
                                if (determine_size) {
                                        p = talloc_asprintf(
                                                ctx,
                                                ",SIZE:%.0f",
                                                (double)size);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize,
                                                     ",SIZE:%.0f",
                                                     (double)size);
                                }
                        } else if (StrCaseCmp(name, "size") == 0) {
                                if (determine_size) {
                                        p = talloc_asprintf(
                                                ctx,
                                                "%.0f",
                                                (double)size);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize,
                                                     "%.0f",
                                                     (double)size);
                                }
                        }
        
                        if (!determine_size && n > bufsize) {
                                errno = ERANGE;
                                return -1;
                        }
                        buf += n;
                        n_used += n;
                        bufsize -= n;
                }

                if (! exclude_dos_ctime) {
                        if (all || all_dos) {
                                if (determine_size) {
                                        p = talloc_asprintf(ctx,
                                                            ",C_TIME:%lu",
                                                            c_time);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize,
                                                     ",C_TIME:%lu", c_time);
                                }
                        } else if (StrCaseCmp(name, "c_time") == 0) {
                                if (determine_size) {
                                        p = talloc_asprintf(ctx, "%lu", c_time);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize,
                                                     "%lu", c_time);
                                }
                        }
        
                        if (!determine_size && n > bufsize) {
                                errno = ERANGE;
                                return -1;
                        }
                        buf += n;
                        n_used += n;
                        bufsize -= n;
                }

                if (! exclude_dos_atime) {
                        if (all || all_dos) {
                                if (determine_size) {
                                        p = talloc_asprintf(ctx,
                                                            ",A_TIME:%lu",
                                                            a_time);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize,
                                                     ",A_TIME:%lu", a_time);
                                }
                        } else if (StrCaseCmp(name, "a_time") == 0) {
                                if (determine_size) {
                                        p = talloc_asprintf(ctx, "%lu", a_time);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize,
                                                     "%lu", a_time);
                                }
                        }
        
                        if (!determine_size && n > bufsize) {
                                errno = ERANGE;
                                return -1;
                        }
                        buf += n;
                        n_used += n;
                        bufsize -= n;
                }

                if (! exclude_dos_mtime) {
                        if (all || all_dos) {
                                if (determine_size) {
                                        p = talloc_asprintf(ctx,
                                                            ",M_TIME:%lu",
                                                            m_time);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize,
                                                     ",M_TIME:%lu", m_time);
                                }
                        } else if (StrCaseCmp(name, "m_time") == 0) {
                                if (determine_size) {
                                        p = talloc_asprintf(ctx, "%lu", m_time);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize,
                                                     "%lu", m_time);
                                }
                        }
        
                        if (!determine_size && n > bufsize) {
                                errno = ERANGE;
                                return -1;
                        }
                        buf += n;
                        n_used += n;
                        bufsize -= n;
                }

                if (! exclude_dos_inode) {
                        if (all || all_dos) {
                                if (determine_size) {
                                        p = talloc_asprintf(
                                                ctx,
                                                ",INODE:%.0f",
                                                (double)ino);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize,
                                                     ",INODE:%.0f",
                                                     (double) ino);
                                }
                        } else if (StrCaseCmp(name, "inode") == 0) {
                                if (determine_size) {
                                        p = talloc_asprintf(
                                                ctx,
                                                "%.0f",
                                                (double) ino);
                                        if (!p) {
                                                errno = ENOMEM;
                                                return -1;
                                        }
                                        n = strlen(p);
                                } else {
                                        n = snprintf(buf, bufsize,
                                                     "%.0f",
                                                     (double) ino);
                                }
                        }
        
                        if (!determine_size && n > bufsize) {
                                errno = ERANGE;
                                return -1;
                        }
                        buf += n;
                        n_used += n;
                        bufsize -= n;
                }

                /* Restore name pointer to its original value */
                name -= 16;
        }

        if (n_used == 0) {
                errno = ENOATTR;
                return -1;
        }

	return n_used;
}


/***************************************************** 
set the ACLs on a file given an ascii description
*******************************************************/
static int
cacl_set(TALLOC_CTX *ctx,
         struct cli_state *cli,
         struct cli_state *ipc_cli,
         POLICY_HND *pol,
         const char *filename,
         const char *the_acl,
         int mode,
         int flags)
{
	int fnum;
        int err = 0;
	SEC_DESC *sd = NULL, *old;
        SEC_ACL *dacl = NULL;
	DOM_SID *owner_sid = NULL; 
	DOM_SID *grp_sid = NULL;
	uint32 i, j;
	size_t sd_size;
	int ret = 0;
        char *p;
        BOOL numeric = True;

        /* the_acl will be null for REMOVE_ALL operations */
        if (the_acl) {
                numeric = ((p = strchr(the_acl, ':')) != NULL &&
                           p > the_acl &&
                           p[-1] != '+');

                /* if this is to set the entire ACL... */
                if (*the_acl == '*') {
                        /* ... then increment past the first colon */
                        the_acl = p + 1;
                }

                sd = sec_desc_parse(ctx, ipc_cli, pol, numeric,
                                    CONST_DISCARD(char *, the_acl));

                if (!sd) {
			errno = EINVAL;
			return -1;
                }
        }

	/* SMBC_XATTR_MODE_REMOVE_ALL is the only caller
	   that doesn't deref sd */

	if (!sd && (mode != SMBC_XATTR_MODE_REMOVE_ALL)) {
		errno = EINVAL;
		return -1;
	}

	/* The desired access below is the only one I could find that works
	   with NT4, W2KP and Samba */

	fnum = cli_nt_create(cli, filename, CREATE_ACCESS_READ);

	if (fnum == -1) {
                DEBUG(5, ("cacl_set failed to open %s: %s\n",
                          filename, cli_errstr(cli)));
                errno = 0;
		return -1;
	}

	old = cli_query_secdesc(cli, fnum, ctx);

	if (!old) {
                DEBUG(5, ("cacl_set Failed to query old descriptor\n"));
                errno = 0;
		return -1;
	}

	cli_close(cli, fnum);

	switch (mode) {
	case SMBC_XATTR_MODE_REMOVE_ALL:
                old->dacl->num_aces = 0;
                SAFE_FREE(old->dacl->ace);
                SAFE_FREE(old->dacl);
                old->off_dacl = 0;
                dacl = old->dacl;
                break;

        case SMBC_XATTR_MODE_REMOVE:
		for (i=0;sd->dacl && i<sd->dacl->num_aces;i++) {
			BOOL found = False;

			for (j=0;old->dacl && j<old->dacl->num_aces;j++) {
                                if (sec_ace_equal(&sd->dacl->ace[i],
                                                  &old->dacl->ace[j])) {
					uint32 k;
					for (k=j; k<old->dacl->num_aces-1;k++) {
						old->dacl->ace[k] =
                                                        old->dacl->ace[k+1];
					}
					old->dacl->num_aces--;
					if (old->dacl->num_aces == 0) {
						SAFE_FREE(old->dacl->ace);
						SAFE_FREE(old->dacl);
						old->off_dacl = 0;
					}
					found = True;
                                        dacl = old->dacl;
					break;
				}
			}

			if (!found) {
                                err = ENOATTR;
                                ret = -1;
                                goto failed;
			}
		}
		break;

	case SMBC_XATTR_MODE_ADD:
		for (i=0;sd->dacl && i<sd->dacl->num_aces;i++) {
			BOOL found = False;

			for (j=0;old->dacl && j<old->dacl->num_aces;j++) {
				if (sid_equal(&sd->dacl->ace[i].trustee,
					      &old->dacl->ace[j].trustee)) {
                                        if (!(flags & SMBC_XATTR_FLAG_CREATE)) {
                                                err = EEXIST;
                                                ret = -1;
                                                goto failed;
                                        }
                                        old->dacl->ace[j] = sd->dacl->ace[i];
                                        ret = -1;
					found = True;
				}
			}

			if (!found && (flags & SMBC_XATTR_FLAG_REPLACE)) {
                                err = ENOATTR;
                                ret = -1;
                                goto failed;
			}
                        
                        for (i=0;sd->dacl && i<sd->dacl->num_aces;i++) {
                                add_ace(&old->dacl, &sd->dacl->ace[i], ctx);
                        }
		}
                dacl = old->dacl;
		break;

	case SMBC_XATTR_MODE_SET:
 		old = sd;
                owner_sid = old->owner_sid;
                grp_sid = old->grp_sid;
                dacl = old->dacl;
		break;

        case SMBC_XATTR_MODE_CHOWN:
                owner_sid = sd->owner_sid;
                break;

        case SMBC_XATTR_MODE_CHGRP:
                grp_sid = sd->grp_sid;
                break;
	}

	/* Denied ACE entries must come before allowed ones */
	sort_acl(old->dacl);

	/* Create new security descriptor and set it */
	sd = make_sec_desc(ctx, old->revision, SEC_DESC_SELF_RELATIVE, 
			   owner_sid, grp_sid, NULL, dacl, &sd_size);

	fnum = cli_nt_create(cli, filename,
                             WRITE_DAC_ACCESS | WRITE_OWNER_ACCESS);

	if (fnum == -1) {
		DEBUG(5, ("cacl_set failed to open %s: %s\n",
                          filename, cli_errstr(cli)));
                errno = 0;
		return -1;
	}

	if (!cli_set_secdesc(cli, fnum, sd)) {
		DEBUG(5, ("ERROR: secdesc set failed: %s\n", cli_errstr(cli)));
		ret = -1;
	}

	/* Clean up */

 failed:
	cli_close(cli, fnum);

        if (err != 0) {
                errno = err;
        }
        
	return ret;
}


static int
smbc_setxattr_ctx(SMBCCTX *context,
                  const char *fname,
                  const char *name,
                  const void *value,
                  size_t size,
                  int flags)
{
        int ret;
        int ret2;
        SMBCSRV *srv;
        SMBCSRV *ipc_srv;
	fstring server;
        fstring share;
        fstring user;
        fstring password;
        fstring workgroup;
	pstring path;
        TALLOC_CTX *ctx;
        POLICY_HND pol;
        DOS_ATTR_DESC *dad;

	if (!context || !context->internal ||
	    !context->internal->_initialized) {

		errno = EINVAL;  /* Best I can think of ... */
		return -1;
    
	}

	if (!fname) {

		errno = EINVAL;
		return -1;

	}
  
	DEBUG(4, ("smbc_setxattr(%s, %s, %.*s)\n",
                  fname, name, (int) size, (const char*)value));

	if (smbc_parse_path(context, fname,
                            workgroup, sizeof(workgroup),
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            NULL, 0)) {
                errno = EINVAL;
                return -1;
        }

	if (user[0] == (char)0) fstrcpy(user, context->user);

	srv = smbc_server(context, True,
                          server, share, workgroup, user, password);
	if (!srv) {
		return -1;  /* errno set by smbc_server */
	}

        if (! srv->no_nt_session) {
                ipc_srv = smbc_attr_server(context, server, share,
                                           workgroup, user, password,
                                           &pol);
                srv->no_nt_session = True;
        } else {
                ipc_srv = NULL;
        }
        
        ctx = talloc_init("smbc_setxattr");
        if (!ctx) {
                errno = ENOMEM;
                return -1;
        }

        /*
         * Are they asking to set the entire set of known attributes?
         */
        if (StrCaseCmp(name, "system.*") == 0 ||
            StrCaseCmp(name, "system.*+") == 0) {
                /* Yup. */
                char *namevalue =
                        talloc_asprintf(ctx, "%s:%s",
                                        name+7, (const char *) value);
                if (! namevalue) {
                        errno = ENOMEM;
                        ret = -1;
                        return -1;
                }

                if (ipc_srv) {
                        ret = cacl_set(ctx, &srv->cli,
                                       &ipc_srv->cli, &pol, path,
                                       namevalue,
                                       (*namevalue == '*'
                                        ? SMBC_XATTR_MODE_SET
                                        : SMBC_XATTR_MODE_ADD),
                                       flags);
                } else {
                        ret = 0;
                }

                /* get a DOS Attribute Descriptor with current attributes */
                dad = dos_attr_query(context, ctx, path, srv);
                if (dad) {
                        /* Overwrite old with new, using what was provided */
                        dos_attr_parse(context, dad, srv, namevalue);

                        /* Set the new DOS attributes */
                        if (! smbc_setatr(context, srv, path,
                                          dad->c_time,
                                          dad->a_time,
                                          dad->m_time,
                                          dad->mode)) {

                                /* cause failure if NT failed too */
                                dad = NULL; 
                        }
                }

                /* we only fail if both NT and DOS sets failed */
                if (ret < 0 && ! dad) {
                        ret = -1; /* in case dad was null */
                }
                else {
                        ret = 0;
                }

                talloc_destroy(ctx);
                return ret;
        }

        /*
         * Are they asking to set an access control element or to set
         * the entire access control list?
         */
        if (StrCaseCmp(name, "system.nt_sec_desc.*") == 0 ||
            StrCaseCmp(name, "system.nt_sec_desc.*+") == 0 ||
            StrCaseCmp(name, "system.nt_sec_desc.revision") == 0 ||
            StrnCaseCmp(name, "system.nt_sec_desc.acl", 22) == 0 ||
            StrnCaseCmp(name, "system.nt_sec_desc.acl+", 23) == 0) {

                /* Yup. */
                char *namevalue =
                        talloc_asprintf(ctx, "%s:%s",
                                        name+19, (const char *) value);

                if (! ipc_srv) {
                        ret = -1; /* errno set by smbc_server() */
                }
                else if (! namevalue) {
                        errno = ENOMEM;
                        ret = -1;
                } else {
                        ret = cacl_set(ctx, &srv->cli,
                                       &ipc_srv->cli, &pol, path,
                                       namevalue,
                                       (*namevalue == '*'
                                        ? SMBC_XATTR_MODE_SET
                                        : SMBC_XATTR_MODE_ADD),
                                       flags);
                }
                talloc_destroy(ctx);
                return ret;
        }

        /*
         * Are they asking to set the owner?
         */
        if (StrCaseCmp(name, "system.nt_sec_desc.owner") == 0 ||
            StrCaseCmp(name, "system.nt_sec_desc.owner+") == 0) {

                /* Yup. */
                char *namevalue =
                        talloc_asprintf(ctx, "%s:%s",
                                        name+19, (const char *) value);

                if (! ipc_srv) {
                        
                        ret = -1; /* errno set by smbc_server() */
                }
                else if (! namevalue) {
                        errno = ENOMEM;
                        ret = -1;
                } else {
                        ret = cacl_set(ctx, &srv->cli,
                                       &ipc_srv->cli, &pol, path,
                                       namevalue, SMBC_XATTR_MODE_CHOWN, 0);
                }
                talloc_destroy(ctx);
                return ret;
        }

        /*
         * Are they asking to set the group?
         */
        if (StrCaseCmp(name, "system.nt_sec_desc.group") == 0 ||
            StrCaseCmp(name, "system.nt_sec_desc.group+") == 0) {

                /* Yup. */
                char *namevalue =
                        talloc_asprintf(ctx, "%s:%s",
                                        name+19, (const char *) value);

                if (! ipc_srv) {
                        /* errno set by smbc_server() */
                        ret = -1;
                }
                else if (! namevalue) {
                        errno = ENOMEM;
                        ret = -1;
                } else {
                        ret = cacl_set(ctx, &srv->cli,
                                       &ipc_srv->cli, &pol, path,
                                       namevalue, SMBC_XATTR_MODE_CHOWN, 0);
                }
                talloc_destroy(ctx);
                return ret;
        }

        /*
         * Are they asking to set a DOS attribute?
         */
        if (StrCaseCmp(name, "system.dos_attr.*") == 0 ||
            StrCaseCmp(name, "system.dos_attr.mode") == 0 ||
            StrCaseCmp(name, "system.dos_attr.c_time") == 0 ||
            StrCaseCmp(name, "system.dos_attr.a_time") == 0 ||
            StrCaseCmp(name, "system.dos_attr.m_time") == 0) {

                /* get a DOS Attribute Descriptor with current attributes */
                dad = dos_attr_query(context, ctx, path, srv);
                if (dad) {
                        char *namevalue =
                                talloc_asprintf(ctx, "%s:%s",
                                                name+16, (const char *) value);
                        if (! namevalue) {
                                errno = ENOMEM;
                                ret = -1;
                        } else {
                                /* Overwrite old with provided new params */
                                dos_attr_parse(context, dad, srv, namevalue);

                                /* Set the new DOS attributes */
                                ret2 = smbc_setatr(context, srv, path,
                                                   dad->c_time,
                                                   dad->a_time,
                                                   dad->m_time,
                                                   dad->mode);

                                /* ret2 has True (success) / False (failure) */
                                if (ret2) {
                                        ret = 0;
                                } else {
                                        ret = -1;
                                }
                        }
                } else {
                        ret = -1;
                }

                talloc_destroy(ctx);
                return ret;
        }

        /* Unsupported attribute name */
        talloc_destroy(ctx);
        errno = EINVAL;
        return -1;
}

static int
smbc_getxattr_ctx(SMBCCTX *context,
                  const char *fname,
                  const char *name,
                  const void *value,
                  size_t size)
{
        int ret;
        SMBCSRV *srv;
        SMBCSRV *ipc_srv;
        fstring server;
        fstring share;
        fstring user;
        fstring password;
        fstring workgroup;
        pstring path;
        TALLOC_CTX *ctx;
        POLICY_HND pol;


        if (!context || !context->internal ||
            !context->internal->_initialized) {

                errno = EINVAL;  /* Best I can think of ... */
                return -1;
    
        }

        if (!fname) {

                errno = EINVAL;
                return -1;

        }
  
        DEBUG(4, ("smbc_getxattr(%s, %s)\n", fname, name));

        if (smbc_parse_path(context, fname,
                            workgroup, sizeof(workgroup),
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            NULL, 0)) {
                errno = EINVAL;
                return -1;
        }

        if (user[0] == (char)0) fstrcpy(user, context->user);

        srv = smbc_server(context, True,
                          server, share, workgroup, user, password);
        if (!srv) {
                return -1;  /* errno set by smbc_server */
        }

        if (! srv->no_nt_session) {
                ipc_srv = smbc_attr_server(context, server, share,
                                           workgroup, user, password,
                                           &pol);
                if (! ipc_srv) {
                        srv->no_nt_session = True;
                }
        } else {
                ipc_srv = NULL;
        }
        
        ctx = talloc_init("smbc:getxattr");
        if (!ctx) {
                errno = ENOMEM;
                return -1;
        }

        /* Are they requesting a supported attribute? */
        if (StrCaseCmp(name, "system.*") == 0 ||
            StrnCaseCmp(name, "system.*!", 9) == 0 ||
            StrCaseCmp(name, "system.*+") == 0 ||
            StrnCaseCmp(name, "system.*+!", 10) == 0 ||
            StrCaseCmp(name, "system.nt_sec_desc.*") == 0 ||
            StrnCaseCmp(name, "system.nt_sec_desc.*!", 21) == 0 ||
            StrCaseCmp(name, "system.nt_sec_desc.*+") == 0 ||
            StrnCaseCmp(name, "system.nt_sec_desc.*+!", 22) == 0 ||
            StrCaseCmp(name, "system.nt_sec_desc.revision") == 0 ||
            StrCaseCmp(name, "system.nt_sec_desc.owner") == 0 ||
            StrCaseCmp(name, "system.nt_sec_desc.owner+") == 0 ||
            StrCaseCmp(name, "system.nt_sec_desc.group") == 0 ||
            StrCaseCmp(name, "system.nt_sec_desc.group+") == 0 ||
            StrnCaseCmp(name, "system.nt_sec_desc.acl", 22) == 0 ||
            StrnCaseCmp(name, "system.nt_sec_desc.acl+", 23) == 0 ||
            StrCaseCmp(name, "system.dos_attr.*") == 0 ||
            StrnCaseCmp(name, "system.dos_attr.*!", 18) == 0 ||
            StrCaseCmp(name, "system.dos_attr.mode") == 0 ||
            StrCaseCmp(name, "system.dos_attr.size") == 0 ||
            StrCaseCmp(name, "system.dos_attr.c_time") == 0 ||
            StrCaseCmp(name, "system.dos_attr.a_time") == 0 ||
            StrCaseCmp(name, "system.dos_attr.m_time") == 0 ||
            StrCaseCmp(name, "system.dos_attr.inode") == 0) {

                /* Yup. */
                ret = cacl_get(context, ctx, srv,
                               ipc_srv == NULL ? NULL : &ipc_srv->cli, 
                               &pol, path,
                               CONST_DISCARD(char *, name),
                               CONST_DISCARD(char *, value), size);
                if (ret < 0 && errno == 0) {
                        errno = smbc_errno(context, &srv->cli);
                }
                talloc_destroy(ctx);
                return ret;
        }

        /* Unsupported attribute name */
        talloc_destroy(ctx);
        errno = EINVAL;
        return -1;
}


static int
smbc_removexattr_ctx(SMBCCTX *context,
                     const char *fname,
                     const char *name)
{
        int ret;
        SMBCSRV *srv;
        SMBCSRV *ipc_srv;
        fstring server;
        fstring share;
        fstring user;
        fstring password;
        fstring workgroup;
        pstring path;
        TALLOC_CTX *ctx;
        POLICY_HND pol;

        if (!context || !context->internal ||
            !context->internal->_initialized) {

                errno = EINVAL;  /* Best I can think of ... */
                return -1;
    
        }

        if (!fname) {

                errno = EINVAL;
                return -1;

        }
  
        DEBUG(4, ("smbc_removexattr(%s, %s)\n", fname, name));

        if (smbc_parse_path(context, fname,
                            workgroup, sizeof(workgroup),
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            NULL, 0)) {
                errno = EINVAL;
                return -1;
        }

        if (user[0] == (char)0) fstrcpy(user, context->user);

        srv = smbc_server(context, True,
                          server, share, workgroup, user, password);
        if (!srv) {
                return -1;  /* errno set by smbc_server */
        }

        if (! srv->no_nt_session) {
                ipc_srv = smbc_attr_server(context, server, share,
                                           workgroup, user, password,
                                           &pol);
                srv->no_nt_session = True;
        } else {
                ipc_srv = NULL;
        }
        
        if (! ipc_srv) {
                return -1; /* errno set by smbc_attr_server */
        }

        ctx = talloc_init("smbc_removexattr");
        if (!ctx) {
                errno = ENOMEM;
                return -1;
        }

        /* Are they asking to set the entire ACL? */
        if (StrCaseCmp(name, "system.nt_sec_desc.*") == 0 ||
            StrCaseCmp(name, "system.nt_sec_desc.*+") == 0) {

                /* Yup. */
                ret = cacl_set(ctx, &srv->cli,
                               &ipc_srv->cli, &pol, path,
                               NULL, SMBC_XATTR_MODE_REMOVE_ALL, 0);
                talloc_destroy(ctx);
                return ret;
        }

        /*
         * Are they asking to remove one or more spceific security descriptor
         * attributes?
         */
        if (StrCaseCmp(name, "system.nt_sec_desc.revision") == 0 ||
            StrCaseCmp(name, "system.nt_sec_desc.owner") == 0 ||
            StrCaseCmp(name, "system.nt_sec_desc.owner+") == 0 ||
            StrCaseCmp(name, "system.nt_sec_desc.group") == 0 ||
            StrCaseCmp(name, "system.nt_sec_desc.group+") == 0 ||
            StrnCaseCmp(name, "system.nt_sec_desc.acl", 22) == 0 ||
            StrnCaseCmp(name, "system.nt_sec_desc.acl+", 23) == 0) {

                /* Yup. */
                ret = cacl_set(ctx, &srv->cli,
                               &ipc_srv->cli, &pol, path,
                               name + 19, SMBC_XATTR_MODE_REMOVE, 0);
                talloc_destroy(ctx);
                return ret;
        }

        /* Unsupported attribute name */
        talloc_destroy(ctx);
        errno = EINVAL;
        return -1;
}

static int
smbc_listxattr_ctx(SMBCCTX *context,
                   const char *fname,
                   char *list,
                   size_t size)
{
        /*
         * This isn't quite what listxattr() is supposed to do.  This returns
         * the complete set of attribute names, always, rather than only those
         * attribute names which actually exist for a file.  Hmmm...
         */
        const char supported[] =
                "system.*\0"
                "system.*+\0"
                "system.nt_sec_desc.revision\0"
                "system.nt_sec_desc.owner\0"
                "system.nt_sec_desc.owner+\0"
                "system.nt_sec_desc.group\0"
                "system.nt_sec_desc.group+\0"
                "system.nt_sec_desc.acl.*\0"
                "system.nt_sec_desc.acl\0"
                "system.nt_sec_desc.acl+\0"
                "system.nt_sec_desc.*\0"
                "system.nt_sec_desc.*+\0"
                "system.dos_attr.*\0"
                "system.dos_attr.mode\0"
                "system.dos_attr.c_time\0"
                "system.dos_attr.a_time\0"
                "system.dos_attr.m_time\0"
                ;

        if (size == 0) {
                return sizeof(supported);
        }

        if (sizeof(supported) > size) {
                errno = ERANGE;
                return -1;
        }

        /* this can't be strcpy() because there are embedded null characters */
        memcpy(list, supported, sizeof(supported));
        return sizeof(supported);
}


/*
 * Open a print file to be written to by other calls
 */

static SMBCFILE *
smbc_open_print_job_ctx(SMBCCTX *context,
                        const char *fname)
{
        fstring server;
        fstring share;
        fstring user;
        fstring password;
        pstring path;
        
        if (!context || !context->internal ||
            !context->internal->_initialized) {

                errno = EINVAL;
                return NULL;
    
        }

        if (!fname) {

                errno = EINVAL;
                return NULL;

        }
  
        DEBUG(4, ("smbc_open_print_job_ctx(%s)\n", fname));

        if (smbc_parse_path(context, fname,
                            NULL, 0,
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            NULL, 0)) {
                errno = EINVAL;
                return NULL;
        }

        /* What if the path is empty, or the file exists? */

        return context->open(context, fname, O_WRONLY, 666);

}

/*
 * Routine to print a file on a remote server ...
 *
 * We open the file, which we assume to be on a remote server, and then
 * copy it to a print file on the share specified by printq.
 */

static int
smbc_print_file_ctx(SMBCCTX *c_file,
                    const char *fname,
                    SMBCCTX *c_print,
                    const char *printq)
{
        SMBCFILE *fid1;
        SMBCFILE *fid2;
        int bytes;
        int saverr;
        int tot_bytes = 0;
        char buf[4096];

        if (!c_file || !c_file->internal->_initialized || !c_print ||
            !c_print->internal->_initialized) {

                errno = EINVAL;
                return -1;

        }

        if (!fname && !printq) {

                errno = EINVAL;
                return -1;

        }

        /* Try to open the file for reading ... */

        if ((long)(fid1 = c_file->open(c_file, fname, O_RDONLY, 0666)) < 0) {
                
                DEBUG(3, ("Error, fname=%s, errno=%i\n", fname, errno));
                return -1;  /* smbc_open sets errno */
                
        }

        /* Now, try to open the printer file for writing */

        if ((long)(fid2 = c_print->open_print_job(c_print, printq)) < 0) {

                saverr = errno;  /* Save errno */
                c_file->close_fn(c_file, fid1);
                errno = saverr;
                return -1;

        }

        while ((bytes = c_file->read(c_file, fid1, buf, sizeof(buf))) > 0) {

                tot_bytes += bytes;

                if ((c_print->write(c_print, fid2, buf, bytes)) < 0) {

                        saverr = errno;
                        c_file->close_fn(c_file, fid1);
                        c_print->close_fn(c_print, fid2);
                        errno = saverr;

                }

        }

        saverr = errno;

        c_file->close_fn(c_file, fid1);  /* We have to close these anyway */
        c_print->close_fn(c_print, fid2);

        if (bytes < 0) {

                errno = saverr;
                return -1;

        }

        return tot_bytes;

}

/*
 * Routine to list print jobs on a printer share ...
 */

static int
smbc_list_print_jobs_ctx(SMBCCTX *context,
                         const char *fname,
                         smbc_list_print_job_fn fn)
{
        SMBCSRV *srv;
        fstring server;
        fstring share;
        fstring user;
        fstring password;
        fstring workgroup;
        pstring path;

        if (!context || !context->internal ||
            !context->internal->_initialized) {

                errno = EINVAL;
                return -1;

        }

        if (!fname) {
                
                errno = EINVAL;
                return -1;

        }
  
        DEBUG(4, ("smbc_list_print_jobs(%s)\n", fname));

        if (smbc_parse_path(context, fname,
                            workgroup, sizeof(workgroup),
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            NULL, 0)) {
                errno = EINVAL;
                return -1;
        }

        if (user[0] == (char)0) fstrcpy(user, context->user);
        
        srv = smbc_server(context, True,
                          server, share, workgroup, user, password);

        if (!srv) {

                return -1;  /* errno set by smbc_server */

        }

        if (cli_print_queue(&srv->cli,
                            (void (*)(struct print_job_info *))fn) < 0) {

                errno = smbc_errno(context, &srv->cli);
                return -1;

        }
        
        return 0;

}

/*
 * Delete a print job from a remote printer share
 */

static int
smbc_unlink_print_job_ctx(SMBCCTX *context,
                          const char *fname,
                          int id)
{
        SMBCSRV *srv;
        fstring server;
        fstring share;
        fstring user;
        fstring password;
        fstring workgroup;
        pstring path;
        int err;

        if (!context || !context->internal ||
            !context->internal->_initialized) {

                errno = EINVAL;
                return -1;

        }

        if (!fname) {

                errno = EINVAL;
                return -1;

        }
  
        DEBUG(4, ("smbc_unlink_print_job(%s)\n", fname));

        if (smbc_parse_path(context, fname,
                            workgroup, sizeof(workgroup),
                            server, sizeof(server),
                            share, sizeof(share),
                            path, sizeof(path),
                            user, sizeof(user),
                            password, sizeof(password),
                            NULL, 0)) {
                errno = EINVAL;
                return -1;
        }

        if (user[0] == (char)0) fstrcpy(user, context->user);

        srv = smbc_server(context, True,
                          server, share, workgroup, user, password);

        if (!srv) {

                return -1;  /* errno set by smbc_server */

        }

        if ((err = cli_printjob_del(&srv->cli, id)) != 0) {

                if (err < 0)
                        errno = smbc_errno(context, &srv->cli);
                else if (err == ERRnosuchprintjob)
                        errno = EINVAL;
                return -1;

        }

        return 0;

}

/*
 * Get a new empty handle to fill in with your own info 
 */
SMBCCTX *
smbc_new_context(void)
{
        SMBCCTX *context;

        context = SMB_MALLOC_P(SMBCCTX);
        if (!context) {
                errno = ENOMEM;
                return NULL;
        }

        ZERO_STRUCTP(context);

        context->internal = SMB_MALLOC_P(struct smbc_internal_data);
        if (!context->internal) {
		SAFE_FREE(context);
                errno = ENOMEM;
                return NULL;
        }

        ZERO_STRUCTP(context->internal);

        
        /* ADD REASONABLE DEFAULTS */
        context->debug            = 0;
        context->timeout          = 20000; /* 20 seconds */

	context->options.browse_max_lmb_count      = 3;    /* # LMBs to query */
	context->options.urlencode_readdir_entries = False;/* backward compat */
	context->options.one_share_per_server      = False;/* backward compat */

        context->open                              = smbc_open_ctx;
        context->creat                             = smbc_creat_ctx;
        context->read                              = smbc_read_ctx;
        context->write                             = smbc_write_ctx;
        context->close_fn                          = smbc_close_ctx;
        context->unlink                            = smbc_unlink_ctx;
        context->rename                            = smbc_rename_ctx;
        context->lseek                             = smbc_lseek_ctx;
        context->stat                              = smbc_stat_ctx;
        context->fstat                             = smbc_fstat_ctx;
        context->opendir                           = smbc_opendir_ctx;
        context->closedir                          = smbc_closedir_ctx;
        context->readdir                           = smbc_readdir_ctx;
        context->getdents                          = smbc_getdents_ctx;
        context->mkdir                             = smbc_mkdir_ctx;
        context->rmdir                             = smbc_rmdir_ctx;
        context->telldir                           = smbc_telldir_ctx;
        context->lseekdir                          = smbc_lseekdir_ctx;
        context->fstatdir                          = smbc_fstatdir_ctx;
        context->chmod                             = smbc_chmod_ctx;
        context->utimes                            = smbc_utimes_ctx;
        context->setxattr                          = smbc_setxattr_ctx;
        context->getxattr                          = smbc_getxattr_ctx;
        context->removexattr                       = smbc_removexattr_ctx;
        context->listxattr                         = smbc_listxattr_ctx;
        context->open_print_job                    = smbc_open_print_job_ctx;
        context->print_file                        = smbc_print_file_ctx;
        context->list_print_jobs                   = smbc_list_print_jobs_ctx;
        context->unlink_print_job                  = smbc_unlink_print_job_ctx;

        context->callbacks.check_server_fn         = smbc_check_server;
        context->callbacks.remove_unused_server_fn = smbc_remove_unused_server;

        smbc_default_cache_functions(context);

        return context;
}

/* 
 * Free a context
 *
 * Returns 0 on success. Otherwise returns 1, the SMBCCTX is _not_ freed 
 * and thus you'll be leaking memory if not handled properly.
 *
 */
int
smbc_free_context(SMBCCTX *context,
                  int shutdown_ctx)
{
        if (!context) {
                errno = EBADF;
                return 1;
        }
        
        if (shutdown_ctx) {
                SMBCFILE * f;
                DEBUG(1,("Performing aggressive shutdown.\n"));
                
                f = context->internal->_files;
                while (f) {
                        context->close_fn(context, f);
                        f = f->next;
                }
                context->internal->_files = NULL;

                /* First try to remove the servers the nice way. */
                if (context->callbacks.purge_cached_fn(context)) {
                        SMBCSRV * s;
                        SMBCSRV * next;
                        DEBUG(1, ("Could not purge all servers, "
                                  "Nice way shutdown failed.\n"));
                        s = context->internal->_servers;
                        while (s) {
                                DEBUG(1, ("Forced shutdown: %p (fd=%d)\n",
                                          s, s->cli.fd));
                                cli_shutdown(&s->cli);
                                context->callbacks.remove_cached_srv_fn(context,
                                                                        s);
                                next = s->next;
                                DLIST_REMOVE(context->internal->_servers, s);
                                SAFE_FREE(s);
                                s = next;
                        }
                        context->internal->_servers = NULL;
                }
        }
        else {
                /* This is the polite way */    
                if (context->callbacks.purge_cached_fn(context)) {
                        DEBUG(1, ("Could not purge all servers, "
                                  "free_context failed.\n"));
                        errno = EBUSY;
                        return 1;
                }
                if (context->internal->_servers) {
                        DEBUG(1, ("Active servers in context, "
                                  "free_context failed.\n"));
                        errno = EBUSY;
                        return 1;
                }
                if (context->internal->_files) {
                        DEBUG(1, ("Active files in context, "
                                  "free_context failed.\n"));
                        errno = EBUSY;
                        return 1;
                }               
        }

        /* Things we have to clean up */
        SAFE_FREE(context->workgroup);
        SAFE_FREE(context->netbios_name);
        SAFE_FREE(context->user);
        
        DEBUG(3, ("Context %p succesfully freed\n", context));
        SAFE_FREE(context->internal);
        SAFE_FREE(context);
        return 0;
}


/*
 * Each time the context structure is changed, we have binary backward
 * compatibility issues.  Instead of modifying the public portions of the
 * context structure to add new options, instead, we put them in the internal
 * portion of the context structure and provide a set function for these new
 * options.
 */
void
smbc_option_set(SMBCCTX *context,
                char *option_name,
                void *option_value)
{
        if (strcmp(option_name, "debug_stderr") == 0) {
                /*
                 * Log to standard error instead of standard output.
                 */
                context->internal->_debug_stderr =
                        (option_value == NULL ? False : True);
        } else if (strcmp(option_name, "auth_function") == 0) {
                /*
                 * Use the new-style authentication function which includes
                 * the context.
                 */
                context->internal->_auth_fn_with_context = option_value;
        } else if (strcmp(option_name, "user_data") == 0) {
                /*
                 * Save a user data handle which may be retrieved by the user
                 * with smbc_option_get()
                 */
                context->internal->_user_data = option_value;
        }
}


/*
 * Retrieve the current value of an option
 */
void *
smbc_option_get(SMBCCTX *context,
                char *option_name)
{
        if (strcmp(option_name, "debug_stderr") == 0) {
                /*
                 * Log to standard error instead of standard output.
                 */
#if defined(__intptr_t_defined) || defined(HAVE_INTPTR_T)
                return (void *) (intptr_t) context->internal->_debug_stderr;
#else
                return (void *) context->internal->_debug_stderr;
#endif
        } else if (strcmp(option_name, "auth_function") == 0) {
                /*
                 * Use the new-style authentication function which includes
                 * the context.
                 */
                return (void *) context->internal->_auth_fn_with_context;
        } else if (strcmp(option_name, "user_data") == 0) {
                /*
                 * Save a user data handle which may be retrieved by the user
                 * with smbc_option_get()
                 */
                return context->internal->_user_data;
        }

        return NULL;
}


/*
 * Initialise the library etc 
 *
 * We accept a struct containing handle information.
 * valid values for info->debug from 0 to 100,
 * and insist that info->fn must be non-null.
 */
SMBCCTX *
smbc_init_context(SMBCCTX *context)
{
        pstring conf;
        int pid;
        char *user = NULL;
        char *home = NULL;

        if (!context || !context->internal) {
                errno = EBADF;
                return NULL;
        }

        /* Do not initialise the same client twice */
        if (context->internal->_initialized) { 
                return 0;
        }

        if ((!context->callbacks.auth_fn &&
             !context->internal->_auth_fn_with_context) ||
            context->debug < 0 ||
            context->debug > 100) {

                errno = EINVAL;
                return NULL;

        }

        if (!smbc_initialized) {
                /*
                 * Do some library-wide intializations the first time we get
                 * called
                 */
		BOOL conf_loaded = False;

                /* Set this to what the user wants */
                DEBUGLEVEL = context->debug;
                
                load_case_tables();

                setup_logging("libsmbclient", True);
                if (context->internal->_debug_stderr) {
                        dbf = x_stderr;
                        x_setbuf(x_stderr, NULL);
                }

                /* Here we would open the smb.conf file if needed ... */
                
                in_client = True; /* FIXME, make a param */
#ifdef _XBOX
				home = "Q:";
				slprintf(conf, sizeof(conf), "%s\\smb.conf", home);
#elif //_XBOX
                home = getenv("HOME");
#endif //_XBOX
		if (home) {
			slprintf(conf, sizeof(conf), "%s/.smb/smb.conf", home);
			if (lp_load(conf, True, False, False, True)) {
				conf_loaded = True;
			} else {
                                DEBUG(5, ("Could not load config file: %s\n",
                                          conf));
			}
               	}
 
		if (!conf_loaded) {
                        /*
                         * Well, if that failed, try the dyn_CONFIGFILE
                         * Which points to the standard locn, and if that
                         * fails, silently ignore it and use the internal
                         * defaults ...
                         */

                        if (!lp_load(dyn_CONFIGFILE, True, False, False, False)) {
                                DEBUG(5, ("Could not load config file: %s\n",
                                          dyn_CONFIGFILE));
                        } else if (home) {
                                /*
                                 * We loaded the global config file.  Now lets
                                 * load user-specific modifications to the
                                 * global config.
                                 */
                                slprintf(conf, sizeof(conf),
                                         "%s/.smb/smb.conf.append", home);
                                if (!lp_load(conf, True, False, False, False)) {
                                        DEBUG(10,
                                              ("Could not append config file: "
                                               "%s\n",
                                               conf));
                                }
                        }
                }

                load_interfaces();  /* Load the list of interfaces ... */
                
                reopen_logs();  /* Get logging working ... */
        
                /* 
                 * Block SIGPIPE (from lib/util_sock.c: write())  
                 * It is not needed and should not stop execution 
                 */
#ifdef _XBOX
                // OutputDebugString("Todo: smbc_init_context\n");
#elif //_XBOX
                BlockSignals(True, SIGPIPE);
#endif //_XBOX
                
                /* Done with one-time initialisation */
                smbc_initialized = 1; 

        }
        
        if (!context->user) {
                /*
                 * FIXME: Is this the best way to get the user info? 
                 */
#ifdef _XBOX
                user = NULL;
#elif //_XBOX
                user = getenv("USER");
#endif //_XBOX
                /* walk around as "guest" if no username can be found */
                if (!user) context->user = SMB_STRDUP("guest");
                else context->user = SMB_STRDUP(user);
        }

        if (!context->netbios_name) {
                /*
                 * We try to get our netbios name from the config. If that
                 * fails we fall back on constructing our netbios name from
                 * our hostname etc
                 */
                if (global_myname()) {
                        context->netbios_name = SMB_STRDUP(global_myname());
                }
                else {
                        /*
                         * Hmmm, I want to get hostname as well, but I am too
                         * lazy for the moment
                         */
                        pid = sys_getpid();
                        context->netbios_name = SMB_MALLOC(17);
                        if (!context->netbios_name) {
                                errno = ENOMEM;
                                return NULL;
                        }
                        slprintf(context->netbios_name, 16,
                                 "smbc%s%d", context->user, pid);
                }
        }

        DEBUG(1, ("Using netbios name %s.\n", context->netbios_name));

        if (!context->workgroup) {
                if (lp_workgroup()) {
                        context->workgroup = SMB_STRDUP(lp_workgroup());
                }
                else {
                        /* TODO: Think about a decent default workgroup */
                        context->workgroup = SMB_STRDUP("samba");
                }
        }

        DEBUG(1, ("Using workgroup %s.\n", context->workgroup));
                                        
        /* shortest timeout is 1 second */
        if (context->timeout > 0 && context->timeout < 1000) 
                context->timeout = 1000;

        /*
         * FIXME: Should we check the function pointers here? 
         */

        context->internal->_initialized = True;
        
        return context;
}


/* Return the verion of samba, and thus libsmbclient */
const char *
smbc_version(void)
{
        return samba_version_string();
}
