/* 
 *  Unix SMB/Netbios implementation.
 *  SEC_DESC handling functions
 *  Copyright (C) Jeremy R. Allison            1995-2003.
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

/*******************************************************************
 Create the share security tdb.
 ********************************************************************/

static TDB_CONTEXT *share_tdb; /* used for share security descriptors */
#define SHARE_DATABASE_VERSION_V1 1
#define SHARE_DATABASE_VERSION_V2 2 /* version id in little endian. */

/* Map generic permissions to file object specific permissions */

static struct generic_mapping file_generic_mapping = {
        FILE_GENERIC_READ,
        FILE_GENERIC_WRITE,
        FILE_GENERIC_EXECUTE,
        FILE_GENERIC_ALL
};


BOOL share_info_db_init(void)
{
	const char *vstring = "INFO/version";
	int32 vers_id;
 
	if (share_tdb) {
		return True;
	}

	share_tdb = tdb_open_log(lock_path("share_info.tdb"), 0, TDB_DEFAULT, O_RDWR|O_CREAT, 0600);
	if (!share_tdb) {
		DEBUG(0,("Failed to open share info database %s (%s)\n",
			lock_path("share_info.tdb"), strerror(errno) ));
		return False;
	}
 
	/* handle a Samba upgrade */
	tdb_lock_bystring(share_tdb, vstring);

	/* Cope with byte-reversed older versions of the db. */
	vers_id = tdb_fetch_int32(share_tdb, vstring);
	if ((vers_id == SHARE_DATABASE_VERSION_V1) || (IREV(vers_id) == SHARE_DATABASE_VERSION_V1)) {
		/* Written on a bigendian machine with old fetch_int code. Save as le. */
		tdb_store_int32(share_tdb, vstring, SHARE_DATABASE_VERSION_V2);
		vers_id = SHARE_DATABASE_VERSION_V2;
	}

	if (vers_id != SHARE_DATABASE_VERSION_V2) {
		tdb_traverse(share_tdb, tdb_traverse_delete_fn, NULL);
		tdb_store_int32(share_tdb, vstring, SHARE_DATABASE_VERSION_V2);
	}
	tdb_unlock_bystring(share_tdb, vstring);

	return True;
}

/*******************************************************************
 Fake up a Everyone, default access as a default.
 def_access is a GENERIC_XXX access mode.
 ********************************************************************/

SEC_DESC *get_share_security_default( TALLOC_CTX *ctx, size_t *psize, uint32 def_access)
{
	SEC_ACCESS sa;
	SEC_ACE ace;
	SEC_ACL *psa = NULL;
	SEC_DESC *psd = NULL;
	uint32 spec_access = def_access;

	se_map_generic(&spec_access, &file_generic_mapping);

	init_sec_access(&sa, def_access | spec_access );
	init_sec_ace(&ace, &global_sid_World, SEC_ACE_TYPE_ACCESS_ALLOWED, sa, 0);

	if ((psa = make_sec_acl(ctx, NT4_ACL_REVISION, 1, &ace)) != NULL) {
		psd = make_sec_desc(ctx, SEC_DESC_REVISION, SEC_DESC_SELF_RELATIVE, NULL, NULL, NULL, psa, psize);
	}

	if (!psd) {
		DEBUG(0,("get_share_security: Failed to make SEC_DESC.\n"));
		return NULL;
	}

	return psd;
}

/*******************************************************************
 Pull a security descriptor from the share tdb.
 ********************************************************************/

SEC_DESC *get_share_security( TALLOC_CTX *ctx, int snum, size_t *psize)
{
	prs_struct ps;
	fstring key;
	SEC_DESC *psd = NULL;

	if (!share_info_db_init()) {
		return NULL;
	}

	*psize = 0;

	/* Fetch security descriptor from tdb */
 
	slprintf(key, sizeof(key)-1, "SECDESC/%s", lp_servicename(snum));
 
	if (tdb_prs_fetch(share_tdb, key, &ps, ctx)!=0 ||
		!sec_io_desc("get_share_security", &psd, &ps, 1)) {
 
		DEBUG(4,("get_share_security: using default secdesc for %s\n", lp_servicename(snum) ));
 
		return get_share_security_default(ctx, psize, GENERIC_ALL_ACCESS);
	}

	if (psd)
		*psize = sec_desc_size(psd);

	prs_mem_free(&ps);
	return psd;
}

/*******************************************************************
 Store a security descriptor in the share db.
 ********************************************************************/

