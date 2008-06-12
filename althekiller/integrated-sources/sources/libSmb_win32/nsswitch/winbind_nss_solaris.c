/*
  Solaris NSS wrapper for winbind 
  - Shirish Kalele 2000
  
  Based on Luke Howard's ldap_nss module for Solaris 
  */

/*
  Copyright (C) 1997-2003 Luke Howard.
  This file is part of the nss_ldap library.

  The nss_ldap library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.

  The nss_ldap library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with the nss_ldap library; see the file COPYING.LIB.  If not,
  write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA.
*/

#undef DEVELOPER

#include <stdlib.h>
#include <sys/types.h>
#include <sys/param.h>
#include <string.h>
#include <pwd.h>
#include "includes.h"
#include <syslog.h>
#if !defined(HPUX)
#include <sys/syslog.h>
#endif /*hpux*/
#include "winbind_nss_config.h"

#if defined(HAVE_NSS_COMMON_H) || defined(HPUX) 

#undef NSS_DEBUG

#ifdef NSS_DEBUG
#define NSS_DEBUG(str) syslog(LOG_DEBUG, "nss_winbind: %s", str);
#else
#define NSS_DEBUG(str) ;
#endif

#define NSS_ARGS(args) ((nss_XbyY_args_t *)args)

#ifdef HPUX

/*
 * HP-UX 11 has no definiton of the nss_groupsbymem structure.   This
 * definition is taken from the nss_ldap project at:
 *  http://www.padl.com/OSS/nss_ldap.html
 */

struct nss_groupsbymem {
       const char *username;
       gid_t *gid_array;
       int maxgids;
       int force_slow_way;
       int (*str2ent)(const char *instr, int instr_len, void *ent, 
		      char *buffer, int buflen);
       nss_status_t (*process_cstr)(const char *instr, int instr_len, 
				    struct nss_groupsbymem *);
       int numgids;
};

#endif /* HPUX */

#define make_pwent_str(dest, src) 					\
{									\
  if((dest = get_static(buffer, buflen, strlen(src)+1)) == NULL)	\
    {									\
      *errnop = ERANGE;							\
      NSS_DEBUG("ERANGE error");					\
      return NSS_STATUS_TRYAGAIN; 		       			\
    }									\
  strcpy(dest, src);							\
}

static NSS_STATUS _nss_winbind_setpwent_solwrap (nss_backend_t* be, void* args)
{
	NSS_DEBUG("_nss_winbind_setpwent_solwrap");
	return _nss_winbind_setpwent();
}

static NSS_STATUS
_nss_winbind_endpwent_solwrap (nss_backend_t * be, void *args)
{
	NSS_DEBUG("_nss_winbind_endpwent_solwrap");
	return _nss_winbind_endpwent();
}

static NSS_STATUS
_nss_winbind_getpwent_solwrap (nss_backend_t* be, void *args)
{
	NSS_STATUS ret;
	char* buffer = NSS_ARGS(args)->buf.buffer;
	int buflen = NSS_ARGS(args)->buf.buflen;
	struct passwd* result = (struct passwd*) NSS_ARGS(args)->buf.result;
	int* errnop = &NSS_ARGS(args)->erange;
	char logmsg[80];

	ret = _nss_winbind_getpwent_r(result, buffer, 
				      buflen, errnop);

	if(ret == NSS_STATUS_SUCCESS)
		{
			snprintf(logmsg, 79, "_nss_winbind_getpwent_solwrap: Returning user: %s\n",
				 result->pw_name);
			NSS_DEBUG(logmsg);
			NSS_ARGS(args)->returnval = (void*) result;
		} else {
			snprintf(logmsg, 79, "_nss_winbind_getpwent_solwrap: Returning error: %d.\n",ret);
			NSS_DEBUG(logmsg);
		}
    
	return ret;
}

static NSS_STATUS
_nss_winbind_getpwnam_solwrap (nss_backend_t* be, void* args)
{
	NSS_STATUS ret;
	struct passwd* result = (struct passwd*) NSS_ARGS(args)->buf.result;

	NSS_DEBUG("_nss_winbind_getpwnam_solwrap");

	ret = _nss_winbind_getpwnam_r (NSS_ARGS(args)->key.name,
						result,
						NSS_ARGS(args)->buf.buffer,
						NSS_ARGS(args)->buf.buflen,
						&NSS_ARGS(args)->erange);
	if(ret == NSS_STATUS_SUCCESS)
		NSS_ARGS(args)->returnval = (void*) result;
  
	return ret;
}

