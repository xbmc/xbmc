/* 
   Unix SMB/CIFS implementation.
   Password and authentication handling
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

/* users from session setup */
static char *session_userlist = NULL;
static int len_session_userlist = 0;

/* this holds info on user ids that are already validated for this VC */
static user_struct *validated_users;
static int next_vuid = VUID_OFFSET;
static int num_validated_vuids;

extern userdom_struct current_user_info;


/****************************************************************************
 Check if a uid has been validated, and return an pointer to the user_struct
 if it has. NULL if not. vuid is biased by an offset. This allows us to
 tell random client vuid's (normally zero) from valid vuids.
****************************************************************************/

user_struct *get_valid_user_struct(uint16 vuid)
{
	user_struct *usp;
	int count=0;

	if (vuid == UID_FIELD_INVALID)
		return NULL;

	for (usp=validated_users;usp;usp=usp->next,count++) {
		if (vuid == usp->vuid && usp->server_info) {
			if (count > 10) {
				DLIST_PROMOTE(validated_users, usp);
			}
			return usp;
		}
	}

	return NULL;
}

/****************************************************************************
 Get the user struct of a partial NTLMSSP login
****************************************************************************/

user_struct *get_partial_auth_user_struct(uint16 vuid)
{
	user_struct *usp;
	int count=0;

	if (vuid == UID_FIELD_INVALID)
		return NULL;

	for (usp=validated_users;usp;usp=usp->next,count++) {
		if (vuid == usp->vuid && !usp->server_info) {
			if (count > 10) {
				DLIST_PROMOTE(validated_users, usp);
			}
			return usp;
		}
	}

	return NULL;
}

/****************************************************************************
 Invalidate a uid.
****************************************************************************/

void invalidate_vuid(uint16 vuid)
{
	user_struct *vuser = get_valid_user_struct(vuid);

	if (vuser == NULL)
		return;
	
	SAFE_FREE(vuser->homedir);
	SAFE_FREE(vuser->unix_homedir);
	SAFE_FREE(vuser->logon_script);
	
	session_yield(vuser);
	SAFE_FREE(vuser->session_keystr);

	TALLOC_FREE(vuser->server_info);

	data_blob_free(&vuser->session_key);

	DLIST_REMOVE(validated_users, vuser);

	/* clear the vuid from the 'cache' on each connection, and
	   from the vuid 'owner' of connections */
	conn_clear_vuid_cache(vuid);

	SAFE_FREE(vuser->groups);
	TALLOC_FREE(vuser->nt_user_token);
	SAFE_FREE(vuser);
	num_validated_vuids--;
}

/****************************************************************************
 Invalidate all vuid entries for this process.
****************************************************************************/

void invalidate_all_vuids(void)
{
	user_struct *usp, *next=NULL;

	for (usp=validated_users;usp;usp=next) {
		next = usp->next;
		
		invalidate_vuid(usp->vuid);
	}
}

/**
 *  register that a valid login has been performed, establish 'session'.
 *  @param server_info The token returned from the authentication process. 
 *   (now 'owned' by register_vuid)
 *
 *  @param session_key The User session key for the login session (now also
 *  'owned' by register_vuid)
 *
 *  @param respose_blob The NT challenge-response, if available.  (May be
 *  freed after this call)
 *
 *  @param smb_name The untranslated name of the user
 *
 *  @return Newly allocated vuid, biased by an offset. (This allows us to
 *   tell random client vuid's (normally zero) from valid vuids.)
 *
 */