BOOL set_share_security(TALLOC_CTX *ctx, const char *share_name, SEC_DESC *psd)
{
	prs_struct ps;
	TALLOC_CTX *mem_ctx = NULL;
	fstring key;
	BOOL ret = False;

	if (!share_info_db_init()) {
		return False;
	}

	mem_ctx = talloc_init("set_share_security");
	if (mem_ctx == NULL)
		return False;

	prs_init(&ps, (uint32)sec_desc_size(psd), mem_ctx, MARSHALL);
 
	if (!sec_io_desc("share_security", &psd, &ps, 1))
		goto out;
 
	slprintf(key, sizeof(key)-1, "SECDESC/%s", share_name);
 
	if (tdb_prs_store(share_tdb, key, &ps)==0) {
		ret = True;
		DEBUG(5,("set_share_security: stored secdesc for %s\n", share_name ));
	} else {
		DEBUG(1,("set_share_security: Failed to store secdesc for %s\n", share_name ));
	} 

	/* Free malloc'ed memory */
 
out:
 
	prs_mem_free(&ps);
	if (mem_ctx)
		talloc_destroy(mem_ctx);
	return ret;
}

/*******************************************************************
 Delete a security descriptor.
********************************************************************/

BOOL delete_share_security(int snum)
{
	TDB_DATA kbuf;
	fstring key;

	slprintf(key, sizeof(key)-1, "SECDESC/%s", lp_servicename(snum));
	kbuf.dptr = key;
	kbuf.dsize = strlen(key)+1;

	if (tdb_delete(share_tdb, kbuf) != 0) {
		DEBUG(0,("delete_share_security: Failed to delete entry for share %s\n",
				lp_servicename(snum) ));
		return False;
	}

	return True;
}

/***************************************************************************
 Parse the contents of an acl string from a usershare file.
***************************************************************************/

BOOL parse_usershare_acl(TALLOC_CTX *ctx, const char *acl_str, SEC_DESC **ppsd)
{
	size_t s_size = 0;
	const char *pacl = acl_str;
	int num_aces = 0;
	SEC_ACE *ace_list = NULL;
	SEC_ACL *psa = NULL;
	SEC_DESC *psd = NULL;
	size_t sd_size = 0;
	int i;

	*ppsd = NULL;

	/* If the acl string is blank return "Everyone:R" */
	if (!*acl_str) {
		SEC_DESC *default_psd = get_share_security_default(ctx, &s_size, GENERIC_READ_ACCESS);
		if (!default_psd) {
			return False;
		}
		*ppsd = default_psd;
		return True;
	}

	num_aces = 1;

	/* Add the number of ',' characters to get the number of aces. */
	num_aces += count_chars(pacl,',');

	ace_list = TALLOC_ARRAY(ctx, SEC_ACE, num_aces);
	if (!ace_list) {
		return False;
	}

	for (i = 0; i < num_aces; i++) {
		SEC_ACCESS sa;
		uint32 g_access;
		uint32 s_access;
		DOM_SID sid;
		fstring sidstr;
		uint8 type = SEC_ACE_TYPE_ACCESS_ALLOWED;

		if (!next_token(&pacl, sidstr, ":", sizeof(sidstr))) {
			DEBUG(0,("parse_usershare_acl: malformed usershare acl looking "
				"for ':' in string '%s'\n", pacl));
			return False;
		}

		if (!string_to_sid(&sid, sidstr)) {
			DEBUG(0,("parse_usershare_acl: failed to convert %s to sid.\n",
				sidstr ));
			return False;
		}

		switch (*pacl) {
			case 'F': /* Full Control, ie. R+W */
			case 'f': /* Full Control, ie. R+W */
				s_access = g_access = GENERIC_ALL_ACCESS;
				break;
			case 'R': /* Read only. */
			case 'r': /* Read only. */
				s_access = g_access = GENERIC_READ_ACCESS;
				break;
			case 'D': /* Deny all to this SID. */
			case 'd': /* Deny all to this SID. */
				type = SEC_ACE_TYPE_ACCESS_DENIED;
				s_access = g_access = GENERIC_ALL_ACCESS;
				break;
			default:
				DEBUG(0,("parse_usershare_acl: unknown acl type at %s.\n",
					pacl ));
				return False;
		}

		pacl++;
		if (*pacl && *pacl != ',') {
			DEBUG(0,("parse_usershare_acl: bad acl string at %s.\n",
				pacl ));
			return False;
		}
		pacl++; /* Go past any ',' */

		se_map_generic(&s_access, &file_generic_mapping);
		init_sec_access(&sa, g_access | s_access );
		init_sec_ace(&ace_list[i], &sid, type, sa, 0);
	}

	if ((psa = make_sec_acl(ctx, NT4_ACL_REVISION, num_aces, ace_list)) != NULL) {
		psd = make_sec_desc(ctx, SEC_DESC_REVISION, SEC_DESC_SELF_RELATIVE, NULL, NULL, NULL, psa, &sd_size);
	}

	if (!psd) {
		DEBUG(0,("parse_usershare_acl: Failed to make SEC_DESC.\n"));
		return False;
	}

	*ppsd = psd;
	return True;
}