static NSS_STATUS
_nss_winbind_getpwuid_solwrap(nss_backend_t* be, void* args)
{
	NSS_STATUS ret;
	struct passwd* result = (struct passwd*) NSS_ARGS(args)->buf.result;
  
	NSS_DEBUG("_nss_winbind_getpwuid_solwrap");
	ret = _nss_winbind_getpwuid_r (NSS_ARGS(args)->key.uid,
				       result,
				       NSS_ARGS(args)->buf.buffer,
				       NSS_ARGS(args)->buf.buflen,
				       &NSS_ARGS(args)->erange);
	if(ret == NSS_STATUS_SUCCESS)
		NSS_ARGS(args)->returnval = (void*) result;
  
	return ret;
}

static NSS_STATUS _nss_winbind_passwd_destr (nss_backend_t * be, void *args)
{
	SAFE_FREE(be);
	NSS_DEBUG("_nss_winbind_passwd_destr");
	return NSS_STATUS_SUCCESS;
}

static nss_backend_op_t passwd_ops[] =
{
	_nss_winbind_passwd_destr,
	_nss_winbind_endpwent_solwrap,		/* NSS_DBOP_ENDENT */
	_nss_winbind_setpwent_solwrap,		/* NSS_DBOP_SETENT */
	_nss_winbind_getpwent_solwrap,		/* NSS_DBOP_GETENT */
	_nss_winbind_getpwnam_solwrap,		/* NSS_DBOP_PASSWD_BYNAME */
	_nss_winbind_getpwuid_solwrap		/* NSS_DBOP_PASSWD_BYUID */
};

nss_backend_t*
_nss_winbind_passwd_constr (const char* db_name,
			    const char* src_name,
			    const char* cfg_args)
{
	nss_backend_t *be;
  
	if(!(be = SMB_MALLOC_P(nss_backend_t)) )
		return NULL;

	be->ops = passwd_ops;
	be->n_ops = sizeof(passwd_ops) / sizeof(nss_backend_op_t);

	NSS_DEBUG("Initialized nss_winbind passwd backend");
	return be;
}

/*****************************************************************
 GROUP database backend
 *****************************************************************/

static NSS_STATUS _nss_winbind_setgrent_solwrap (nss_backend_t* be, void* args)
{
	NSS_DEBUG("_nss_winbind_setgrent_solwrap");
	return _nss_winbind_setgrent();
}

static NSS_STATUS
_nss_winbind_endgrent_solwrap (nss_backend_t * be, void *args)
{
	NSS_DEBUG("_nss_winbind_endgrent_solwrap");
	return _nss_winbind_endgrent();
}

static NSS_STATUS
_nss_winbind_getgrent_solwrap(nss_backend_t* be, void* args)
{
	NSS_STATUS ret;
	char* buffer = NSS_ARGS(args)->buf.buffer;
	int buflen = NSS_ARGS(args)->buf.buflen;
	struct group* result = (struct group*) NSS_ARGS(args)->buf.result;
	int* errnop = &NSS_ARGS(args)->erange;
	char logmsg[80];

	ret = _nss_winbind_getgrent_r(result, buffer, 
				      buflen, errnop);

	if(ret == NSS_STATUS_SUCCESS)
		{
			snprintf(logmsg, 79, "_nss_winbind_getgrent_solwrap: Returning group: %s\n", result->gr_name);
			NSS_DEBUG(logmsg);
			NSS_ARGS(args)->returnval = (void*) result;
		} else {
			snprintf(logmsg, 79, "_nss_winbind_getgrent_solwrap: Returning error: %d.\n", ret);
			NSS_DEBUG(logmsg);
		}

	return ret;
	
}

static NSS_STATUS
_nss_winbind_getgrnam_solwrap(nss_backend_t* be, void* args)
{
	NSS_STATUS ret;
	struct group* result = (struct group*) NSS_ARGS(args)->buf.result;

	NSS_DEBUG("_nss_winbind_getgrnam_solwrap");
	ret = _nss_winbind_getgrnam_r(NSS_ARGS(args)->key.name,
				      result,
				      NSS_ARGS(args)->buf.buffer,
				      NSS_ARGS(args)->buf.buflen,
				      &NSS_ARGS(args)->erange);

	if(ret == NSS_STATUS_SUCCESS)
		NSS_ARGS(args)->returnval = (void*) result;
  
	return ret;
}
  
