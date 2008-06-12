/* 
   Unix SMB/CIFS implementation.
   service (connection) opening and closing
   Copyright (C) Andrew Tridgell 1992-1998
   
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

extern userdom_struct current_user_info;

/****************************************************************************
 Ensure when setting connectpath it is a canonicalized (no ./ // or ../)
 absolute path stating in / and not ending in /.
 Observent people will notice a similarity between this and check_path_syntax :-).
****************************************************************************/

void set_conn_connectpath(connection_struct *conn, const pstring connectpath)
{
	pstring destname;
	char *d = destname;
	const char *s = connectpath;
        BOOL start_of_name_component = True;

	*d++ = '/'; /* Always start with root. */

	while (*s) {
		if (*s == '/') {
			/* Eat multiple '/' */
			while (*s == '/') {
                                s++;
                        }
			if ((d > destname + 1) && (*s != '\0')) {
				*d++ = '/';
			}
			start_of_name_component = True;
			continue;
		}

		if (start_of_name_component) {
			if ((s[0] == '.') && (s[1] == '.') && (s[2] == '/' || s[2] == '\0')) {
				/* Uh oh - "/../" or "/..\0" ! */

				/* Go past the ../ or .. */
				if (s[2] == '/') {
					s += 3;
				} else {
					s += 2; /* Go past the .. */
				}

				/* If  we just added a '/' - delete it */
				if ((d > destname) && (*(d-1) == '/')) {
					*(d-1) = '\0';
					d--;
				}

				/* Are we at the start ? Can't go back further if so. */
				if (d <= destname) {
					*d++ = '/'; /* Can't delete root */
					continue;
				}
				/* Go back one level... */
				/* Decrement d first as d points to the *next* char to write into. */
				for (d--; d > destname; d--) {
					if (*d == '/') {
						break;
					}
				}
				/* We're still at the start of a name component, just the previous one. */
				continue;
			} else if ((s[0] == '.') && ((s[1] == '\0') || s[1] == '/')) {
				/* Component of pathname can't be "." only - skip the '.' . */
				if (s[1] == '/') {
					s += 2;
				} else {
					s++;
				}
				continue;
			}
		}

		if (!(*s & 0x80)) {
			*d++ = *s++;
		} else {
			switch(next_mb_char_size(s)) {
				case 4:
					*d++ = *s++;
				case 3:
					*d++ = *s++;
				case 2:
					*d++ = *s++;
				case 1:
					*d++ = *s++;
					break;
				default:
					break;
			}
		}
		start_of_name_component = False;
	}
	*d = '\0';

	/* And must not end in '/' */
	if (d > destname + 1 && (*(d-1) == '/')) {
		*(d-1) = '\0';
	}

	DEBUG(10,("set_conn_connectpath: service %s, connectpath = %s\n",
		lp_servicename(SNUM(conn)), destname ));

	string_set(&conn->connectpath, destname);
}

/****************************************************************************
 Load parameters specific to a connection/service.
****************************************************************************/

BOOL set_current_service(connection_struct *conn, uint16 flags, BOOL do_chdir)
{
	static connection_struct *last_conn;
	static uint16 last_flags;
	int snum;

	if (!conn)  {
		last_conn = NULL;
		return(False);
	}

	conn->lastused_count++;

	snum = SNUM(conn);
  
	if (do_chdir &&
	    vfs_ChDir(conn,conn->connectpath) != 0 &&
	    vfs_ChDir(conn,conn->origpath) != 0) {
		DEBUG(0,("chdir (%s) failed\n",
			 conn->connectpath));
		return(False);
	}

	if ((conn == last_conn) && (last_flags == flags)) {
		return(True);
	}

	last_conn = conn;
	last_flags = flags;
	
	/* Obey the client case sensitivity requests - only for clients that support it. */
	switch (lp_casesensitive(snum)) {
		case Auto:
			{
				/* We need this uglyness due to DOS/Win9x clients that lie about case insensitivity. */
				enum remote_arch_types ra_type = get_remote_arch();
				if ((ra_type != RA_SAMBA) && (ra_type != RA_CIFSFS)) {
					/* Client can't support per-packet case sensitive pathnames. */
					conn->case_sensitive = False;
				} else {
					conn->case_sensitive = !(flags & FLAG_CASELESS_PATHNAMES);
				}
			}
			break;
		case True:
			conn->case_sensitive = True;
			break;
		default:
			conn->case_sensitive = False;
			break;
	}
	return(True);
}

/****************************************************************************
 Add a home service. Returns the new service number or -1 if fail.
****************************************************************************/

