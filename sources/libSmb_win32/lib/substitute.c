/* 
   Unix SMB/CIFS implementation.
   string substitution functions
   Copyright (C) Andrew Tridgell 1992-2000
   Copyright (C) Gerald Carter   2006
   
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

extern struct current_user current_user;

fstring local_machine="";
fstring remote_arch="UNKNOWN";
userdom_struct current_user_info;
fstring remote_proto="UNKNOWN";

static fstring remote_machine;
static fstring smb_user_name;

/** 
 * Set the 'local' machine name
 * @param local_name the name we are being called
 * @param if this is the 'final' name for us, not be be changed again
 */

void set_local_machine_name(const char* local_name, BOOL perm)
{
	static BOOL already_perm = False;
	fstring tmp_local_machine;

	fstrcpy(tmp_local_machine,local_name);
	trim_char(tmp_local_machine,' ',' ');

	/*
	 * Windows NT/2k uses "*SMBSERVER" and XP uses "*SMBSERV"
	 * arrggg!!! 
	 */

	if ( strequal(tmp_local_machine, "*SMBSERVER") || strequal(tmp_local_machine, "*SMBSERV") )  {
		fstrcpy( local_machine, client_socket_addr() );
		return;
	}

	if (already_perm)
		return;

	already_perm = perm;

	alpha_strcpy(local_machine,tmp_local_machine,SAFE_NETBIOS_CHARS,sizeof(local_machine)-1);
	strlower_m(local_machine);
}

/** 
 * Set the 'remote' machine name
 * @param remote_name the name our client wants to be called by
 * @param if this is the 'final' name for them, not be be changed again
 */

void set_remote_machine_name(const char* remote_name, BOOL perm)
{
	static BOOL already_perm = False;
	fstring tmp_remote_machine;

	if (already_perm)
		return;

	already_perm = perm;

	fstrcpy(tmp_remote_machine,remote_name);
	trim_char(tmp_remote_machine,' ',' ');
	alpha_strcpy(remote_machine,tmp_remote_machine,SAFE_NETBIOS_CHARS,sizeof(remote_machine)-1);
	strlower_m(remote_machine);
}

const char* get_remote_machine_name(void) 
{
	return remote_machine;
}

const char* get_local_machine_name(void) 
{
	if (!*local_machine) {
		return global_myname();
	}

	return local_machine;
}

/*******************************************************************
 Setup the string used by %U substitution.
********************************************************************/

void sub_set_smb_name(const char *name)
{
	fstring tmp;
	int len;
	BOOL is_machine_account = False;

	/* don't let anonymous logins override the name */
	if (! *name)
		return;


	fstrcpy( tmp, name );
	trim_char( tmp, ' ', ' ' );
	strlower_m( tmp );

	len = strlen( tmp );

	if ( len == 0 )
		return;

	/* long story but here goes....we have to allow usernames
	   ending in '$' as they are valid machine account names.
	   So check for a machine account and re-add the '$'
	   at the end after the call to alpha_strcpy().   --jerry  */
	   
	if ( tmp[len-1] == '$' )
		is_machine_account = True;
	
	alpha_strcpy( smb_user_name, tmp, SAFE_NETBIOS_CHARS, sizeof(smb_user_name)-1 );

	if ( is_machine_account ) {
		len = strlen( smb_user_name );
		smb_user_name[len-1] = '$';
	}
}

char* sub_get_smb_name( void )
{
	return smb_user_name;
}

/*******************************************************************
 Setup the strings used by substitutions. Called per packet. Ensure
 %U name is set correctly also.
********************************************************************/

void set_current_user_info(const userdom_struct *pcui)
{
	current_user_info = *pcui;
	/* The following is safe as current_user_info.smb_name
	 * has already been sanitised in register_vuid. */
	fstrcpy(smb_user_name, current_user_info.smb_name);
}

/*******************************************************************
 return the current active user name
*******************************************************************/

const char* get_current_username( void )
{
	if ( current_user_info.smb_name[0] == '\0' )
		return smb_user_name;

	return current_user_info.smb_name; 
}

/*******************************************************************
 Given a pointer to a %$(NAME) in p and the whole string in str
 expand it as an environment variable.
 Return a new allocated and expanded string.
 Based on code by Branko Cibej <branko.cibej@hermes.si>
 When this is called p points at the '%' character.
 May substitute multiple occurrencies of the same env var.
********************************************************************/

