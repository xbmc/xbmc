/*
 *  idmap_ad: map between Active Directory and RFC 2307 or "Services for Unix" (SFU) Accounts
 *
 * Unix SMB/CIFS implementation.
 *
 * Winbind ADS backend functions
 *
 * Copyright (C) Andrew Tridgell 2001
 * Copyright (C) Andrew Bartlett <abartlet@samba.org> 2003
 * Copyright (C) Gerald (Jerry) Carter 2004
 * Copyright (C) Luke Howard 2001-2004
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_IDMAP

#define WINBIND_CCACHE_NAME "MEMORY:winbind_ccache"

NTSTATUS init_module(void);

static ADS_STRUCT *ad_idmap_ads = NULL;

static char *attr_uidnumber = NULL;
static char *attr_gidnumber = NULL;

static ADS_STATUS ad_idmap_check_attr_mapping(ADS_STRUCT *ads)
{
	ADS_STATUS status;
	enum wb_posix_mapping map_type;

	if (attr_uidnumber != NULL && attr_gidnumber != NULL) {
		return ADS_ERROR(LDAP_SUCCESS);
	}

	SMB_ASSERT(ads->server.workgroup);

	map_type = get_nss_info(ads->server.workgroup);

	if ((map_type == WB_POSIX_MAP_SFU) ||
	    (map_type == WB_POSIX_MAP_RFC2307)) {

		status = ads_check_posix_schema_mapping(ads, map_type);
		if (ADS_ERR_OK(status)) {
			attr_uidnumber = SMB_STRDUP(ads->schema.posix_uidnumber_attr);
			attr_gidnumber = SMB_STRDUP(ads->schema.posix_gidnumber_attr);
			ADS_ERROR_HAVE_NO_MEMORY(attr_uidnumber);
			ADS_ERROR_HAVE_NO_MEMORY(attr_gidnumber);
			return ADS_ERROR(LDAP_SUCCESS);
		} else {
			DEBUG(0,("ads_check_posix_schema_mapping failed: %s\n", ads_errstr(status)));
			/* return status; */
		}
	}
	
	/* fallback to XAD defaults */
	attr_uidnumber = SMB_STRDUP("uidNumber");
	attr_gidnumber = SMB_STRDUP("gidNumber");
	ADS_ERROR_HAVE_NO_MEMORY(attr_uidnumber);
	ADS_ERROR_HAVE_NO_MEMORY(attr_gidnumber);

	return ADS_ERROR(LDAP_SUCCESS);
}

static ADS_STRUCT *ad_idmap_cached_connection(void)
{
	ADS_STRUCT *ads;
	ADS_STATUS status;
	BOOL local = False;

	if (ad_idmap_ads != NULL) {
		ads = ad_idmap_ads;

		/* check for a valid structure */

		DEBUG(7, ("Current tickets expire at %d, time is now %d\n",
			  (uint32) ads->auth.expire, (uint32) time(NULL)));
		if ( ads->config.realm && (ads->auth.expire > time(NULL))) {
			return ads;
		} else {
			/* we own this ADS_STRUCT so make sure it goes away */
			ads->is_mine = True;
			ads_destroy( &ads );
			ads_kdestroy(WINBIND_CCACHE_NAME);
			ad_idmap_ads = NULL;
		}
	}

	if (!local) {
		/* we don't want this to affect the users ccache */
		setenv("KRB5CCNAME", WINBIND_CCACHE_NAME, 1);
	}

	ads = ads_init(lp_realm(), lp_workgroup(), NULL);
	if (!ads) {
		DEBUG(1,("ads_init failed\n"));
		return NULL;
	}

	/* the machine acct password might have change - fetch it every time */
	SAFE_FREE(ads->auth.password);
	ads->auth.password = secrets_fetch_machine_password(lp_workgroup(), NULL, NULL);

	SAFE_FREE(ads->auth.realm);
	ads->auth.realm = SMB_STRDUP(lp_realm());

	status = ads_connect(ads);
	if (!ADS_ERR_OK(status)) {
		DEBUG(1, ("ad_idmap_init: failed to connect to AD\n"));
		ads_destroy(&ads);
		return NULL;
	}

	ads->is_mine = False;

	status = ad_idmap_check_attr_mapping(ads);
	if (!ADS_ERR_OK(status)) {
		DEBUG(1, ("ad_idmap_init: failed to check attribute mapping\n"));
		return NULL;
	}

	ad_idmap_ads = ads;
	return ads;
}