int add_home_service(const char *service, const char *username, const char *homedir)
{
	int iHomeService;

	if (!service || !homedir)
		return -1;

	if ((iHomeService = lp_servicenumber(HOMES_NAME)) < 0)
		return -1;

	/*
	 * If this is a winbindd provided username, remove
	 * the domain component before adding the service.
	 * Log a warning if the "path=" parameter does not
	 * include any macros.
	 */

	{
		const char *p = strchr(service,*lp_winbind_separator());

		/* We only want the 'user' part of the string */
		if (p) {
			service = p + 1;
		}
	}

	if (!lp_add_home(service, iHomeService, username, homedir)) {
		return -1;
	}
	
	return lp_servicenumber(service);

}


/**
 * Find a service entry.
 *
 * @param service is modified (to canonical form??)
 **/

int find_service(fstring service)
{
	int iService;

	all_string_sub(service,"\\","/",0);

	iService = lp_servicenumber(service);

	/* now handle the special case of a home directory */
	if (iService < 0) {
		char *phome_dir = get_user_home_dir(service);

		if(!phome_dir) {
			/*
			 * Try mapping the servicename, it may
			 * be a Windows to unix mapped user name.
			 */
			if(map_username(service))
				phome_dir = get_user_home_dir(service);
		}

		DEBUG(3,("checking for home directory %s gave %s\n",service,
			phome_dir?phome_dir:"(NULL)"));

		iService = add_home_service(service,service /* 'username' */, phome_dir);
	}

	/* If we still don't have a service, attempt to add it as a printer. */
	if (iService < 0) {
		int iPrinterService;

		if ((iPrinterService = lp_servicenumber(PRINTERS_NAME)) >= 0) {
			DEBUG(3,("checking whether %s is a valid printer name...\n", service));
			if (pcap_printername_ok(service)) {
				DEBUG(3,("%s is a valid printer name\n", service));
				DEBUG(3,("adding %s as a printer service\n", service));
				lp_add_printer(service, iPrinterService);
				iService = lp_servicenumber(service);
				if (iService < 0) {
					DEBUG(0,("failed to add %s as a printer service!\n", service));
				}
			} else {
				DEBUG(3,("%s is not a valid printer name\n", service));
			}
		}
	}

	/* Check for default vfs service?  Unsure whether to implement this */
	if (iService < 0) {
	}

	/* just possibly it's a default service? */
	if (iService < 0) {
		char *pdefservice = lp_defaultservice();
		if (pdefservice && *pdefservice && !strequal(pdefservice,service) && !strstr_m(service,"..")) {
			/*
			 * We need to do a local copy here as lp_defaultservice() 
			 * returns one of the rotating lp_string buffers that
			 * could get overwritten by the recursive find_service() call
			 * below. Fix from Josef Hinteregger <joehtg@joehtg.co.at>.
			 */
			pstring defservice;
			pstrcpy(defservice, pdefservice);
			iService = find_service(defservice);
			if (iService >= 0) {
				all_string_sub(service, "_","/",0);
				iService = lp_add_service(service, iService);
			}
		}
	}

	/* Is it a usershare service ? */
	if (iService < 0 && *lp_usershare_path()) {
		/* Ensure the name is canonicalized. */
		strlower_m(service);
		iService = load_usershare_service(service);
	}

	if (iService >= 0) {
		if (!VALID_SNUM(iService)) {
			DEBUG(0,("Invalid snum %d for %s\n",iService, service));
			iService = -1;
		}
	}

	if (iService < 0)
		DEBUG(3,("find_service() failed to find service %s\n", service));

	return (iService);
}


/****************************************************************************
 do some basic sainity checks on the share.  
 This function modifies dev, ecode.
****************************************************************************/

static NTSTATUS share_sanity_checks(int snum, fstring dev) 
{
	
	if (!lp_snum_ok(snum) || 
	    !check_access(smbd_server_fd(), 
			  lp_hostsallow(snum), lp_hostsdeny(snum))) {    
		return NT_STATUS_ACCESS_DENIED;
	}

	if (dev[0] == '?' || !dev[0]) {
		if (lp_print_ok(snum)) {
			fstrcpy(dev,"LPT1:");
		} else if (strequal(lp_fstype(snum), "IPC")) {
			fstrcpy(dev, "IPC");
		} else {
			fstrcpy(dev,"A:");
		}
	}

	strupper_m(dev);

	if (lp_print_ok(snum)) {
		if (!strequal(dev, "LPT1:")) {
			return NT_STATUS_BAD_DEVICE_TYPE;
		}
	} else if (strequal(lp_fstype(snum), "IPC")) {
		if (!strequal(dev, "IPC")) {
			return NT_STATUS_BAD_DEVICE_TYPE;
		}
	} else if (!strequal(dev, "A:")) {
		return NT_STATUS_BAD_DEVICE_TYPE;
	}

	/* Behave as a printer if we are supposed to */
	if (lp_print_ok(snum) && (strcmp(dev, "A:") == 0)) {
		fstrcpy(dev, "LPT1:");
	}

	return NT_STATUS_OK;
}