static char * realloc_expand_env_var(char *str, char *p)
{
	char *envname;
	char *envval;
	char *q, *r;
	int copylen;

	if (p[0] != '%' || p[1] != '$' || p[2] != '(') {
		return str;
	}

	/*
	 * Look for the terminating ')'.
	 */

	if ((q = strchr_m(p,')')) == NULL) {
		DEBUG(0,("expand_env_var: Unterminated environment variable [%s]\n", p));
		return str;
	}

	/*
	 * Extract the name from within the %$(NAME) string.
	 */

	r = p + 3;
	copylen = q - r;
	
	/* reserve space for use later add %$() chars */
	if ( (envname = (char *)SMB_MALLOC(copylen + 1 + 4)) == NULL ) {
		return NULL;
	}
	
	strncpy(envname,r,copylen);
	envname[copylen] = '\0';

	if ((envval = getenv(envname)) == NULL) {
		DEBUG(0,("expand_env_var: Environment variable [%s] not set\n", envname));
		SAFE_FREE(envname);
		return str;
	}

	/*
	 * Copy the full %$(NAME) into envname so it
	 * can be replaced.
	 */

	copylen = q + 1 - p;
	strncpy(envname,p,copylen);
	envname[copylen] = '\0';
	r = realloc_string_sub(str, envname, envval);
	SAFE_FREE(envname);
		
	return r;
}

/*******************************************************************
*******************************************************************/

static char *longvar_domainsid( void )
{
	DOM_SID sid;
	char *sid_string;
	
	if ( !secrets_fetch_domain_sid( lp_workgroup(), &sid ) ) {
		return NULL;
	}
	
	sid_string = SMB_STRDUP( sid_string_static( &sid ) );
	
	if ( !sid_string ) {
		DEBUG(0,("longvar_domainsid: failed to dup SID string!\n"));
	}
	
	return sid_string;
}

/*******************************************************************
*******************************************************************/

struct api_longvar {
	const char *name;
	char* (*fn)( void );
};

struct api_longvar longvar_table[] = {
	{ "DomainSID",		longvar_domainsid },
	{ NULL, 		NULL }
};

static char *get_longvar_val( const char *varname )
{
	int i;
	
	DEBUG(7,("get_longvar_val: expanding variable [%s]\n", varname));
	
	for ( i=0; longvar_table[i].name; i++ ) {
		if ( strequal( longvar_table[i].name, varname ) ) {
			return longvar_table[i].fn();
		}
	}
	
	return NULL;
}

/*******************************************************************
 Expand the long smb.conf variable names given a pointer to a %(NAME).
 Return the number of characters by which the pointer should be advanced.
 When this is called p points at the '%' character.
********************************************************************/

static char *realloc_expand_longvar(char *str, char *p)
{
	fstring varname;
	char *value;
	char *q, *r;
	int copylen;

	if ( p[0] != '%' || p[1] != '(' ) {
		return str;
	}

	/* Look for the terminating ')'.*/

	if ((q = strchr_m(p,')')) == NULL) {
		DEBUG(0,("realloc_expand_longvar: Unterminated environment variable [%s]\n", p));
		return str;
	}

	/* Extract the name from within the %(NAME) string.*/

	r = p+2;
	copylen = MIN( (q-r), (sizeof(varname)-1) );
	strncpy(varname, r, copylen);
	varname[copylen] = '\0';

	if ((value = get_longvar_val(varname)) == NULL) {
		DEBUG(0,("realloc_expand_longvar: Variable [%s] not set.  Skipping\n", varname));
		return str;
	}

	/* Copy the full %(NAME) into envname so it can be replaced.*/

	copylen = MIN( (q+1-p),(sizeof(varname)-1) );
	strncpy( varname, p, copylen );
	varname[copylen] = '\0';
	r = realloc_string_sub(str, varname, value);
	SAFE_FREE( value );
	
	/* skip over the %(varname) */
	
	return r;
}

/*******************************************************************
 Patch from jkf@soton.ac.uk
 Added this to implement %p (NIS auto-map version of %H)
*******************************************************************/

