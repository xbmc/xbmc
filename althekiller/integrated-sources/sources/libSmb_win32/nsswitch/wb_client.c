/* 
   Unix SMB/CIFS implementation.

   winbind client code

   Copyright (C) Tim Potter 2000
   Copyright (C) Andrew Tridgell 2000
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA  02111-1307, USA.   
*/

#include "includes.h"
#include "nsswitch/winbind_nss.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_WINBIND

NSS_STATUS winbindd_request_response(int req_type,
                                 struct winbindd_request *request,
                                 struct winbindd_response *response);

/* Call winbindd to convert a name to a sid */

BOOL winbind_lookup_name(const char *dom_name, const char *name, DOM_SID *sid, 
                         enum SID_NAME_USE *name_type)
{
	struct winbindd_request request;
	struct winbindd_response response;
	NSS_STATUS result;
	
	if (!sid || !name_type)
		return False;

	/* Send off request */

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	fstrcpy(request.data.name.dom_name, dom_name);
	fstrcpy(request.data.name.name, name);

	if ((result = winbindd_request_response(WINBINDD_LOOKUPNAME, &request, 
				       &response)) == NSS_STATUS_SUCCESS) {
		if (!string_to_sid(sid, response.data.sid.sid))
			return False;
		*name_type = (enum SID_NAME_USE)response.data.sid.type;
	}

	return result == NSS_STATUS_SUCCESS;
}

/* Call winbindd to convert sid to name */

BOOL winbind_lookup_sid(TALLOC_CTX *mem_ctx, const DOM_SID *sid, 
			const char **domain, const char **name,
                        enum SID_NAME_USE *name_type)
{
	struct winbindd_request request;
	struct winbindd_response response;
	NSS_STATUS result;
	
	/* Initialise request */

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	fstrcpy(request.data.sid, sid_string_static(sid));
	
	/* Make request */

	result = winbindd_request_response(WINBINDD_LOOKUPSID, &request,
					   &response);

	if (result != NSS_STATUS_SUCCESS) {
		return False;
	}

	/* Copy out result */

	if (domain != NULL) {
		*domain = talloc_strdup(mem_ctx, response.data.name.dom_name);
		if (*domain == NULL) {
			DEBUG(0, ("talloc failed\n"));
			return False;
		}
	}
	if (name != NULL) {
		*name = talloc_strdup(mem_ctx, response.data.name.name);
		if (*name == NULL) {
			DEBUG(0, ("talloc failed\n"));
			return False;
		}
	}

	*name_type = (enum SID_NAME_USE)response.data.name.type;

	DEBUG(10, ("winbind_lookup_sid: SUCCESS: SID %s -> %s %s\n", 
		   sid_string_static(sid), response.data.name.dom_name,
		   response.data.name.name));
	return True;
}

/* Call winbindd to convert SID to uid */

BOOL winbind_sid_to_uid(uid_t *puid, const DOM_SID *sid)
{
	struct winbindd_request request;
	struct winbindd_response response;
	int result;
	fstring sid_str;

	if (!puid)
		return False;

	/* Initialise request */

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	sid_to_string(sid_str, sid);
	fstrcpy(request.data.sid, sid_str);
	
	/* Make request */

	result = winbindd_request_response(WINBINDD_SID_TO_UID, &request, &response);

	/* Copy out result */

	if (result == NSS_STATUS_SUCCESS) {
		*puid = response.data.uid;
	}

	return (result == NSS_STATUS_SUCCESS);
}

/* Call winbindd to convert uid to sid */

BOOL winbind_uid_to_sid(DOM_SID *sid, uid_t uid)
{
	struct winbindd_request request;
	struct winbindd_response response;
	int result;

	if (!sid)
		return False;

	/* Initialise request */

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	request.data.uid = uid;

	/* Make request */

	result = winbindd_request_response(WINBINDD_UID_TO_SID, &request, &response);

	/* Copy out result */

	if (result == NSS_STATUS_SUCCESS) {
		if (!string_to_sid(sid, response.data.sid.sid))
			return False;
	} else {
		sid_copy(sid, &global_sid_NULL);
	}

	return (result == NSS_STATUS_SUCCESS);
}

/* Call winbindd to convert SID to gid */

BOOL winbind_sid_to_gid(gid_t *pgid, const DOM_SID *sid)
{
	struct winbindd_request request;
	struct winbindd_response response;
	int result;
	fstring sid_str;

	if (!pgid)
		return False;

	/* Initialise request */

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	sid_to_string(sid_str, sid);
	fstrcpy(request.data.sid, sid_str);
	
	/* Make request */

	result = winbindd_request_response(WINBINDD_SID_TO_GID, &request, &response);

	/* Copy out result */

	if (result == NSS_STATUS_SUCCESS) {
		*pgid = response.data.gid;
	}

	return (result == NSS_STATUS_SUCCESS);
}

/* Call winbindd to convert gid to sid */

BOOL winbind_gid_to_sid(DOM_SID *sid, gid_t gid)
{
	struct winbindd_request request;
	struct winbindd_response response;
	int result;

	if (!sid)
		return False;

	/* Initialise request */

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	request.data.gid = gid;

	/* Make request */

	result = winbindd_request_response(WINBINDD_GID_TO_SID, &request, &response);

	/* Copy out result */

	if (result == NSS_STATUS_SUCCESS) {
		if (!string_to_sid(sid, response.data.sid.sid))
			return False;
	} else {
		sid_copy(sid, &global_sid_NULL);
	}

	return (result == NSS_STATUS_SUCCESS);
}