static NTSTATUS find_forced_user(int snum, BOOL vuser_is_guest,
				 uid_t *uid, gid_t *gid, fstring username,
				 struct nt_user_token **token)
{
	TALLOC_CTX *mem_ctx;
	char *fuser, *found_username;
	NTSTATUS result;

	mem_ctx = talloc_new(NULL);
	if (mem_ctx == NULL) {
		DEBUG(0, ("talloc_new failed\n"));
		return NT_STATUS_NO_MEMORY;
	}

	fuser = talloc_string_sub(mem_ctx, lp_force_user(snum), "%S",
				  lp_servicename(snum));
	if (fuser == NULL) {
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}

	result = create_token_from_username(mem_ctx, fuser, vuser_is_guest,
					    uid, gid, &found_username,
					    token);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	talloc_steal(NULL, *token);
	fstrcpy(username, found_username);

	result = NT_STATUS_OK;
 done:
	TALLOC_FREE(mem_ctx);
	return result;
}

/*
 * Go through lookup_name etc to find the force'd group.  
 *
 * Create a new token from src_token, replacing the primary group sid with the
 * one found.
 */

static NTSTATUS find_forced_group(BOOL force_user,
				  int snum, const char *username,
				  DOM_SID *pgroup_sid,
				  gid_t *pgid)
{
	NTSTATUS result = NT_STATUS_NO_SUCH_GROUP;
	TALLOC_CTX *mem_ctx;
	DOM_SID group_sid;
	enum SID_NAME_USE type;
	char *groupname;
	BOOL user_must_be_member = False;
	gid_t gid;

	ZERO_STRUCTP(pgroup_sid);
	*pgid = (gid_t)-1;

	mem_ctx = talloc_new(NULL);
	if (mem_ctx == NULL) {
		DEBUG(0, ("talloc_new failed\n"));
		return NT_STATUS_NO_MEMORY;
	}

	groupname = talloc_strdup(mem_ctx, lp_force_group(snum));
	if (groupname == NULL) {
		DEBUG(1, ("talloc_strdup failed\n"));
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}

	if (groupname[0] == '+') {
		user_must_be_member = True;
		groupname += 1;
	}

	groupname = talloc_string_sub(mem_ctx, groupname,
				      "%S", lp_servicename(snum));

	if (!lookup_name_smbconf(mem_ctx, groupname,
			 LOOKUP_NAME_ALL|LOOKUP_NAME_GROUP,
			 NULL, NULL, &group_sid, &type)) {
		DEBUG(10, ("lookup_name_smbconf(%s) failed\n",
			   groupname));
		goto done;
	}

	if ((type != SID_NAME_DOM_GRP) && (type != SID_NAME_ALIAS) &&
	    (type != SID_NAME_WKN_GRP)) {
		DEBUG(10, ("%s is a %s, not a group\n", groupname,
			   sid_type_lookup(type)));
		goto done;
	}

	if (!sid_to_gid(&group_sid, &gid)) {
		DEBUG(10, ("sid_to_gid(%s) for %s failed\n",
			   sid_string_static(&group_sid), groupname));
		goto done;
	}

	/*
	 * If the user has been forced and the forced group starts with a '+',
	 * then we only set the group to be the forced group if the forced
	 * user is a member of that group.  Otherwise, the meaning of the '+'
	 * would be ignored.
	 */

	if (force_user && user_must_be_member) {
		if (user_in_group_sid(username, &group_sid)) {
			sid_copy(pgroup_sid, &group_sid);
			*pgid = gid;
			DEBUG(3,("Forced group %s for member %s\n",
				 groupname, username));
		} else {
			DEBUG(0,("find_forced_group: forced user %s is not a member "
				"of forced group %s. Disallowing access.\n",
				username, groupname ));
			result = NT_STATUS_MEMBER_NOT_IN_GROUP;
			goto done;
		}
	} else {
		sid_copy(pgroup_sid, &group_sid);
		*pgid = gid;
		DEBUG(3,("Forced group %s\n", groupname));
	}

	result = NT_STATUS_OK;
 done:
	TALLOC_FREE(mem_ctx);
	return result;
}