static char *automount_path(const char *user_name)
{
	static pstring server_path;

	/* use the passwd entry as the default */
	/* this will be the default if WITH_AUTOMOUNT is not used or fails */

	pstrcpy(server_path, get_user_home_dir(user_name));

#if (defined(HAVE_NETGROUP) && defined (WITH_AUTOMOUNT))

	if (lp_nis_home_map()) {
		const char *home_path_start;
		const char *automount_value = automount_lookup(user_name);

		if(strlen(automount_value) > 0) {
			home_path_start = strchr_m(automount_value,':');
			if (home_path_start != NULL) {
				DEBUG(5, ("NIS lookup succeeded.  Home path is: %s\n",
						home_path_start?(home_path_start+1):""));
				pstrcpy(server_path, home_path_start+1);
			}
		} else {
			/* NIS key lookup failed: default to user home directory from password file */
			DEBUG(5, ("NIS lookup failed. Using Home path from passwd file. Home path is: %s\n", server_path ));
		}
	}
#endif

	DEBUG(4,("Home server path: %s\n", server_path));

	return server_path;
}

/*******************************************************************
 Patch from jkf@soton.ac.uk
 This is Luke's original function with the NIS lookup code
 moved out to a separate function.
*******************************************************************/

static const char *automount_server(const char *user_name)
{
	static pstring server_name;
	const char *local_machine_name = get_local_machine_name(); 

	/* use the local machine name as the default */
	/* this will be the default if WITH_AUTOMOUNT is not used or fails */
	if (local_machine_name && *local_machine_name)
		pstrcpy(server_name, local_machine_name);
	else
		pstrcpy(server_name, global_myname());
	
#if (defined(HAVE_NETGROUP) && defined (WITH_AUTOMOUNT))

	if (lp_nis_home_map()) {
	        int home_server_len;
		char *automount_value = automount_lookup(user_name);
		home_server_len = strcspn(automount_value,":");
		DEBUG(5, ("NIS lookup succeeded.  Home server length: %d\n",home_server_len));
		if (home_server_len > sizeof(pstring))
			home_server_len = sizeof(pstring);
		strncpy(server_name, automount_value, home_server_len);
                server_name[home_server_len] = '\0';
	}
#endif

	DEBUG(4,("Home server: %s\n", server_name));

	return server_name;
}

/****************************************************************************
 Do some standard substitutions in a string.
 len is the length in bytes of the space allowed in string str. If zero means
 don't allow expansions.
****************************************************************************/

void standard_sub_basic(const char *smb_name, char *str, size_t len)
{
	char *s;
	
	if ( (s = alloc_sub_basic( smb_name, str )) != NULL ) {
		strncpy( str, s, len );
	}
	
	SAFE_FREE( s );
	
}

/****************************************************************************
 Do some standard substitutions in a string.
 This function will return an allocated string that have to be freed.
****************************************************************************/

char *talloc_sub_basic(TALLOC_CTX *mem_ctx, const char *smb_name, const char *str)
{
	char *a, *t;
	
	if ( (a = alloc_sub_basic(smb_name, str)) == NULL ) {
		return NULL;
	}
	t = talloc_strdup(mem_ctx, a);
	SAFE_FREE(a);
	return t;
}

/****************************************************************************
****************************************************************************/