/* no op */
static NTSTATUS ad_idmap_init(char *uri)
{
	return NT_STATUS_OK;
}

static NTSTATUS ad_idmap_get_sid_from_id(DOM_SID *sid, unid_t unid, int id_type)
{
	ADS_STATUS rc;
	NTSTATUS status = NT_STATUS_NONE_MAPPED;
	const char *attrs[] = { "objectSid", NULL };
	void *res = NULL;
	void *msg = NULL;
	char *expr = NULL;
	fstring sid_string;
	int count;
	ADS_STRUCT *ads;

	if (sid == NULL) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	ads = ad_idmap_cached_connection();
	if (ads == NULL) {
		DEBUG(1, ("ad_idmap_get_id_from_sid ADS uninitialized\n"));
		return NT_STATUS_NOT_SUPPORTED;
	}

	switch (id_type & ID_TYPEMASK) {
		case ID_USERID:
			if (asprintf(&expr, "(&(|(sAMAccountType=%d)(sAMAccountType=%d)(sAMAccountType=%d))(%s=%d))",
				ATYPE_NORMAL_ACCOUNT, ATYPE_WORKSTATION_TRUST, ATYPE_INTERDOMAIN_TRUST,
				ads->schema.posix_uidnumber_attr, (int)unid.uid) == -1) {
				return NT_STATUS_NO_MEMORY;
			}
			break;
		case ID_GROUPID:
			if (asprintf(&expr, "(&(|(sAMAccountType=%d)(sAMAccountType=%d))(%s=%d))",
				ATYPE_SECURITY_GLOBAL_GROUP, ATYPE_SECURITY_LOCAL_GROUP,
				ads->schema.posix_gidnumber_attr, (int)unid.gid) == -1) {
				return NT_STATUS_NO_MEMORY;
			}
			break;
		default:
			return NT_STATUS_INVALID_PARAMETER;
			break;
	}

	rc = ads_search_retry(ads, &res, expr, attrs);
	free(expr);
	if (!ADS_ERR_OK(rc)) {
		DEBUG(1, ("ad_idmap_get_sid_from_id: ads_search: %s\n", ads_errstr(rc)));
		goto done;
	}

	count = ads_count_replies(ads, res);
	if (count == 0) {
		DEBUG(1, ("ad_idmap_get_sid_from_id: ads_count_replies: no results\n"));
		goto done;
	} else if (count != 1) {
		DEBUG(1, ("ad_idmap_get_sid_from_id: ads_count_replies: incorrect cardinality\n"));
		goto done;
	}

	msg = ads_first_entry(ads, res);
	if (msg == NULL) {
		DEBUG(1, ("ad_idmap_get_sid_from_id: ads_first_entry: could not retrieve search result\n"));
		goto done;
	}

	if (!ads_pull_sid(ads, msg, "objectSid", sid)) {
		DEBUG(1, ("ad_idmap_get_sid_from_id: ads_pull_sid: could not retrieve SID from entry\n"));
		goto done;
	}

	status = NT_STATUS_OK;
	DEBUG(1, ("ad_idmap_get_sid_from_id mapped POSIX %s %d to SID [%s]\n",
		(id_type == ID_GROUPID) ? "GID" : "UID", (int)unid.uid,
		sid_to_string(sid_string, sid)));

done:
	if (res != NULL) {
		ads_msgfree(ads, res);
	}

	return status;
}