/****************************************************************************
  Make a connection, given the snum to connect to, and the vuser of the
  connecting user if appropriate.
****************************************************************************/

static connection_struct *make_connection_snum(int snum, user_struct *vuser,
					       DATA_BLOB password, 
					       const char *pdev,
					       NTSTATUS *status)
{
	struct passwd *pass = NULL;
	BOOL guest = False;
	connection_struct *conn;
	SMB_STRUCT_STAT st;
	fstring user;
	fstring dev;
	int ret;

	*user = 0;
	fstrcpy(dev, pdev);
	SET_STAT_INVALID(st);

	if (NT_STATUS_IS_ERR(*status = share_sanity_checks(snum, dev))) {
		return NULL;
	}	

	conn = conn_new();
	if (!conn) {
		DEBUG(0,("Couldn't find free connection.\n"));
		*status = NT_STATUS_INSUFFICIENT_RESOURCES;
		return NULL;
	}

	conn->nt_user_token = NULL;

	if (lp_guest_only(snum)) {
		const char *guestname = lp_guestaccount();
		NTSTATUS status2;
		char *found_username;
		guest = True;
		pass = getpwnam_alloc(NULL, guestname);
		if (!pass) {
			DEBUG(0,("make_connection_snum: Invalid guest "
				 "account %s??\n",guestname));
			conn_free(conn);
			*status = NT_STATUS_NO_SUCH_USER;
			return NULL;
		}
		status2 = create_token_from_username(NULL, pass->pw_name, True,
						     &conn->uid, &conn->gid,
						     &found_username,
						     &conn->nt_user_token);
		if (!NT_STATUS_IS_OK(status2)) {
			conn_free(conn);
			*status = status2;
			return NULL;
		}
		fstrcpy(user, found_username);
		string_set(&conn->user,user);
		conn->force_user = True;
		TALLOC_FREE(pass);
		DEBUG(3,("Guest only user %s\n",user));
	} else if (vuser) {
		if (vuser->guest) {
			if (!lp_guest_ok(snum)) {
				DEBUG(2, ("guest user (from session setup) "
					  "not permitted to access this share "
					  "(%s)\n", lp_servicename(snum)));
				      conn_free(conn);
				      *status = NT_STATUS_ACCESS_DENIED;
				      return NULL;
			}
		} else {
			if (!user_ok_token(vuser->user.unix_name,
					   vuser->nt_user_token, snum)) {
				DEBUG(2, ("user '%s' (from session setup) not "
					  "permitted to access this share "
					  "(%s)\n", vuser->user.unix_name,
					  lp_servicename(snum)));
				conn_free(conn);
				*status = NT_STATUS_ACCESS_DENIED;
				return NULL;
			}
		}
		conn->vuid = vuser->vuid;
		conn->uid = vuser->uid;
		conn->gid = vuser->gid;
		string_set(&conn->user,vuser->user.unix_name);
		fstrcpy(user,vuser->user.unix_name);
		guest = vuser->guest; 
	} else if (lp_security() == SEC_SHARE) {
		NTSTATUS status2;
		char *found_username;
		/* add it as a possible user name if we 
		   are in share mode security */
		add_session_user(lp_servicename(snum));
		/* shall we let them in? */
		if (!authorise_login(snum,user,password,&guest)) {
			DEBUG( 2, ( "Invalid username/password for [%s]\n", 
				    lp_servicename(snum)) );
			conn_free(conn);
			*status = NT_STATUS_WRONG_PASSWORD;
			return NULL;
		}
		pass = Get_Pwnam(user);
		status2 = create_token_from_username(NULL, pass->pw_name, True,
						     &conn->uid, &conn->gid,
						     &found_username,
						     &conn->nt_user_token);
		if (!NT_STATUS_IS_OK(status2)) {
			conn_free(conn);
			*status = status2;
			return NULL;
		}
		fstrcpy(user, found_username);
		string_set(&conn->user,user);
		conn->force_user = True;
	} else {
		DEBUG(0, ("invalid VUID (vuser) but not in security=share\n"));
		conn_free(conn);
		*status = NT_STATUS_ACCESS_DENIED;
		return NULL;
	}

	add_session_user(user);

	safe_strcpy(conn->client_address, client_addr(), 
		    sizeof(conn->client_address)-1);
	conn->num_files_open = 0;
	conn->lastused = conn->lastused_count = time(NULL);
	conn->service = snum;
	conn->used = True;
	conn->printer = (strncmp(dev,"LPT",3) == 0);
	conn->ipc = ( (strncmp(dev,"IPC",3) == 0) ||
		      ( lp_enable_asu_support() && strequal(dev,"ADMIN$")) );
	conn->dirptr = NULL;

	/* Case options for the share. */
	if (lp_casesensitive(snum) == Auto) {
		/* We will be setting this per packet. Set to be case
		 * insensitive for now. */
		conn->case_sensitive = False;
	} else {
		conn->case_sensitive = (BOOL)lp_casesensitive(snum);
	}

	conn->case_preserve = lp_preservecase(snum);
	conn->short_case_preserve = lp_shortpreservecase(snum);

	conn->veto_list = NULL;
	conn->hide_list = NULL;
	conn->veto_oplock_list = NULL;
	conn->aio_write_behind_list = NULL;
	string_set(&conn->dirpath,"");
	string_set(&conn->user,user);

	conn->read_only = lp_readonly(conn->service);
	conn->admin_user = False;

	/*
	 * If force user is true, then store the given userid and the gid of
	 * the user we're forcing.
	 * For auxiliary groups see below.
	 */
	
	if (*lp_force_user(snum)) {
		NTSTATUS status2;

		status2 = find_forced_user(snum,
					   (vuser != NULL) && vuser->guest,
					   &conn->uid, &conn->gid, user,
					   &conn->nt_user_token);
		if (!NT_STATUS_IS_OK(status2)) {
			conn_free(conn);
			*status = status2;
			return NULL;
		}
		string_set(&conn->user,user);
		conn->force_user = True;
		DEBUG(3,("Forced user %s\n",user));	  
	}

	/*
	 * If force group is true, then override
	 * any groupid stored for the connecting user.
	 */
	
	if (*lp_force_group(snum)) {
		NTSTATUS status2;
		DOM_SID group_sid;

		status2 = find_forced_group(conn->force_user,
					    snum, user,
					    &group_sid, &conn->gid);
		if (!NT_STATUS_IS_OK(status2)) {
			conn_free(conn);
			*status = status2;
			return NULL;
		}

		if ((conn->nt_user_token == NULL) && (vuser != NULL)) {

			/* Not force user and not security=share, but force
			 * group. vuser has a token to copy */
			
			conn->nt_user_token = dup_nt_token(
				NULL, vuser->nt_user_token);
			if (conn->nt_user_token == NULL) {
				DEBUG(0, ("dup_nt_token failed\n"));
				conn_free(conn);
				*status = NT_STATUS_NO_MEMORY;
				return NULL;
			}
		}

		/* If conn->nt_user_token is still NULL, we have
		 * security=share. This means ignore the SID, as we had no
		 * vuser to copy from */

		if (conn->nt_user_token != NULL) {
			/* Overwrite the primary group sid */
			sid_copy(&conn->nt_user_token->user_sids[1],
				 &group_sid);

		}
		conn->force_group = True;
	}

	if (conn->nt_user_token != NULL) {
		size_t i;

		/* We have a share-specific token from force [user|group].
		 * This means we have to create the list of unix groups from
		 * the list of sids. */

		conn->ngroups = 0;
		conn->groups = NULL;

		for (i=0; i<conn->nt_user_token->num_sids; i++) {
			gid_t gid;
			DOM_SID *sid = &conn->nt_user_token->user_sids[i];

			if (!sid_to_gid(sid, &gid)) {
				DEBUG(10, ("Could not convert SID %s to gid, "
					   "ignoring it\n",
					   sid_string_static(sid)));
				continue;
			}
			add_gid_to_array_unique(NULL, gid, &conn->groups,
						&conn->ngroups);
		}
	}

	{
		pstring s;
		pstrcpy(s,lp_pathname(snum));
		standard_sub_conn(conn,s,sizeof(s));
		set_conn_connectpath(conn,s);
		DEBUG(3,("Connect path is '%s' for service [%s]\n",s,
			 lp_servicename(snum)));
	}

	/*
	 * New code to check if there's a share security descripter
	 * added from NT server manager. This is done after the
	 * smb.conf checks are done as we need a uid and token. JRA.
	 *
	 */

	{
		BOOL can_write = share_access_check(conn, snum, vuser,
						    FILE_WRITE_DATA);

		if (!can_write) {
			if (!share_access_check(conn, snum, vuser,
						FILE_READ_DATA)) {
				/* No access, read or write. */
				DEBUG(0,("make_connection: connection to %s "
					 "denied due to security "
					 "descriptor.\n",
					  lp_servicename(snum)));
				conn_free(conn);
				*status = NT_STATUS_ACCESS_DENIED;
				return NULL;
			} else {
				conn->read_only = True;
			}
		}
	}
	/* Initialise VFS function pointers */

	if (!smbd_vfs_init(conn)) {
		DEBUG(0, ("vfs_init failed for service %s\n",
			  lp_servicename(snum)));
		conn_free(conn);
		*status = NT_STATUS_BAD_NETWORK_NAME;
		return NULL;
	}

	/*
	 * If widelinks are disallowed we need to canonicalise the connect
	 * path here to ensure we don't have any symlinks in the
	 * connectpath. We will be checking all paths on this connection are
	 * below this directory. We must do this after the VFS init as we
	 * depend on the realpath() pointer in the vfs table. JRA.
	 */
	if (!lp_widelinks(snum)) {
		pstring s;
		pstrcpy(s,conn->connectpath);
		canonicalize_path(conn, s);
		set_conn_connectpath(conn,s);
	}

/* ROOT Activities: */	
	/* check number of connections */
	if (!claim_connection(conn,
			      lp_servicename(snum),
			      lp_max_connections(snum),
			      False,0)) {
		DEBUG(1,("too many connections - rejected\n"));
		conn_free(conn);
		*status = NT_STATUS_INSUFFICIENT_RESOURCES;
		return NULL;
	}  

	/* Preexecs are done here as they might make the dir we are to ChDir
	 * to below */
	/* execute any "root preexec = " line */
	if (*lp_rootpreexec(snum)) {
		pstring cmd;
		pstrcpy(cmd,lp_rootpreexec(snum));
		standard_sub_conn(conn,cmd,sizeof(cmd));
		DEBUG(5,("cmd=%s\n",cmd));
		ret = smbrun(cmd,NULL);
		if (ret != 0 && lp_rootpreexec_close(snum)) {
			DEBUG(1,("root preexec gave %d - failing "
				 "connection\n", ret));
			yield_connection(conn, lp_servicename(snum));
			conn_free(conn);
			*status = NT_STATUS_ACCESS_DENIED;
			return NULL;
		}
	}

/* USER Activites: */
	if (!change_to_user(conn, conn->vuid)) {
		/* No point continuing if they fail the basic checks */
		DEBUG(0,("Can't become connected user!\n"));
		yield_connection(conn, lp_servicename(snum));
		conn_free(conn);
		*status = NT_STATUS_LOGON_FAILURE;
		return NULL;
	}

	/* Remember that a different vuid can connect later without these
	 * checks... */
	
	/* Preexecs are done here as they might make the dir we are to ChDir
	 * to below */

	/* execute any "preexec = " line */
	if (*lp_preexec(snum)) {
		pstring cmd;
		pstrcpy(cmd,lp_preexec(snum));
		standard_sub_conn(conn,cmd,sizeof(cmd));
		ret = smbrun(cmd,NULL);
		if (ret != 0 && lp_preexec_close(snum)) {
			DEBUG(1,("preexec gave %d - failing connection\n",
				 ret));
			change_to_root_user();
			yield_connection(conn, lp_servicename(snum));
			conn_free(conn);
			*status = NT_STATUS_ACCESS_DENIED;
			return NULL;
		}
	}

#ifdef WITH_FAKE_KASERVER
	if (lp_afs_share(snum)) {
		afs_login(conn);
	}
#endif
	
	/* Add veto/hide lists */
	if (!IS_IPC(conn) && !IS_PRINT(conn)) {
		set_namearray( &conn->veto_list, lp_veto_files(snum));
		set_namearray( &conn->hide_list, lp_hide_files(snum));
		set_namearray( &conn->veto_oplock_list, lp_veto_oplocks(snum));
	}
	
	/* Invoke VFS make connection hook - do this before the VFS_STAT call
	   to allow any filesystems needing user credentials to initialize
	   themselves. */

	if (SMB_VFS_CONNECT(conn, lp_servicename(snum), user) < 0) {
		DEBUG(0,("make_connection: VFS make connection failed!\n"));
		change_to_root_user();
		yield_connection(conn, lp_servicename(snum));
		conn_free(conn);
		*status = NT_STATUS_UNSUCCESSFUL;
		return NULL;
	}

	/* win2000 does not check the permissions on the directory
	   during the tree connect, instead relying on permission
	   check during individual operations. To match this behaviour
	   I have disabled this chdir check (tridge) */
	/* the alternative is just to check the directory exists */
	if ((ret = SMB_VFS_STAT(conn, conn->connectpath, &st)) != 0 ||
	    !S_ISDIR(st.st_mode)) {
		if (ret == 0 && !S_ISDIR(st.st_mode)) {
			DEBUG(0,("'%s' is not a directory, when connecting to "
				 "[%s]\n", conn->connectpath,
				 lp_servicename(snum)));
		} else {
			DEBUG(0,("'%s' does not exist or permission denied "
				 "when connecting to [%s] Error was %s\n",
				 conn->connectpath, lp_servicename(snum),
				 strerror(errno) ));
		}
		change_to_root_user();
		/* Call VFS disconnect hook */    
		SMB_VFS_DISCONNECT(conn);
		yield_connection(conn, lp_servicename(snum));
		conn_free(conn);
		*status = NT_STATUS_BAD_NETWORK_NAME;
		return NULL;
	}
	
	string_set(&conn->origpath,conn->connectpath);
	
#if SOFTLINK_OPTIMISATION
	/* resolve any soft links early if possible */
	if (vfs_ChDir(conn,conn->connectpath) == 0) {
		pstring s;
		pstrcpy(s,conn->connectpath);
		vfs_GetWd(conn,s);
		set_conn_connectpath(conn,s);
		vfs_ChDir(conn,conn->connectpath);
	}
#endif
	
	/*
	 * Print out the 'connected as' stuff here as we need
	 * to know the effective uid and gid we will be using
	 * (at least initially).
	 */

	if( DEBUGLVL( IS_IPC(conn) ? 3 : 1 ) ) {
		dbgtext( "%s (%s) ", get_remote_machine_name(),
			 conn->client_address );
		dbgtext( "%s", srv_is_signing_active() ? "signed " : "");
		dbgtext( "connect to service %s ", lp_servicename(snum) );
		dbgtext( "initially as user %s ", user );
		dbgtext( "(uid=%d, gid=%d) ", (int)geteuid(), (int)getegid() );
		dbgtext( "(pid %d)\n", (int)sys_getpid() );
	}
	
	/* Setup the minimum value for a change notify wait time (seconds). */
	set_change_notify_timeout(lp_change_notify_timeout(snum));

	/* we've finished with the user stuff - go back to root */
	change_to_root_user();
	return(conn);
}