char *alloc_sub_basic(const char *smb_name, const char *str)
{
	char *b, *p, *s, *r, *a_string;
	fstring pidstr;
	struct passwd *pass;
	const char *local_machine_name = get_local_machine_name();

	/* workaround to prevent a crash while looking at bug #687 */
	
	if (!str) {
		DEBUG(0,("alloc_sub_basic: NULL source string!  This should not happen\n"));
		return NULL;
	}
	
	a_string = SMB_STRDUP(str);
	if (a_string == NULL) {
		DEBUG(0, ("alloc_sub_specified: Out of memory!\n"));
		return NULL;
	}
	
	for (b = s = a_string; (p = strchr_m(s, '%')); s = a_string + (p - b)) {

		r = NULL;
		b = a_string;
		
		switch (*(p+1)) {
		case 'U' : 
			r = strdup_lower(smb_name);
			if (r == NULL) {
				goto error;
			}
			a_string = realloc_string_sub(a_string, "%U", r);
			break;
		case 'G' :
			r = SMB_STRDUP(smb_name);
			if (r == NULL) {
				goto error;
			}
			if ((pass = Get_Pwnam(r))!=NULL) {
				a_string = realloc_string_sub(a_string, "%G", gidtoname(pass->pw_gid));
			} 
			break;
		case 'D' :
			r = strdup_upper(current_user_info.domain);
			if (r == NULL) {
				goto error;
			}
			a_string = realloc_string_sub(a_string, "%D", r);
			break;
		case 'I' :
			a_string = realloc_string_sub(a_string, "%I", client_addr());
			break;
		case 'i': 
			a_string = realloc_string_sub( a_string, "%i", client_socket_addr() );
			break;
		case 'L' : 
			if ( StrnCaseCmp(p, "%LOGONSERVER%", strlen("%LOGONSERVER%")) == 0 ) {
				break;
			}
			if (local_machine_name && *local_machine_name) {
				a_string = realloc_string_sub(a_string, "%L", local_machine_name); 
			} else {
				a_string = realloc_string_sub(a_string, "%L", global_myname()); 
			}
			break;
		case 'N':
			a_string = realloc_string_sub(a_string, "%N", automount_server(smb_name));
			break;
		case 'M' :
			a_string = realloc_string_sub(a_string, "%M", client_name());
			break;
		case 'R' :
			a_string = realloc_string_sub(a_string, "%R", remote_proto);
			break;
		case 'T' :
			a_string = realloc_string_sub(a_string, "%T", timestring(False));
			break;
		case 'a' :
			a_string = realloc_string_sub(a_string, "%a", remote_arch);
			break;
		case 'd' :
			slprintf(pidstr,sizeof(pidstr)-1, "%d",(int)sys_getpid());
			a_string = realloc_string_sub(a_string, "%d", pidstr);
			break;
		case 'h' :
			a_string = realloc_string_sub(a_string, "%h", myhostname());
			break;
		case 'm' :
			a_string = realloc_string_sub(a_string, "%m", remote_machine);
			break;
		case 'v' :
			a_string = realloc_string_sub(a_string, "%v", SAMBA_VERSION_STRING);
			break;
		case 'w' :
			a_string = realloc_string_sub(a_string, "%w", lp_winbind_separator());
			break;
		case '$' :
			a_string = realloc_expand_env_var(a_string, p); /* Expand environment variables */
			break;
		case '(':
			a_string = realloc_expand_longvar( a_string, p );
			break;
		default: 
			break;
		}

		p++;
		SAFE_FREE(r);
		
		if ( !a_string ) {
			return NULL;
		}
	}

	return a_string;

error:
	SAFE_FREE(a_string);
	return NULL;
}

/****************************************************************************
 Do some specific substitutions in a string.
 This function will return an allocated string that have to be freed.
****************************************************************************/

char *talloc_sub_specified(TALLOC_CTX *mem_ctx,
			const char *input_string,
			const char *username,
			const char *domain,
			uid_t uid,
			gid_t gid)
{
	char *a, *t;
       	a = alloc_sub_specified(input_string, username, domain, uid, gid);
	if (!a) {
		return NULL;
	}
	t = talloc_strdup(mem_ctx, a);
	SAFE_FREE(a);
	return t;
}

/****************************************************************************
****************************************************************************/

char *alloc_sub_specified(const char *input_string,
			const char *username,
			const char *domain,
			uid_t uid,
			gid_t gid)
{
	char *a_string, *ret_string;
	char *b, *p, *s;

	a_string = SMB_STRDUP(input_string);
	if (a_string == NULL) {
		DEBUG(0, ("alloc_sub_specified: Out of memory!\n"));
		return NULL;
	}
	
	for (b = s = a_string; (p = strchr_m(s, '%')); s = a_string + (p - b)) {
		
		b = a_string;
		
		switch (*(p+1)) {
		case 'U' : 
			a_string = realloc_string_sub(a_string, "%U", username);
			break;
		case 'u' : 
			a_string = realloc_string_sub(a_string, "%u", username);
			break;
		case 'G' :
			if (gid != -1) {
				a_string = realloc_string_sub(a_string, "%G", gidtoname(gid));
			} else {
				a_string = realloc_string_sub(a_string, "%G", "NO_GROUP");
			}
			break;
		case 'g' :
			if (gid != -1) {
				a_string = realloc_string_sub(a_string, "%g", gidtoname(gid));
			} else {
				a_string = realloc_string_sub(a_string, "%g", "NO_GROUP");
			}
			break;
		case 'D' :
			a_string = realloc_string_sub(a_string, "%D", domain);
			break;
		case 'N' : 
			a_string = realloc_string_sub(a_string, "%N", automount_server(username)); 
			break;
		default: 
			break;
		}

		p++;
		if (a_string == NULL) {
			return NULL;
		}
	}

	ret_string = alloc_sub_basic(username, a_string);
	SAFE_FREE(a_string);
	return ret_string;
}