static NTSTATUS ad_idmap_get_id_from_sid(unid_t *unid, int *id_type, const DOM_SID *sid)
{
	ADS_STATUS rc;
	NTSTATUS status = NT_STATUS_NONE_MAPPED;
	const char *attrs[] = { "sAMAccountType", ADS_ATTR_SFU_UIDNUMBER_OID, 
						  ADS_ATTR_SFU_GIDNUMBER_OID, 
						  ADS_ATTR_RFC2307_UIDNUMBER_OID,
						  ADS_ATTR_RFC2307_GIDNUMBER_OID,
						  NULL };
	void *res = NULL;
	void *msg = NULL;
	char *expr = NULL;
	uint32 atype, uid;
	char *sidstr;
	fstring sid_string;
	int count;
	ADS_STRUCT *ads;

	if (unid == NULL) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	ads = ad_idmap_cached_connection();
	if (ads == NULL) {
		DEBUG(1, ("ad_idmap_get_id_from_sid ADS uninitialized\n"));
		return NT_STATUS_NOT_SUPPORTED;
	}

	sidstr = sid_binstring(sid);
	if (asprintf(&expr, "(objectSid=%s)", sidstr) == -1) {
		free(sidstr);
		return NT_STATUS_NO_MEMORY;
	}

	rc = ads_search_retry(ads, &res, expr, attrs);
	free(sidstr);
	free(expr);
	if (!ADS_ERR_OK(rc)) {
		DEBUG(1, ("ad_idmap_get_id_from_sid: ads_search: %s\n", ads_errstr(rc)));
		goto done;
	}

	count = ads_count_replies(ads, res);
	if (count == 0) {
		DEBUG(1, ("ad_idmap_get_id_from_sid: ads_count_replies: no results\n"));
		goto done;
	} else if (count != 1) {
		DEBUG(1, ("ad_idmap_get_id_from_sid: ads_count_replies: incorrect cardinality\n"));
		goto done;
	}

	msg = ads_first_entry(ads, res);
	if (msg == NULL) {
		DEBUG(1, ("ad_idmap_get_id_from_sid: ads_first_entry: could not retrieve search result\n"));
		goto done;
	}

	if (!ads_pull_uint32(ads, msg, "sAMAccountType", &atype)) {
		DEBUG(1, ("ad_idmap_get_id_from_sid: ads_pull_uint32: could not read SAM account type\n"));
		goto done;
	}

	switch (atype & 0xF0000000) {
	case ATYPE_SECURITY_GLOBAL_GROUP:
	case ATYPE_SECURITY_LOCAL_GROUP:
		*id_type = ID_GROUPID;
		break;
	case ATYPE_NORMAL_ACCOUNT:
	case ATYPE_WORKSTATION_TRUST:
	case ATYPE_INTERDOMAIN_TRUST:
		*id_type = ID_USERID;
		break;
	default:
		DEBUG(1, ("ad_idmap_get_id_from_sid: unrecognized SAM account type %08x\n", atype));
		goto done;
		break;
	}

	if (!ads_pull_uint32(ads, msg, (*id_type == ID_GROUPID) ? attr_gidnumber : attr_uidnumber, &uid)) {
		DEBUG(1, ("ad_idmap_get_id_from_sid: ads_pull_uint32: could not read attribute '%s'\n",
			(*id_type == ID_GROUPID) ? attr_gidnumber : attr_uidnumber));
		goto done;
	}

	unid->uid = (uid_t)uid;

	status = NT_STATUS_OK;
	DEBUG(1, ("ad_idmap_get_id_from_sid mapped SID [%s] to POSIX %s %d\n",
		sid_to_string(sid_string, sid),
		(*id_type == ID_GROUPID) ? "GID" : "UID", uid));

done:
	if (res != NULL) {
		ads_msgfree(ads, res);
	}

	return status;

}

static NTSTATUS ad_idmap_set_mapping(const DOM_SID *sid, unid_t id, int id_type)
{
	/* Not supported, and probably won't be... */
	/* (It's not particularly feasible with a single-master model.) */

	return NT_STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS ad_idmap_close(void)
{
	ADS_STRUCT *ads = ad_idmap_ads;

	if (ads != NULL) {
		/* we own this ADS_STRUCT so make sure it goes away */
		ads->is_mine = True;
		ads_destroy( &ads );
		ad_idmap_ads = NULL;
	}

	SAFE_FREE(attr_uidnumber);
	SAFE_FREE(attr_gidnumber);
	
	return NT_STATUS_OK;
}

static NTSTATUS ad_idmap_allocate_id(unid_t *id, int id_type)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

static void ad_idmap_status(void)
{
	DEBUG(0, ("AD IDMAP Status not available\n"));
}

static struct idmap_methods ad_methods = {
	ad_idmap_init,
	ad_idmap_allocate_id,
	ad_idmap_get_sid_from_id,
	ad_idmap_get_id_from_sid,
	ad_idmap_set_mapping,
	ad_idmap_close,
	ad_idmap_status
};


/* support for new authentication subsystem */
NTSTATUS init_module(void)
{
	return smb_register_idmap(SMB_IDMAP_INTERFACE_VERSION, "ad", &ad_methods);
}