/***************************************************************************************
 Simple wrapper function for make_connection() to include a call to 
 vfs_chdir()
 **************************************************************************************/
 
connection_struct *make_connection_with_chdir(const char *service_in,
					      DATA_BLOB password, 
					      const char *dev, uint16 vuid,
					      NTSTATUS *status)
{
	connection_struct *conn = NULL;
	
	conn = make_connection(service_in, password, dev, vuid, status);
	
	/*
	 * make_connection() does not change the directory for us any more
	 * so we have to do it as a separate step  --jerry
	 */
	 
	if ( conn && vfs_ChDir(conn,conn->connectpath) != 0 ) {
		DEBUG(0,("move_driver_to_download_area: Can't change "
			 "directory to %s for [print$] (%s)\n",
			 conn->connectpath,strerror(errno)));
		yield_connection(conn, lp_servicename(SNUM(conn)));
		conn_free(conn);
		*status = NT_STATUS_UNSUCCESSFUL;
		return NULL;
	}
	
	return conn;
}

/****************************************************************************
 Make a connection to a service.
 *
 * @param service 
****************************************************************************/

connection_struct *make_connection(const char *service_in, DATA_BLOB password, 
				   const char *pdev, uint16 vuid,
				   NTSTATUS *status)
{
	uid_t euid;
	user_struct *vuser = NULL;
	fstring service;
	fstring dev;
	int snum = -1;

	fstrcpy(dev, pdev);

	/* This must ONLY BE CALLED AS ROOT. As it exits this function as
	 * root. */
	if (!non_root_mode() && (euid = geteuid()) != 0) {
		DEBUG(0,("make_connection: PANIC ERROR. Called as nonroot "
			 "(%u)\n", (unsigned int)euid ));
		smb_panic("make_connection: PANIC ERROR. Called as nonroot\n");
	}

	if (conn_num_open() > 2047) {
		*status = NT_STATUS_INSUFF_SERVER_RESOURCES;
		return NULL;
	}

	if(lp_security() != SEC_SHARE) {
		vuser = get_valid_user_struct(vuid);
		if (!vuser) {
			DEBUG(1,("make_connection: refusing to connect with "
				 "no session setup\n"));
			*status = NT_STATUS_ACCESS_DENIED;
			return NULL;
		}
	}

	/* Logic to try and connect to the correct [homes] share, preferably
	   without too many getpwnam() lookups.  This is particulary nasty for
	   winbind usernames, where the share name isn't the same as unix
	   username.

	   The snum of the homes share is stored on the vuser at session setup
	   time.
	*/

	if (strequal(service_in,HOMES_NAME)) {
		if(lp_security() != SEC_SHARE) {
			DATA_BLOB no_pw = data_blob(NULL, 0);
			if (vuser->homes_snum == -1) {
				DEBUG(2, ("[homes] share not available for "
					  "this user because it was not found "
					  "or created at session setup "
					  "time\n"));
				*status = NT_STATUS_BAD_NETWORK_NAME;
				return NULL;
			}
			DEBUG(5, ("making a connection to [homes] service "
				  "created at session setup time\n"));
			return make_connection_snum(vuser->homes_snum,
						    vuser, no_pw, 
						    dev, status);
		} else {
			/* Security = share. Try with
			 * current_user_info.smb_name as the username.  */
			if (*current_user_info.smb_name) {
				fstring unix_username;
				fstrcpy(unix_username,
					current_user_info.smb_name);
				map_username(unix_username);
				snum = find_service(unix_username);
			} 
			if (snum != -1) {
				DEBUG(5, ("making a connection to 'homes' "
					  "service %s based on "
					  "security=share\n", service_in));
				return make_connection_snum(snum, NULL,
							    password,
							    dev, status);
			}
		}
	} else if ((lp_security() != SEC_SHARE) && (vuser->homes_snum != -1)
		   && strequal(service_in,
			       lp_servicename(vuser->homes_snum))) {
		DATA_BLOB no_pw = data_blob(NULL, 0);
		DEBUG(5, ("making a connection to 'homes' service [%s] "
			  "created at session setup time\n", service_in));
		return make_connection_snum(vuser->homes_snum,
					    vuser, no_pw, 
					    dev, status);
	}
	
	fstrcpy(service, service_in);

	strlower_m(service);

	snum = find_service(service);

	if (snum < 0) {
		if (strequal(service,"IPC$") ||
		    (lp_enable_asu_support() && strequal(service,"ADMIN$"))) {
			DEBUG(3,("refusing IPC connection to %s\n", service));
			*status = NT_STATUS_ACCESS_DENIED;
			return NULL;
		}

		DEBUG(0,("%s (%s) couldn't find service %s\n",
			 get_remote_machine_name(), client_addr(), service));
		*status = NT_STATUS_BAD_NETWORK_NAME;
		return NULL;
	}

	/* Handle non-Dfs clients attempting connections to msdfs proxy */
	if (lp_host_msdfs() && (*lp_msdfs_proxy(snum) != '\0'))  {
		DEBUG(3, ("refusing connection to dfs proxy share '%s' "
			  "(pointing to %s)\n", 
			service, lp_msdfs_proxy(snum)));
		*status = NT_STATUS_BAD_NETWORK_NAME;
		return NULL;
	}

	DEBUG(5, ("making a connection to 'normal' service %s\n", service));

	return make_connection_snum(snum, vuser,
				    password,
				    dev, status);
}