static NSS_STATUS
_nss_winbind_getgrgid_solwrap(nss_backend_t* be, void* args)
{
	NSS_STATUS ret;
	struct group* result = (struct group*) NSS_ARGS(args)->buf.result;

	NSS_DEBUG("_nss_winbind_getgrgid_solwrap");
	ret = _nss_winbind_getgrgid_r (NSS_ARGS(args)->key.gid,
				       result,
				       NSS_ARGS(args)->buf.buffer,
				       NSS_ARGS(args)->buf.buflen,
				       &NSS_ARGS(args)->erange);

	if(ret == NSS_STATUS_SUCCESS)
		NSS_ARGS(args)->returnval = (void*) result;

	return ret;
}

static NSS_STATUS
_nss_winbind_getgroupsbymember_solwrap(nss_backend_t* be, void* args)
{
	int errnop;
	struct nss_groupsbymem *gmem = (struct nss_groupsbymem *)args;

	NSS_DEBUG("_nss_winbind_getgroupsbymember");

	_nss_winbind_initgroups_dyn(gmem->username,
		gmem->gid_array[0], /* Primary Group */
		&gmem->numgids,
		&gmem->maxgids,
		&gmem->gid_array,
		gmem->maxgids,
		&errnop);

	/*
	 * If the maximum number of gids have been found, return
	 * SUCCESS so the switch engine will stop searching. Otherwise
	 * return NOTFOUND so nsswitch will continue to get groups
	 * from the remaining database backends specified in the
	 * nsswitch.conf file.
	 */
	return (gmem->numgids == gmem->maxgids ? NSS_STATUS_SUCCESS : NSS_STATUS_NOTFOUND);
}

static NSS_STATUS
_nss_winbind_group_destr (nss_backend_t* be, void* args)
{
	SAFE_FREE(be);
	NSS_DEBUG("_nss_winbind_group_destr");
	return NSS_STATUS_SUCCESS;
}

static nss_backend_op_t group_ops[] = 
{
	_nss_winbind_group_destr,
	_nss_winbind_endgrent_solwrap,
	_nss_winbind_setgrent_solwrap,
	_nss_winbind_getgrent_solwrap,
	_nss_winbind_getgrnam_solwrap,
	_nss_winbind_getgrgid_solwrap,
	_nss_winbind_getgroupsbymember_solwrap
}; 

nss_backend_t*
_nss_winbind_group_constr (const char* db_name,
			   const char* src_name,
			   const char* cfg_args)
{
	nss_backend_t* be;

	if(!(be = SMB_MALLOC_P(nss_backend_t)) )
		return NULL;

	be->ops = group_ops;
	be->n_ops = sizeof(group_ops) / sizeof(nss_backend_op_t);
  
	NSS_DEBUG("Initialized nss_winbind group backend");
	return be;
}

/*****************************************************************
 hosts and ipnodes backend
 *****************************************************************/
#if defined(SUNOS5)	/* not compatible with HP-UX */

/* this parser is shared between get*byname and get*byaddr, as key type
   in request is stored in different locations, I had to provide the
   address family as an argument, caller must free the winbind response. */