int register_vuid(auth_serversupplied_info *server_info,
		  DATA_BLOB session_key, DATA_BLOB response_blob,
		  const char *smb_name)
{
	user_struct *vuser = NULL;

	/* Paranoia check. */
	if(lp_security() == SEC_SHARE) {
		smb_panic("Tried to register uid in security=share\n");
	}

	/* Limit allowed vuids to 16bits - VUID_OFFSET. */
	if (num_validated_vuids >= 0xFFFF-VUID_OFFSET) {
		data_blob_free(&session_key);
		return UID_FIELD_INVALID;
	}

	if((vuser = SMB_MALLOC_P(user_struct)) == NULL) {
		DEBUG(0,("Failed to malloc users struct!\n"));
		data_blob_free(&session_key);
		return UID_FIELD_INVALID;
	}

	ZERO_STRUCTP(vuser);

	/* Allocate a free vuid. Yes this is a linear search... :-) */
	while( get_valid_user_struct(next_vuid) != NULL ) {
		next_vuid++;
		/* Check for vuid wrap. */
		if (next_vuid == UID_FIELD_INVALID)
			next_vuid = VUID_OFFSET;
	}

	DEBUG(10,("register_vuid: allocated vuid = %u\n",
		  (unsigned int)next_vuid ));

	vuser->vuid = next_vuid;

	if (!server_info) {
		/*
		 * This happens in an unfinished NTLMSSP session setup. We
		 * need to allocate a vuid between the first and second calls
		 * to NTLMSSP.
		 */
		next_vuid++;
		num_validated_vuids++;
		
		vuser->server_info = NULL;
		
		DLIST_ADD(validated_users, vuser);
		
		return vuser->vuid;
	}

	/* the next functions should be done by a SID mapping system (SMS) as
	 * the new real sam db won't have reference to unix uids or gids
	 */
	
	vuser->uid = server_info->uid;
	vuser->gid = server_info->gid;
	
	vuser->n_groups = server_info->n_groups;
	if (vuser->n_groups) {
		if (!(vuser->groups = (gid_t *)memdup(server_info->groups,
						      sizeof(gid_t) *
						      vuser->n_groups))) {
			DEBUG(0,("register_vuid: failed to memdup "
				 "vuser->groups\n"));
			data_blob_free(&session_key);
			free(vuser);
			TALLOC_FREE(server_info);
			return UID_FIELD_INVALID;
		}
	}

	vuser->guest = server_info->guest;
	fstrcpy(vuser->user.unix_name, server_info->unix_name); 

	/* This is a potentially untrusted username */
	alpha_strcpy(vuser->user.smb_name, smb_name, ". _-$",
		     sizeof(vuser->user.smb_name));

	fstrcpy(vuser->user.domain, pdb_get_domain(server_info->sam_account));
	fstrcpy(vuser->user.full_name,
		pdb_get_fullname(server_info->sam_account));

	{
		/* Keep the homedir handy */
		const char *homedir =
			pdb_get_homedir(server_info->sam_account);
		const char *logon_script =
			pdb_get_logon_script(server_info->sam_account);

		if (!IS_SAM_DEFAULT(server_info->sam_account,
				    PDB_UNIXHOMEDIR)) {
			const char *unix_homedir =
				pdb_get_unix_homedir(server_info->sam_account);
			if (unix_homedir) {
				vuser->unix_homedir =
					smb_xstrdup(unix_homedir);
			}
		} else {
			struct passwd *passwd =
				getpwnam_alloc(NULL, vuser->user.unix_name);
			if (passwd) {
				vuser->unix_homedir =
					smb_xstrdup(passwd->pw_dir);
				TALLOC_FREE(passwd);
			}
		}
		
		if (homedir) {
			vuser->homedir = smb_xstrdup(homedir);
		}
		if (logon_script) {
			vuser->logon_script = smb_xstrdup(logon_script);
		}
	}

	vuser->session_key = session_key;

	DEBUG(10,("register_vuid: (%u,%u) %s %s %s guest=%d\n", 
		  (unsigned int)vuser->uid, 
		  (unsigned int)vuser->gid,
		  vuser->user.unix_name, vuser->user.smb_name,
		  vuser->user.domain, vuser->guest ));

	DEBUG(3, ("User name: %s\tReal name: %s\n", vuser->user.unix_name,
		  vuser->user.full_name));	

 	if (server_info->ptok) {
		vuser->nt_user_token = dup_nt_token(NULL, server_info->ptok);
	} else {
		DEBUG(1, ("server_info does not contain a user_token - "
			  "cannot continue\n"));
		TALLOC_FREE(server_info);
		data_blob_free(&session_key);
		SAFE_FREE(vuser->homedir);
		SAFE_FREE(vuser->unix_homedir);
		SAFE_FREE(vuser->logon_script);

		SAFE_FREE(vuser);
		return UID_FIELD_INVALID;
	}

	/* use this to keep tabs on all our info from the authentication */
	vuser->server_info = server_info;

	DEBUG(3,("UNIX uid %d is UNIX user %s, and will be vuid %u\n",
		 (int)vuser->uid,vuser->user.unix_name, vuser->vuid));

	next_vuid++;
	num_validated_vuids++;

	DLIST_ADD(validated_users, vuser);

	if (!session_claim(vuser)) {
		DEBUG(1, ("Failed to claim session for vuid=%d\n",
			  vuser->vuid));
		invalidate_vuid(vuser->vuid);
		return UID_FIELD_INVALID;
	}

	/* Register a home dir service for this user iff
	
	   (a) This is not a guest connection,
	   (b) we have a home directory defined 
	   (c) there s not an existing static share by that name
	   
	   If a share exists by this name (autoloaded or not) reuse it . */

	vuser->homes_snum = -1;

	if ( (!vuser->guest) && vuser->unix_homedir && *(vuser->unix_homedir)) 
	{
		int servicenumber = lp_servicenumber(vuser->user.unix_name);

		if ( servicenumber == -1 ) {
			DEBUG(3, ("Adding homes service for user '%s' using "
				  "home directory: '%s'\n", 
				vuser->user.unix_name, vuser->unix_homedir));
			vuser->homes_snum =
				add_home_service(vuser->user.unix_name, 
						 vuser->user.unix_name,
						 vuser->unix_homedir);
		} else {
			DEBUG(3, ("Using static (or previously created) "
				  "service for user '%s'; path = '%s'\n", 
				  vuser->user.unix_name,
				  lp_pathname(servicenumber) ));
			vuser->homes_snum = servicenumber;
		}
	} 
	
	if (srv_is_signing_negotiated() && !vuser->guest &&
	    !srv_signing_started()) {
		/* Try and turn on server signing on the first non-guest
		 * sessionsetup. */
		srv_set_signing(vuser->session_key, response_blob);
	}
	
	/* fill in the current_user_info struct */
	set_current_user_info( &vuser->user );


	return vuser->vuid;
}