/****************************************************************************
****************************************************************************/

char *talloc_sub_advanced(TALLOC_CTX *mem_ctx,
			int snum,
			const char *user,
			const char *connectpath,
			gid_t gid,
			const char *smb_name,
			const char *str)
{
	char *a, *t;
       	a = alloc_sub_advanced(snum, user, connectpath, gid, smb_name, str);
	if (!a) {
		return NULL;
	}
	t = talloc_strdup(mem_ctx, a);
	SAFE_FREE(a);
	return t;
}

/****************************************************************************
****************************************************************************/

char *alloc_sub_advanced(int snum, const char *user, 
				  const char *connectpath, gid_t gid, 
				  const char *smb_name, const char *str)
{
	char *a_string, *ret_string;
	char *b, *p, *s, *h;

	a_string = SMB_STRDUP(str);
	if (a_string == NULL) {
		DEBUG(0, ("alloc_sub_advanced: Out of memory!\n"));
		return NULL;
	}
	
	for (b = s = a_string; (p = strchr_m(s, '%')); s = a_string + (p - b)) {
		
		b = a_string;
		
		switch (*(p+1)) {
		case 'N' :
			a_string = realloc_string_sub(a_string, "%N", automount_server(user));
			break;
		case 'H':
			if ((h = get_user_home_dir(user)))
				a_string = realloc_string_sub(a_string, "%H", h);
			break;
		case 'P': 
			a_string = realloc_string_sub(a_string, "%P", connectpath); 
			break;
		case 'S': 
			a_string = realloc_string_sub(a_string, "%S", lp_servicename(snum)); 
			break;
		case 'g': 
			a_string = realloc_string_sub(a_string, "%g", gidtoname(gid)); 
			break;
		case 'u': 
			a_string = realloc_string_sub(a_string, "%u", user); 
			break;
			
			/* Patch from jkf@soton.ac.uk Left the %N (NIS
			 * server name) in standard_sub_basic as it is
			 * a feature for logon servers, hence uses the
			 * username.  The %p (NIS server path) code is
			 * here as it is used instead of the default
			 * "path =" string in [homes] and so needs the
			 * service name, not the username.  */
		case 'p': 
			a_string = realloc_string_sub(a_string, "%p", automount_path(lp_servicename(snum))); 
			break;
			
		default: 
			break;
		}

		p++;
		if (a_string == NULL) {
			return NULL;
		}
	}

	ret_string = alloc_sub_basic(smb_name, a_string);
	SAFE_FREE(a_string);
	return ret_string;
}

/****************************************************************************
 Do some standard substitutions in a string.
****************************************************************************/

void standard_sub_conn(connection_struct *conn, char *str, size_t len)
{
	char *s;
	
	s = alloc_sub_advanced(SNUM(conn), conn->user, conn->connectpath,
			conn->gid, smb_user_name, str);

	if ( s ) {
		strncpy( str, s, len );
		SAFE_FREE( s );
	}
}

/****************************************************************************
****************************************************************************/

char *talloc_sub_conn(TALLOC_CTX *mem_ctx, connection_struct *conn, const char *str)
{
	return talloc_sub_advanced(mem_ctx, SNUM(conn), conn->user,
			conn->connectpath, conn->gid,
			smb_user_name, str);
}

/****************************************************************************
****************************************************************************/

char *alloc_sub_conn(connection_struct *conn, const char *str)
{
	return alloc_sub_advanced(SNUM(conn), conn->user, conn->connectpath,
			conn->gid, smb_user_name, str);
}

/****************************************************************************
 Like standard_sub but by snum.
****************************************************************************/

void standard_sub_snum(int snum, char *str, size_t len)
{
	static uid_t cached_uid = -1;
	static fstring cached_user;
	char *s;
	
	/* calling uidtoname() on every substitute would be too expensive, so
	   we cache the result here as nearly every call is for the same uid */

	if (cached_uid != current_user.ut.uid) {
		fstrcpy(cached_user, uidtoname(current_user.ut.uid));
		cached_uid = current_user.ut.uid;
	}

	s = alloc_sub_advanced(snum, cached_user, "", current_user.ut.gid,
			      smb_user_name, str);

	if ( s ) {
		strncpy( str, s, len );
		SAFE_FREE( s );
	}
}