static NSS_STATUS
parse_response(int af, nss_XbyY_args_t* argp, struct winbindd_response *response)
{
	struct hostent *he = (struct hostent *)argp->buf.result;
	char *buffer = argp->buf.buffer;
	int buflen =  argp->buf.buflen;
	NSS_STATUS ret;

	char *p, *data;
	int addrcount = 0;
	int len = 0;
	struct in_addr *addrp;
	struct in6_addr *addrp6;
	int i;

	/* response is tab separated list of ip addresses with hostname
	   and newline at the end. so at first we will strip newline
	   then construct list of addresses for hostent.
	*/
	p = strchr(response->data.winsresp, '\n');
	if(p) *p = '\0';
	else {/* it must be broken */
		argp->h_errno = NO_DATA;
		return NSS_STATUS_UNAVAIL;
	}

	for(; p != response->data.winsresp; p--) {
		if(*p == '\t') addrcount++;
	}

	if(addrcount == 0) {/* it must be broken */
		argp->h_errno = NO_DATA;
		return NSS_STATUS_UNAVAIL;
	}

	/* allocate space for addresses and h_addr_list */
	he->h_addrtype = af;
	if( he->h_addrtype == AF_INET) {
		he->h_length =  sizeof(struct in_addr);
		addrp = (struct in_addr *)ROUND_DOWN(buffer + buflen,
						sizeof(struct in_addr));
		addrp -= addrcount;
		he->h_addr_list = (char **)ROUND_DOWN(addrp, sizeof (char*));
		he->h_addr_list -= addrcount+1;
	} else {
		he->h_length = sizeof(struct in6_addr);
		addrp6 = (struct in6_addr *)ROUND_DOWN(buffer + buflen,
						sizeof(struct in6_addr));
		addrp6 -= addrcount;
		he->h_addr_list = (char **)ROUND_DOWN(addrp6, sizeof (char*));
		he->h_addr_list -= addrcount+1;
	}

	/* buffer too small?! */
	if((char *)he->h_addr_list < buffer ) {
		argp->erange = 1;
		return NSS_STR_PARSE_ERANGE;
	}
	
	data = response->data.winsresp;
	for( i = 0; i < addrcount; i++) {
		p = strchr(data, '\t');
		if(p == NULL) break; /* just in case... */

		*p = '\0'; /* terminate the string */
		if(he->h_addrtype == AF_INET) {
		  he->h_addr_list[i] = (char *)&addrp[i];
		  if ((addrp[i].s_addr = inet_addr(data)) == -1) {
		    argp->erange = 1;
		    return NSS_STR_PARSE_ERANGE;
		  }
		} else {
		  he->h_addr_list[i] = (char *)&addrp6[i];
		  if (strchr(data, ':') != 0) {
			if (inet_pton(AF_INET6, data, &addrp6[i]) != 1) {
			  argp->erange = 1;
			  return NSS_STR_PARSE_ERANGE;
			}
		  } else {
			struct in_addr in4;
			if ((in4.s_addr = inet_addr(data)) == -1) {
			  argp->erange = 1;
			  return NSS_STR_PARSE_ERANGE;
			}
			IN6_INADDR_TO_V4MAPPED(&in4, &addrp6[i]);
		  }
		}
		data = p+1;
	}

	he->h_addr_list[i] = (char *)NULL;

	len = strlen(data);
	if(len > he->h_addr_list - (char**)argp->buf.buffer) {
		argp->erange = 1;
		return NSS_STR_PARSE_ERANGE;
	}

	/* this is a bit overkill to use _nss_netdb_aliases here since
	   there seems to be no aliases but it will create all data for us */
	he->h_aliases = _nss_netdb_aliases(data, len, buffer,
				((char*) he->h_addr_list) - buffer);
	if(he->h_aliases == NULL) {
	    argp->erange = 1;
	    ret = NSS_STR_PARSE_ERANGE;
	} else {
	    he->h_name = he->h_aliases[0];
	    he->h_aliases++;
	    ret = NSS_STR_PARSE_SUCCESS;
	}

	argp->returnval = (void*)he;
	return ret;
}

static NSS_STATUS
_nss_winbind_ipnodes_getbyname(nss_backend_t* be, void *args)
{
	nss_XbyY_args_t *argp = (nss_XbyY_args_t*) args;
	struct winbindd_response response;
	struct winbindd_request request;
	NSS_STATUS ret;
	int af;

	ZERO_STRUCT(response);
	ZERO_STRUCT(request);

	/* I assume there that AI_ADDRCONFIG cases are handled in nss
	   frontend code, at least it seems done so in solaris...

	   we will give NO_DATA for pure IPv6; IPv4 will be returned for
	   AF_INET or for AF_INET6 and AI_ALL|AI_V4MAPPED we have to map
	   IPv4 to IPv6.
	 */
#ifdef HAVE_NSS_XBYY_KEY_IPNODE
	af = argp->key.ipnode.af_family;
	if(af == AF_INET6 && argp->key.ipnode.flags == 0) {
		argp->h_errno = NO_DATA;
		return NSS_STATUS_UNAVAIL;
	}
#else
	/* I'm not that sure if this is correct, but... */
	af = AF_INET6;
#endif

	strncpy(request.data.winsreq, argp->key.name, strlen(argp->key.name)) ;

	if( (ret = winbindd_request_response(WINBINDD_WINS_BYNAME, &request, &response))
		== NSS_STATUS_SUCCESS ) {
	  ret = parse_response(af, argp, &response);
	}

	free_response(&response);
	return ret;
}