/****************************************************************************
 Add a name to the session users list.
****************************************************************************/

void add_session_user(const char *user)
{
	fstring suser;
	struct passwd *passwd;

	if (!(passwd = Get_Pwnam(user)))
		return;

	fstrcpy(suser,passwd->pw_name);

	if(!*suser)
		return;

	if( session_userlist && in_list(suser,session_userlist,False) )
		return;

	if( !session_userlist ||
	    (strlen(suser) + strlen(session_userlist) + 2 >=
	     len_session_userlist) ) {
		char *newlist;

		if (len_session_userlist > 128 * PSTRING_LEN) {
			DEBUG(3,("add_session_user: session userlist already "
				 "too large.\n"));
			return;
		}
		newlist = (char *)SMB_REALLOC_KEEP_OLD_ON_ERROR(
			session_userlist,
			len_session_userlist + PSTRING_LEN );
		if( newlist == NULL ) {
			DEBUG(1,("Unable to resize session_userlist\n"));
			return;
		}
		if (!session_userlist) {
			*newlist = '\0';
		}
		session_userlist = newlist;
		len_session_userlist += PSTRING_LEN;
	}

	safe_strcat(session_userlist," ",len_session_userlist-1);
	safe_strcat(session_userlist,suser,len_session_userlist-1);
}

/****************************************************************************
 Check if a user is in a netgroup user list. If at first we don't succeed,
 try lower case.
****************************************************************************/

BOOL user_in_netgroup(const char *user, const char *ngname)
{
#ifdef HAVE_NETGROUP
	static char *mydomain = NULL;
	fstring lowercase_user;

	if (mydomain == NULL)
		yp_get_default_domain(&mydomain);

	if(mydomain == NULL) {
		DEBUG(5,("Unable to get default yp domain, let's try without specifying it\n"));
	}

	DEBUG(5,("looking for user %s of domain %s in netgroup %s\n",
		user, mydomain?mydomain:"(ANY)", ngname));

	if (innetgr(ngname, NULL, user, mydomain)) {
		DEBUG(5,("user_in_netgroup: Found\n"));
		return (True);
	} else {

		/*
		 * Ok, innetgr is case sensitive. Try once more with lowercase
		 * just in case. Attempt to fix #703. JRA.
		 */

		fstrcpy(lowercase_user, user);
		strlower_m(lowercase_user);
	
		DEBUG(5,("looking for user %s of domain %s in netgroup %s\n",
			lowercase_user, mydomain?mydomain:"(ANY)", ngname));

		if (innetgr(ngname, NULL, lowercase_user, mydomain)) {
			DEBUG(5,("user_in_netgroup: Found\n"));
			return (True);
		}
	}
#endif /* HAVE_NETGROUP */
	return False;
}