BOOL winbind_allocate_uid(uid_t *uid)
{
	struct winbindd_request request;
	struct winbindd_response response;
	int result;

	/* Initialise request */

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	/* Make request */

	result = winbindd_request_response(WINBINDD_ALLOCATE_UID,
					   &request, &response);

	if (result != NSS_STATUS_SUCCESS)
		return False;

	/* Copy out result */
	*uid = response.data.uid;

	return True;
}

BOOL winbind_allocate_gid(gid_t *gid)
{
	struct winbindd_request request;
	struct winbindd_response response;
	int result;

	/* Initialise request */

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	/* Make request */

	result = winbindd_request_response(WINBINDD_ALLOCATE_GID,
					   &request, &response);

	if (result != NSS_STATUS_SUCCESS)
		return False;

	/* Copy out result */
	*gid = response.data.gid;

	return True;
}

/* Fetch the list of groups a user is a member of from winbindd.  This is
   used by winbind_getgroups. */

static int wb_getgroups(const char *user, gid_t **groups)
{
	struct winbindd_request request;
	struct winbindd_response response;
	int result;

	/* Call winbindd */

	ZERO_STRUCT(request);
	fstrcpy(request.data.username, user);

	ZERO_STRUCT(response);

	result = winbindd_request_response(WINBINDD_GETGROUPS, &request, &response);

	if (result == NSS_STATUS_SUCCESS) {
		
		/* Return group list.  Don't forget to free the group list
		   when finished. */

		*groups = (gid_t *)response.extra_data.data;
		return response.data.num_entries;
	}

	return -1;
}

/* Call winbindd to initialise group membership.  This is necessary for
   some systems (i.e RH5.2) that do not have an initgroups function as part
   of the nss extension.  In RH5.2 this is implemented using getgrent()
   which can be amazingly inefficient as well as having problems with
   username case. */

int winbind_initgroups(char *user, gid_t gid)
{
	gid_t *groups = NULL;
	int result;

	/* Call normal initgroups if we are a local user */

	if (!strchr(user, *lp_winbind_separator())) {
		return initgroups(user, gid);
	}

	result = wb_getgroups(user, &groups);

	DEBUG(10,("winbind_getgroups: %s: result = %s\n", user, 
		  result == -1 ? "FAIL" : "SUCCESS"));

	if (result != -1) {
		int ngroups = result, i;
		BOOL is_member = False;

		/* Check to see if the passed gid is already in the list */

		for (i = 0; i < ngroups; i++) {
			if (groups[i] == gid) {
				is_member = True;
			}
		}

		/* Add group to list if necessary */

		if (!is_member) {
			groups = SMB_REALLOC_ARRAY(groups, gid_t, ngroups + 1);
			if (!groups) {
				errno = ENOMEM;
				result = -1;
				goto done;
			}

			groups[ngroups] = gid;
			ngroups++;
		}

		/* Set the groups */

		if (sys_setgroups(ngroups, groups) == -1) {
			errno = EPERM;
			result = -1;
			goto done;
		}

	} else {
		
		/* The call failed.  Set errno to something so we don't get
		   a bogus value from the last failed system call. */

		errno = EIO;
	}

	/* Free response data if necessary */

 done:
	SAFE_FREE(groups);

	return result;
}

/* Return a list of groups the user is a member of.  This function is
   useful for large systems where inverting the group database would be too
   time consuming.  If size is zero, list is not modified and the total
   number of groups for the user is returned. */

int winbind_getgroups(const char *user, gid_t **list)
{
	/*
	 * Don't do the lookup if the name has no separator _and_ we are not in
	 * 'winbind use default domain' mode.
	 */

	if (!(strchr(user, *lp_winbind_separator()) || lp_winbind_use_default_domain()))
		return -1;

	/* Fetch list of groups */

	return wb_getgroups(user, list);
}

/**********************************************************************
 simple wrapper function to see if winbindd is alive
**********************************************************************/

BOOL winbind_ping( void )
{
	NSS_STATUS result;

	result = winbindd_request_response(WINBINDD_PING, NULL, NULL);

	return result == NSS_STATUS_SUCCESS;
}

/**********************************************************************
 Is a domain trusted?

 result == NSS_STATUS_UNAVAIL: winbind not around
 result == NSS_STATUS_NOTFOUND: winbind around, but domain missing

 Due to a bad API NSS_STATUS_NOTFOUND is returned both when winbind_off and
 when winbind return WINBINDD_ERROR. So the semantics of this routine depends
 on winbind_on. Grepping for winbind_off I just found 3 places where winbind
 is turned off, and this does not conflict (as far as I have seen) with the
 callers of is_trusted_domains.

 I *hate* global variables....

 Volker

**********************************************************************/

NSS_STATUS wb_is_trusted_domain(const char *domain)
{
	struct winbindd_request request;
	struct winbindd_response response;

	/* Call winbindd */

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	fstrcpy(request.domain_name, domain);

	return winbindd_request_response(WINBINDD_DOMAIN_INFO, &request, &response);
}