static NSS_STATUS
_nss_winbind_hosts_getbyname(nss_backend_t* be, void *args)
{
	nss_XbyY_args_t *argp = (nss_XbyY_args_t*) args;
	struct winbindd_response response;
	struct winbindd_request request;
	NSS_STATUS ret;

	ZERO_STRUCT(response);
	ZERO_STRUCT(request);
	
	strncpy(request.data.winsreq, argp->key.name, strlen(argp->key.name));

	if( (ret = winbindd_request_response(WINBINDD_WINS_BYNAME, &request, &response))
		== NSS_STATUS_SUCCESS ) {
	  ret = parse_response(AF_INET, argp, &response);
	}

	free_response(&response);
	return ret;
}

static NSS_STATUS
_nss_winbind_hosts_getbyaddr(nss_backend_t* be, void *args)
{
	NSS_STATUS ret;
	struct winbindd_response response;
	struct winbindd_request request;
	nss_XbyY_args_t	*argp = (nss_XbyY_args_t *)args;
	const char *p;

	ZERO_STRUCT(response);
	ZERO_STRUCT(request);

	/* winbindd currently does not resolve IPv6 */
	if(argp->key.hostaddr.type == AF_INET6) {
		argp->h_errno = NO_DATA;
		return NSS_STATUS_UNAVAIL;
	}

	p = inet_ntop(argp->key.hostaddr.type, argp->key.hostaddr.addr,
			request.data.winsreq, INET6_ADDRSTRLEN);

	ret = winbindd_request_response(WINBINDD_WINS_BYIP, &request, &response);

	if( ret == NSS_STATUS_SUCCESS) {
	  parse_response(argp->key.hostaddr.type, argp, &response);
	}
	free_response(&response);
        return ret;
}

/* winbind does not provide setent, getent, endent for wins */
static NSS_STATUS
_nss_winbind_common_endent(nss_backend_t* be, void *args)
{
        return (NSS_STATUS_UNAVAIL);
}

static NSS_STATUS
_nss_winbind_common_setent(nss_backend_t* be, void *args)
{
        return (NSS_STATUS_UNAVAIL);
}

static NSS_STATUS
_nss_winbind_common_getent(nss_backend_t* be, void *args)
{
        return (NSS_STATUS_UNAVAIL);
}

static nss_backend_t*
_nss_winbind_common_constr (nss_backend_op_t ops[], int n_ops)
{
	nss_backend_t* be;

	if(!(be = SMB_MALLOC_P(nss_backend_t)) )
	return NULL;

	be->ops = ops;
	be->n_ops = n_ops;

	return be;
}

static NSS_STATUS
_nss_winbind_common_destr (nss_backend_t* be, void* args)
{
	SAFE_FREE(be);
	return NSS_STATUS_SUCCESS;
}

static nss_backend_op_t ipnodes_ops[] = {
	_nss_winbind_common_destr,
	_nss_winbind_common_endent,
	_nss_winbind_common_setent,
	_nss_winbind_common_getent,
	_nss_winbind_ipnodes_getbyname,
	_nss_winbind_hosts_getbyaddr,
};

nss_backend_t *
_nss_winbind_ipnodes_constr(dummy1, dummy2, dummy3)
        const char      *dummy1, *dummy2, *dummy3;
{
	return (_nss_winbind_common_constr(ipnodes_ops,
		sizeof (ipnodes_ops) / sizeof (ipnodes_ops[0])));
}

static nss_backend_op_t host_ops[] = {
	_nss_winbind_common_destr,
	_nss_winbind_common_endent,
	_nss_winbind_common_setent,
	_nss_winbind_common_getent,
	_nss_winbind_hosts_getbyname,
	_nss_winbind_hosts_getbyaddr,
};

nss_backend_t *
_nss_winbind_hosts_constr(dummy1, dummy2, dummy3)
        const char      *dummy1, *dummy2, *dummy3;
{
	return (_nss_winbind_common_constr(host_ops,
		sizeof (host_ops) / sizeof (host_ops[0])));
}

#endif	/* defined(SUNOS5) */
#endif 	/* defined(HAVE_NSS_COMMON_H) || defined(HPUX) */