/****************************************************************************
 Check if a user is in a user list - can check combinations of UNIX
 and netgroup lists.
****************************************************************************/

BOOL user_in_list(const char *user,const char **list)
{
	if (!list || !*list)
		return False;

	DEBUG(10,("user_in_list: checking user %s in list\n", user));

	while (*list) {

		DEBUG(10,("user_in_list: checking user |%s| against |%s|\n",
			  user, *list));

		/*
		 * Check raw username.
		 */
		if (strequal(user, *list))
			return(True);

		/*
		 * Now check to see if any combination
		 * of UNIX and netgroups has been specified.
		 */

		if(**list == '@') {
			/*
			 * Old behaviour. Check netgroup list
			 * followed by UNIX list.
			 */
			if(user_in_netgroup(user, *list +1))
				return True;
			if(user_in_group(user, *list +1))
				return True;
		} else if (**list == '+') {

			if((*(*list +1)) == '&') {
				/*
				 * Search UNIX list followed by netgroup.
				 */
				if(user_in_group(user, *list +2))
					return True;
				if(user_in_netgroup(user, *list +2))
					return True;

			} else {

				/*
				 * Just search UNIX list.
				 */

				if(user_in_group(user, *list +1))
					return True;
			}

		} else if (**list == '&') {

			if(*(*list +1) == '+') {
				/*
				 * Search netgroup list followed by UNIX list.
				 */
				if(user_in_netgroup(user, *list +2))
					return True;
				if(user_in_group(user, *list +2))
					return True;
			} else {
				/*
				 * Just search netgroup list.
				 */
				if(user_in_netgroup(user, *list +1))
					return True;
			}
		}
    
		list++;
	}
	return(False);
}

/****************************************************************************
 Check if a username is valid.
****************************************************************************/

static BOOL user_ok(const char *user, int snum)
{
	char **valid, **invalid;
	BOOL ret;

	valid = invalid = NULL;
	ret = True;

	if (lp_invalid_users(snum)) {
		str_list_copy(&invalid, lp_invalid_users(snum));
		if (invalid &&
		    str_list_substitute(invalid, "%S", lp_servicename(snum))) {
			if ( invalid &&
			     str_list_sub_basic(invalid,
						current_user_info.smb_name) ) {
				ret = !user_in_list(user,
						    (const char **)invalid);
			}
		}
	}
	if (invalid)
		str_list_free (&invalid);

	if (ret && lp_valid_users(snum)) {
		str_list_copy(&valid, lp_valid_users(snum));
		if ( valid &&
		     str_list_substitute(valid, "%S", lp_servicename(snum)) ) {
			if ( valid &&
			     str_list_sub_basic(valid,
						current_user_info.smb_name) ) {
				ret = user_in_list(user, (const char **)valid);
			}
		}
	}
	if (valid)
		str_list_free (&valid);

	if (ret && lp_onlyuser(snum)) {
		char **user_list = str_list_make (lp_username(snum), NULL);
		if (user_list &&
		    str_list_substitute(user_list, "%S",
					lp_servicename(snum))) {
			ret = user_in_list(user, (const char **)user_list);
		}
		if (user_list) str_list_free (&user_list);
	}

	return(ret);
}

/****************************************************************************
 Validate a group username entry. Return the username or NULL.
****************************************************************************/

static char *validate_group(char *group, DATA_BLOB password,int snum)
{
#ifdef HAVE_NETGROUP
	{
		char *host, *user, *domain;
		setnetgrent(group);
		while (getnetgrent(&host, &user, &domain)) {
			if (user) {
				if (user_ok(user, snum) && 
				    password_ok(user,password)) {
					endnetgrent();
					return(user);
				}
			}
		}
		endnetgrent();
	}
#endif
  
#ifdef HAVE_GETGRENT
	{
		struct group *gptr;
		setgrent();
		while ((gptr = (struct group *)getgrent())) {
			if (strequal(gptr->gr_name,group))
				break;
		}

		/*
		 * As user_ok can recurse doing a getgrent(), we must
		 * copy the member list into a pstring on the stack before
		 * use. Bug pointed out by leon@eatworms.swmed.edu.
		 */

		if (gptr) {
			pstring member_list;
			char *member;
			size_t copied_len = 0;
			int i;

			*member_list = '\0';
			member = member_list;

			for(i = 0; gptr->gr_mem && gptr->gr_mem[i]; i++) {
				size_t member_len = strlen(gptr->gr_mem[i])+1;
				if(copied_len+member_len < sizeof(pstring)) { 

					DEBUG(10,("validate_group: = gr_mem = "
						  "%s\n", gptr->gr_mem[i]));

					safe_strcpy(member, gptr->gr_mem[i],
						    sizeof(pstring) -
						    copied_len - 1);
					copied_len += member_len;
					member += copied_len;
				} else {
					*member = '\0';
				}
			}

			endgrent();

			member = member_list;
			while (*member) {
				static fstring name;
				fstrcpy(name,member);
				if (user_ok(name,snum) &&
				    password_ok(name,password)) {
					endgrent();
					return(&name[0]);
				}

				DEBUG(10,("validate_group = member = %s\n",
					  member));

				member += strlen(member) + 1;
			}
		} else {
			endgrent();
			return NULL;
		}
	}
#endif
	return(NULL);
}