/****************************************************************************
 Close a cnum.
****************************************************************************/

void close_cnum(connection_struct *conn, uint16 vuid)
{
	if (IS_IPC(conn)) {
		pipe_close_conn(conn);
	} else {
		file_close_conn(conn);
		dptr_closecnum(conn);
	}

	change_to_root_user();

	DEBUG(IS_IPC(conn)?3:1, ("%s (%s) closed connection to service %s\n",
				 get_remote_machine_name(),
				 conn->client_address,
				 lp_servicename(SNUM(conn))));

	/* Call VFS disconnect hook */    
	SMB_VFS_DISCONNECT(conn);

	yield_connection(conn, lp_servicename(SNUM(conn)));

	/* make sure we leave the directory available for unmount */
	vfs_ChDir(conn, "/");

	/* execute any "postexec = " line */
	if (*lp_postexec(SNUM(conn)) && 
	    change_to_user(conn, vuid))  {
		pstring cmd;
		pstrcpy(cmd,lp_postexec(SNUM(conn)));
		standard_sub_conn(conn,cmd,sizeof(cmd));
		smbrun(cmd,NULL);
		change_to_root_user();
	}

	change_to_root_user();
	/* execute any "root postexec = " line */
	if (*lp_rootpostexec(SNUM(conn)))  {
		pstring cmd;
		pstrcpy(cmd,lp_rootpostexec(SNUM(conn)));
		standard_sub_conn(conn,cmd,sizeof(cmd));
		smbrun(cmd,NULL);
	}

	conn_free(conn);
}