/****************************************************************************
 Check for authority to login to a service with a given username/password.
 Note this is *NOT* used when logging on using sessionsetup_and_X.
****************************************************************************/

BOOL authorise_login(int snum, fstring user, DATA_BLOB password, 
		     BOOL *guest)
{
	BOOL ok = False;
	
#ifdef DEBUG_PASSWORD
	DEBUG(100,("authorise_login: checking authorisation on "
		   "user=%s pass=%s\n", user,password.data));
#endif

	*guest = False;
  
	/* there are several possibilities:
		1) login as the given user with given password
		2) login as a previously registered username with the given 
		   password
		3) login as a session list username with the given password
		4) login as a previously validated user/password pair
		5) login as the "user =" user with given password
		6) login as the "user =" user with no password 
		   (guest connection)
		7) login as guest user with no password

		if the service is guest_only then steps 1 to 5 are skipped
	*/

	/* now check the list of session users */
	if (!ok) {
		char *auser;
		char *user_list = NULL;

		if ( session_userlist )
			user_list = SMB_STRDUP(session_userlist);
		else
			user_list = SMB_STRDUP("");

		if (!user_list)
			return(False);
		
		for (auser=strtok(user_list,LIST_SEP); !ok && auser;
		     auser = strtok(NULL,LIST_SEP)) {
			fstring user2;
			fstrcpy(user2,auser);
			if (!user_ok(user2,snum))
				continue;
			
			if (password_ok(user2,password)) {
				ok = True;
				fstrcpy(user,user2);
				DEBUG(3,("authorise_login: ACCEPTED: session "
					 "list username (%s) and given "
					 "password ok\n", user));
			}
		}

		SAFE_FREE(user_list);
	}
	
	/* check the user= fields and the given password */
	if (!ok && lp_username(snum)) {
		char *auser;
		pstring user_list;
		pstrcpy(user_list,lp_username(snum));
		
		pstring_sub(user_list,"%S",lp_servicename(snum));
		
		for (auser=strtok(user_list,LIST_SEP); auser && !ok;
		     auser = strtok(NULL,LIST_SEP)) {
			if (*auser == '@') {
				auser = validate_group(auser+1,password,snum);
				if (auser) {
					ok = True;
					fstrcpy(user,auser);
					DEBUG(3,("authorise_login: ACCEPTED: "
						 "group username and given "
						 "password ok (%s)\n", user));
				}
			} else {
				fstring user2;
				fstrcpy(user2,auser);
				if (user_ok(user2,snum) &&
				    password_ok(user2,password)) {
					ok = True;
					fstrcpy(user,user2);
					DEBUG(3,("authorise_login: ACCEPTED: "
						 "user list username and "
						 "given password ok (%s)\n",
						 user));
				}
			}
		}
	}

	/* check for a normal guest connection */
	if (!ok && GUEST_OK(snum)) {
		fstring guestname;
		fstrcpy(guestname,lp_guestaccount());
		if (Get_Pwnam(guestname)) {
			fstrcpy(user,guestname);
			ok = True;
			DEBUG(3,("authorise_login: ACCEPTED: guest account "
				 "and guest ok (%s)\n",	user));
		} else {
			DEBUG(0,("authorise_login: Invalid guest account "
				 "%s??\n",guestname));
		}
		*guest = True;
	}

	if (ok && !user_ok(user, snum)) {
		DEBUG(0,("authorise_login: rejected invalid user %s\n",user));
		ok = False;
	}

	return(ok);
}
