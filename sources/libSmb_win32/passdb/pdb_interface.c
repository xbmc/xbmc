/* 
   Unix SMB/CIFS implementation.
   Password and authentication handling
   Copyright (C) Andrew Bartlett			2002
   Copyright (C) Jelmer Vernooij			2002
   Copyright (C) Simo Sorce				2003
   Copyright (C) Volker Lendecke			2006

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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_PASSDB

/* Cache of latest SAM lookup query */

static struct samu *csamuser = NULL;

static_decl_pdb;

static struct pdb_init_function_entry *backends = NULL;

static void lazy_initialize_passdb(void)
{
	static BOOL initialized = False;
	if(initialized) {
		return;
	}
	static_init_pdb;
	initialized = True;
}

static BOOL lookup_global_sam_rid(TALLOC_CTX *mem_ctx, uint32 rid,
				  const char **name,
				  enum SID_NAME_USE *psid_name_use,
				  union unid_t *unix_id);
/*******************************************************************
 Clean up uninitialised passwords.  The only way to tell 
 that these values are not 'real' is that they do not
 have a valid last set time.  Instead, the value is fixed at 0. 
 Therefore we use that as the key for 'is this a valid password'.
 However, it is perfectly valid to have a 'default' last change
 time, such LDAP with a missing attribute would produce.
********************************************************************/

static void pdb_force_pw_initialization(struct samu *pass) 
{
	const uint8 *lm_pwd, *nt_pwd;
	
	/* only reset a password if the last set time has been 
	   explicitly been set to zero.  A default last set time 
	   is ignored */

	if ( (pdb_get_init_flags(pass, PDB_PASSLASTSET) != PDB_DEFAULT) 
		&& (pdb_get_pass_last_set_time(pass) == 0) ) 
	{
		
		if (pdb_get_init_flags(pass, PDB_LMPASSWD) != PDB_DEFAULT) 
		{
			lm_pwd = pdb_get_lanman_passwd(pass);
			if (lm_pwd) 
				pdb_set_lanman_passwd(pass, NULL, PDB_CHANGED);
		}
		if (pdb_get_init_flags(pass, PDB_NTPASSWD) != PDB_DEFAULT) 
		{
			nt_pwd = pdb_get_nt_passwd(pass);
			if (nt_pwd) 
				pdb_set_nt_passwd(pass, NULL, PDB_CHANGED);
		}
	}

	return;
}

NTSTATUS smb_register_passdb(int version, const char *name, pdb_init_function init) 
{
	struct pdb_init_function_entry *entry = backends;

	if(version != PASSDB_INTERFACE_VERSION) {
		DEBUG(0,("Can't register passdb backend!\n"
			 "You tried to register a passdb module with PASSDB_INTERFACE_VERSION %d, "
			 "while this version of samba uses version %d\n", 
			 version,PASSDB_INTERFACE_VERSION));
		return NT_STATUS_OBJECT_TYPE_MISMATCH;
	}

	if (!name || !init) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	DEBUG(5,("Attempting to register passdb backend %s\n", name));

	/* Check for duplicates */
	if (pdb_find_backend_entry(name)) {
		DEBUG(0,("There already is a passdb backend registered with the name %s!\n", name));
		return NT_STATUS_OBJECT_NAME_COLLISION;
	}

	entry = SMB_XMALLOC_P(struct pdb_init_function_entry);
	entry->name = smb_xstrdup(name);
	entry->init = init;

	DLIST_ADD(backends, entry);
	DEBUG(5,("Successfully added passdb backend '%s'\n", name));
	return NT_STATUS_OK;
}

struct pdb_init_function_entry *pdb_find_backend_entry(const char *name)
{
	struct pdb_init_function_entry *entry = backends;

	while(entry) {
		if (strcmp(entry->name, name)==0) return entry;
		entry = entry->next;
	}

	return NULL;
}

/******************************************************************
  Make a pdb_methods from scratch
 *******************************************************************/

NTSTATUS make_pdb_method_name(struct pdb_methods **methods, const char *selected)
{
	char *module_name = smb_xstrdup(selected);
	char *module_location = NULL, *p;
	struct pdb_init_function_entry *entry;
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;

	lazy_initialize_passdb();

	p = strchr(module_name, ':');

	if (p) {
		*p = 0;
		module_location = p+1;
		trim_char(module_location, ' ', ' ');
	}

	trim_char(module_name, ' ', ' ');


	DEBUG(5,("Attempting to find an passdb backend to match %s (%s)\n", selected, module_name));

	entry = pdb_find_backend_entry(module_name);
	
	/* Try to find a module that contains this module */
	if (!entry) { 
		DEBUG(2,("No builtin backend found, trying to load plugin\n"));
		if(NT_STATUS_IS_OK(smb_probe_module("pdb", module_name)) && !(entry = pdb_find_backend_entry(module_name))) {
			DEBUG(0,("Plugin is available, but doesn't register passdb backend %s\n", module_name));
			SAFE_FREE(module_name);
			return NT_STATUS_UNSUCCESSFUL;
		}
	}
	
	/* No such backend found */
	if(!entry) { 
		DEBUG(0,("No builtin nor plugin backend for %s found\n", module_name));
		SAFE_FREE(module_name);
		return NT_STATUS_INVALID_PARAMETER;
	}

	DEBUG(5,("Found pdb backend %s\n", module_name));

	if ( !NT_STATUS_IS_OK( nt_status = entry->init(methods, module_location) ) ) {
		DEBUG(0,("pdb backend %s did not correctly init (error was %s)\n", 
			selected, nt_errstr(nt_status)));
		SAFE_FREE(module_name);
		return nt_status;
	}

	SAFE_FREE(module_name);

	DEBUG(5,("pdb backend %s has a valid init\n", selected));

	return nt_status;
}

/******************************************************************
 Return an already initialised pdn_methods structure
*******************************************************************/

static struct pdb_methods *pdb_get_methods_reload( BOOL reload ) 
{
	static struct pdb_methods *pdb = NULL;

	if ( pdb && reload ) {
		pdb->free_private_data( &(pdb->private_data) );
		if ( !NT_STATUS_IS_OK( make_pdb_method_name( &pdb, lp_passdb_backend() ) ) ) {
			pstring msg;
			slprintf(msg, sizeof(msg)-1, "pdb_get_methods_reload: failed to get pdb methods for backend %s\n",
				lp_passdb_backend() );
			smb_panic(msg);
		}
	}

	if ( !pdb ) {
		if ( !NT_STATUS_IS_OK( make_pdb_method_name( &pdb, lp_passdb_backend() ) ) ) {
			pstring msg;
			slprintf(msg, sizeof(msg)-1, "pdb_get_methods_reload: failed to get pdb methods for backend %s\n",
				lp_passdb_backend() );
			smb_panic(msg);
		}
	}

	return pdb;
}

static struct pdb_methods *pdb_get_methods(void)
{
	return pdb_get_methods_reload(False);
}

/******************************************************************
 Backward compatibility functions for the original passdb interface
*******************************************************************/

BOOL pdb_setsampwent(BOOL update, uint16 acb_mask) 
{
	struct pdb_methods *pdb = pdb_get_methods();
	return NT_STATUS_IS_OK(pdb->setsampwent(pdb, update, acb_mask));
}

void pdb_endsampwent(void) 
{
	struct pdb_methods *pdb = pdb_get_methods();
	pdb->endsampwent(pdb);
}

BOOL pdb_getsampwent(struct samu *user) 
{
	struct pdb_methods *pdb = pdb_get_methods();

	if ( !NT_STATUS_IS_OK(pdb->getsampwent(pdb, user) ) ) {
		return False;
	}
	pdb_force_pw_initialization( user );
	return True;
}

BOOL pdb_getsampwnam(struct samu *sam_acct, const char *username) 
{
	struct pdb_methods *pdb = pdb_get_methods();

	if (!NT_STATUS_IS_OK(pdb->getsampwnam(pdb, sam_acct, username))) {
		return False;
	}

	if ( csamuser ) {
		TALLOC_FREE(csamuser);
	}

	pdb_force_pw_initialization( sam_acct );
	
	csamuser = samu_new( NULL );
	if (!csamuser) {
		return False;
	}

	if (!pdb_copy_sam_account(csamuser, sam_acct)) {
		TALLOC_FREE(csamuser);
		return False;
	}

	return True;
}

/**********************************************************************
**********************************************************************/

BOOL guest_user_info( struct samu *user )
{
	struct passwd *pwd;
	NTSTATUS result;
	const char *guestname = lp_guestaccount();
	
	if ( !(pwd = getpwnam_alloc( NULL, guestname ) ) ) {
		DEBUG(0,("guest_user_info: Unable to locate guest account [%s]!\n", 
			guestname));
		return False;
	}
	
	result = samu_set_unix(user, pwd );

	TALLOC_FREE( pwd );

	return NT_STATUS_IS_OK( result );
}

/**********************************************************************
**********************************************************************/

BOOL pdb_getsampwsid(struct samu *sam_acct, const DOM_SID *sid) 
{
	struct pdb_methods *pdb = pdb_get_methods();
	uint32 rid;

	/* hard code the Guest RID of 501 */

	if ( !sid_peek_check_rid( get_global_sam_sid(), sid, &rid ) )
		return False;

	if ( rid == DOMAIN_USER_RID_GUEST ) {
		DEBUG(6,("pdb_getsampwsid: Building guest account\n"));
		return guest_user_info( sam_acct );
	}
	
	/* check the cache first */
	
	if ( csamuser && sid_equal(sid, pdb_get_user_sid(csamuser) ) )
		return pdb_copy_sam_account(sam_acct, csamuser);

	return NT_STATUS_IS_OK(pdb->getsampwsid(pdb, sam_acct, sid));
}

static NTSTATUS pdb_default_create_user(struct pdb_methods *methods,
					TALLOC_CTX *tmp_ctx, const char *name,
					uint32 acb_info, uint32 *rid)
{
	struct samu *sam_pass;
	NTSTATUS status;
	struct passwd *pwd;

	if ((sam_pass = samu_new(tmp_ctx)) == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	if ( !(pwd = Get_Pwnam_alloc(tmp_ctx, name)) ) {
		pstring add_script;
		int add_ret;
		fstring name2;

		if ((acb_info & ACB_NORMAL) && name[strlen(name)-1] != '$') {
			pstrcpy(add_script, lp_adduser_script());
		} else {
			pstrcpy(add_script, lp_addmachine_script());
		}

		if (add_script[0] == '\0') {
			DEBUG(3, ("Could not find user %s and no add script "
				  "defined\n", name));
			return NT_STATUS_NO_SUCH_USER;
		}

		/* lowercase the username before creating the Unix account for 
		   compatibility with previous Samba releases */
		fstrcpy( name2, name );
		strlower_m( name2 );
		all_string_sub(add_script, "%u", name2, sizeof(add_script));
		add_ret = smbrun(add_script,NULL);
		DEBUG(add_ret ? 0 : 3, ("_samr_create_user: Running the command `%s' gave %d\n",
					add_script, add_ret));
		if (add_ret == 0) {
			smb_nscd_flush_user_cache();
		}
		flush_pwnam_cache();

		pwd = Get_Pwnam_alloc(tmp_ctx, name);
	}

	/* we have a valid SID coming out of this call */

	status = samu_alloc_rid_unix( sam_pass, pwd );

	TALLOC_FREE( pwd );

	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(3, ("pdb_default_create_user: failed to create a new user structure: %s\n", nt_errstr(status)));
		return status;
	}

	if (!sid_peek_check_rid(get_global_sam_sid(),
				pdb_get_user_sid(sam_pass), rid)) {
		DEBUG(0, ("Could not get RID of fresh user\n"));
		return NT_STATUS_INTERNAL_ERROR;
	}

	/* Use the username case specified in the original request */

	pdb_set_username( sam_pass, name, PDB_SET );

	/* Disable the account on creation, it does not have a reasonable password yet. */

	acb_info |= ACB_DISABLED;

	pdb_set_acct_ctrl(sam_pass, acb_info, PDB_CHANGED);

	status = pdb_add_sam_account(sam_pass);

	TALLOC_FREE(sam_pass);

	return status;
}

NTSTATUS pdb_create_user(TALLOC_CTX *mem_ctx, const char *name, uint32 flags,
			 uint32 *rid)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->create_user(pdb, mem_ctx, name, flags, rid);
}

/****************************************************************************
 Delete a UNIX user on demand.
****************************************************************************/

static int smb_delete_user(const char *unix_user)
{
	pstring del_script;
	int ret;

	/* safety check */

	if ( strequal( unix_user, "root" ) ) {
		DEBUG(0,("smb_delete_user: Refusing to delete local system root account!\n"));
		return -1;
	}

	pstrcpy(del_script, lp_deluser_script());
	if (! *del_script)
		return -1;
	all_string_sub(del_script, "%u", unix_user, sizeof(del_script));
	ret = smbrun(del_script,NULL);
	flush_pwnam_cache();
	if (ret == 0) {
		smb_nscd_flush_user_cache();
	}
	DEBUG(ret ? 0 : 3,("smb_delete_user: Running the command `%s' gave %d\n",del_script,ret));

	return ret;
}

static NTSTATUS pdb_default_delete_user(struct pdb_methods *methods,
					TALLOC_CTX *mem_ctx,
					struct samu *sam_acct)
{
	NTSTATUS status;
	fstring username;

	status = pdb_delete_sam_account(sam_acct);
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}

	/*
	 * Now delete the unix side ....
	 * note: we don't check if the delete really happened as the script is
	 * not necessary present and maybe the sysadmin doesn't want to delete
	 * the unix side
	 */

	/* always lower case the username before handing it off to 
	   external scripts */

	fstrcpy( username, pdb_get_username(sam_acct) );
	strlower_m( username );

	smb_delete_user( username );
	
	return status;
}

NTSTATUS pdb_delete_user(TALLOC_CTX *mem_ctx, struct samu *sam_acct)
{
	struct pdb_methods *pdb = pdb_get_methods();
	uid_t uid = -1;

	/* sanity check to make sure we don't delete root */

	if ( !sid_to_uid( pdb_get_user_sid(sam_acct), &uid ) ) {
		return NT_STATUS_NO_SUCH_USER;
	}

	if ( uid == 0 ) {
		return NT_STATUS_ACCESS_DENIED;
	}

	return pdb->delete_user(pdb, mem_ctx, sam_acct);
}

NTSTATUS pdb_add_sam_account(struct samu *sam_acct) 
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->add_sam_account(pdb, sam_acct);
}

NTSTATUS pdb_update_sam_account(struct samu *sam_acct) 
{
	struct pdb_methods *pdb = pdb_get_methods();

	if (csamuser != NULL) {
		TALLOC_FREE(csamuser);
		csamuser = NULL;
	}

	return pdb->update_sam_account(pdb, sam_acct);
}

NTSTATUS pdb_delete_sam_account(struct samu *sam_acct) 
{
	struct pdb_methods *pdb = pdb_get_methods();

	if (csamuser != NULL) {
		TALLOC_FREE(csamuser);
		csamuser = NULL;
	}

	return pdb->delete_sam_account(pdb, sam_acct);
}

NTSTATUS pdb_rename_sam_account(struct samu *oldname, const char *newname)
{
	struct pdb_methods *pdb = pdb_get_methods();
	uid_t uid;
	NTSTATUS status;

	if (csamuser != NULL) {
		TALLOC_FREE(csamuser);
		csamuser = NULL;
	}

	/* sanity check to make sure we don't rename root */

	if ( !sid_to_uid( pdb_get_user_sid(oldname), &uid ) ) {
		return NT_STATUS_NO_SUCH_USER;
	}

	if ( uid == 0 ) {
		return NT_STATUS_ACCESS_DENIED;
	}

	status = pdb->rename_sam_account(pdb, oldname, newname);

	/* always flush the cache here just to be safe */
	flush_pwnam_cache();

	return status;
}

NTSTATUS pdb_update_login_attempts(struct samu *sam_acct, BOOL success)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->update_login_attempts(pdb, sam_acct, success);
}

BOOL pdb_getgrsid(GROUP_MAP *map, DOM_SID sid)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return NT_STATUS_IS_OK(pdb->getgrsid(pdb, map, sid));
}

BOOL pdb_getgrgid(GROUP_MAP *map, gid_t gid)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return NT_STATUS_IS_OK(pdb->getgrgid(pdb, map, gid));
}

BOOL pdb_getgrnam(GROUP_MAP *map, const char *name)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return NT_STATUS_IS_OK(pdb->getgrnam(pdb, map, name));
}

static NTSTATUS pdb_default_create_dom_group(struct pdb_methods *methods,
					     TALLOC_CTX *mem_ctx,
					     const char *name,
					     uint32 *rid)
{
	DOM_SID group_sid;
	struct group *grp;

	grp = getgrnam(name);

	if (grp == NULL) {
		gid_t gid;

		if (smb_create_group(name, &gid) != 0) {
			return NT_STATUS_ACCESS_DENIED;
		}

		grp = getgrgid(gid);
	}

	if (grp == NULL) {
		return NT_STATUS_ACCESS_DENIED;
	}

	if (pdb_rid_algorithm()) {
		*rid = algorithmic_pdb_gid_to_group_rid( grp->gr_gid );
	} else {
		if (!pdb_new_rid(rid)) {
			return NT_STATUS_ACCESS_DENIED;
		}
	}

	sid_compose(&group_sid, get_global_sam_sid(), *rid);
		
	return add_initial_entry(grp->gr_gid, sid_string_static(&group_sid),
				 SID_NAME_DOM_GRP, name, NULL);
}

NTSTATUS pdb_create_dom_group(TALLOC_CTX *mem_ctx, const char *name,
			      uint32 *rid)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->create_dom_group(pdb, mem_ctx, name, rid);
}

static NTSTATUS pdb_default_delete_dom_group(struct pdb_methods *methods,
					     TALLOC_CTX *mem_ctx,
					     uint32 rid)
{
	DOM_SID group_sid;
	GROUP_MAP map;
	NTSTATUS status;
	struct group *grp;
	const char *grp_name;

	sid_compose(&group_sid, get_global_sam_sid(), rid);

	if (!get_domain_group_from_sid(group_sid, &map)) {
		DEBUG(10, ("Could not find group for rid %d\n", rid));
		return NT_STATUS_NO_SUCH_GROUP;
	}

	/* We need the group name for the smb_delete_group later on */

	if (map.gid == (gid_t)-1) {
		return NT_STATUS_NO_SUCH_GROUP;
	}

	grp = getgrgid(map.gid);
	if (grp == NULL) {
		return NT_STATUS_NO_SUCH_GROUP;
	}

	/* Copy the name, no idea what pdb_delete_group_mapping_entry does.. */

	grp_name = talloc_strdup(mem_ctx, grp->gr_name);
	if (grp_name == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	status = pdb_delete_group_mapping_entry(group_sid);

	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}

	/* Don't check the result of smb_delete_group */
	
	smb_delete_group(grp_name);

	return NT_STATUS_OK;
}

NTSTATUS pdb_delete_dom_group(TALLOC_CTX *mem_ctx, uint32 rid)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->delete_dom_group(pdb, mem_ctx, rid);
}

NTSTATUS pdb_add_group_mapping_entry(GROUP_MAP *map)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->add_group_mapping_entry(pdb, map);
}

NTSTATUS pdb_update_group_mapping_entry(GROUP_MAP *map)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->update_group_mapping_entry(pdb, map);
}

NTSTATUS pdb_delete_group_mapping_entry(DOM_SID sid)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->delete_group_mapping_entry(pdb, sid);
}

BOOL pdb_enum_group_mapping(const DOM_SID *sid, enum SID_NAME_USE sid_name_use, GROUP_MAP **pp_rmap,
			    size_t *p_num_entries, BOOL unix_only)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return NT_STATUS_IS_OK(pdb-> enum_group_mapping(pdb, sid, sid_name_use,
		pp_rmap, p_num_entries, unix_only));
}

NTSTATUS pdb_enum_group_members(TALLOC_CTX *mem_ctx,
				const DOM_SID *sid,
				uint32 **pp_member_rids,
				size_t *p_num_members)
{
	struct pdb_methods *pdb = pdb_get_methods();
	NTSTATUS result;

	result = pdb->enum_group_members(pdb, mem_ctx, 
			sid, pp_member_rids, p_num_members);
		
	/* special check for rid 513 */
		
	if ( !NT_STATUS_IS_OK( result ) ) {
		uint32 rid;
		
		sid_peek_rid( sid, &rid );
		
		if ( rid == DOMAIN_GROUP_RID_USERS ) {
			*p_num_members = 0;
			*pp_member_rids = NULL;
			
			return NT_STATUS_OK;
		}
	}
	
	return result;
}

NTSTATUS pdb_enum_group_memberships(TALLOC_CTX *mem_ctx, struct samu *user,
				    DOM_SID **pp_sids, gid_t **pp_gids,
				    size_t *p_num_groups)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->enum_group_memberships(
		pdb, mem_ctx, user,
		pp_sids, pp_gids, p_num_groups);
}

static NTSTATUS pdb_default_set_unix_primary_group(struct pdb_methods *methods,
						   TALLOC_CTX *mem_ctx,
						   struct samu *sampass)
{
	struct group *grp;
	gid_t gid;

	if (!sid_to_gid(pdb_get_group_sid(sampass), &gid) ||
	    (grp = getgrgid(gid)) == NULL) {
		return NT_STATUS_INVALID_PRIMARY_GROUP;
	}

	if (smb_set_primary_group(grp->gr_name,
				  pdb_get_username(sampass)) != 0) {
		return NT_STATUS_ACCESS_DENIED;
	}

	return NT_STATUS_OK;
}

NTSTATUS pdb_set_unix_primary_group(TALLOC_CTX *mem_ctx, struct samu *user)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->set_unix_primary_group(pdb, mem_ctx, user);
}

/*
 * Helper function to see whether a user is in a group. We can't use
 * user_in_group_sid here because this creates dependencies only smbd can
 * fulfil.
 */

static BOOL pdb_user_in_group(TALLOC_CTX *mem_ctx, struct samu *account,
			      const DOM_SID *group_sid)
{
	DOM_SID *sids;
	gid_t *gids;
	size_t i, num_groups;

	if (!NT_STATUS_IS_OK(pdb_enum_group_memberships(mem_ctx, account,
							&sids, &gids,
							&num_groups))) {
		return False;
	}

	for (i=0; i<num_groups; i++) {
		if (sid_equal(group_sid, &sids[i])) {
			return True;
		}
	}
	return False;
}

static NTSTATUS pdb_default_add_groupmem(struct pdb_methods *methods,
					 TALLOC_CTX *mem_ctx,
					 uint32 group_rid,
					 uint32 member_rid)
{
	DOM_SID group_sid, member_sid;
	struct samu *account = NULL;
	GROUP_MAP map;
	struct group *grp;
	struct passwd *pwd;
	const char *group_name;
	uid_t uid;

	sid_compose(&group_sid, get_global_sam_sid(), group_rid);
	sid_compose(&member_sid, get_global_sam_sid(), member_rid);

	if (!get_domain_group_from_sid(group_sid, &map) ||
	    (map.gid == (gid_t)-1) ||
	    ((grp = getgrgid(map.gid)) == NULL)) {
		return NT_STATUS_NO_SUCH_GROUP;
	}

	group_name = talloc_strdup(mem_ctx, grp->gr_name);
	if (group_name == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	if ( !(account = samu_new( NULL )) ) {
		return NT_STATUS_NO_MEMORY;
	}

	if (!pdb_getsampwsid(account, &member_sid) ||
	    !sid_to_uid(&member_sid, &uid) ||
	    ((pwd = getpwuid_alloc(mem_ctx, uid)) == NULL)) {
		return NT_STATUS_NO_SUCH_USER;
	}

	if (pdb_user_in_group(mem_ctx, account, &group_sid)) {
		return NT_STATUS_MEMBER_IN_GROUP;
	}

	/* 
	 * ok, the group exist, the user exist, the user is not in the group,
	 * we can (finally) add it to the group !
	 */

	smb_add_user_group(group_name, pwd->pw_name);

	if (!pdb_user_in_group(mem_ctx, account, &group_sid)) {
		return NT_STATUS_ACCESS_DENIED;
	}

	return NT_STATUS_OK;
}

NTSTATUS pdb_add_groupmem(TALLOC_CTX *mem_ctx, uint32 group_rid,
			  uint32 member_rid)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->add_groupmem(pdb, mem_ctx, group_rid, member_rid);
}

static NTSTATUS pdb_default_del_groupmem(struct pdb_methods *methods,
					 TALLOC_CTX *mem_ctx,
					 uint32 group_rid,
					 uint32 member_rid)
{
	DOM_SID group_sid, member_sid;
	struct samu *account = NULL;
	GROUP_MAP map;
	struct group *grp;
	struct passwd *pwd;
	const char *group_name;
	uid_t uid;

	sid_compose(&group_sid, get_global_sam_sid(), group_rid);
	sid_compose(&member_sid, get_global_sam_sid(), member_rid);

	if (!get_domain_group_from_sid(group_sid, &map) ||
	    (map.gid == (gid_t)-1) ||
	    ((grp = getgrgid(map.gid)) == NULL)) {
		return NT_STATUS_NO_SUCH_GROUP;
	}

	group_name = talloc_strdup(mem_ctx, grp->gr_name);
	if (group_name == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	if ( !(account = samu_new( NULL )) ) {
		return NT_STATUS_NO_MEMORY;
	}

	if (!pdb_getsampwsid(account, &member_sid) ||
	    !sid_to_uid(&member_sid, &uid) ||
	    ((pwd = getpwuid_alloc(mem_ctx, uid)) == NULL)) {
		return NT_STATUS_NO_SUCH_USER;
	}

	if (!pdb_user_in_group(mem_ctx, account, &group_sid)) {
		return NT_STATUS_MEMBER_NOT_IN_GROUP;
	}

	/* 
	 * ok, the group exist, the user exist, the user is in the group,
	 * we can (finally) delete it from the group!
	 */

	smb_delete_user_group(group_name, pwd->pw_name);

	if (pdb_user_in_group(mem_ctx, account, &group_sid)) {
		return NT_STATUS_ACCESS_DENIED;
	}

	return NT_STATUS_OK;
}

NTSTATUS pdb_del_groupmem(TALLOC_CTX *mem_ctx, uint32 group_rid,
			  uint32 member_rid)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->del_groupmem(pdb, mem_ctx, group_rid, member_rid);
}

BOOL pdb_find_alias(const char *name, DOM_SID *sid)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return NT_STATUS_IS_OK(pdb->find_alias(pdb, name, sid));
}

NTSTATUS pdb_create_alias(const char *name, uint32 *rid)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->create_alias(pdb, name, rid);
}

BOOL pdb_delete_alias(const DOM_SID *sid)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return NT_STATUS_IS_OK(pdb->delete_alias(pdb, sid));
							    
}

BOOL pdb_get_aliasinfo(const DOM_SID *sid, struct acct_info *info)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return NT_STATUS_IS_OK(pdb->get_aliasinfo(pdb, sid, info));
}

BOOL pdb_set_aliasinfo(const DOM_SID *sid, struct acct_info *info)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return NT_STATUS_IS_OK(pdb->set_aliasinfo(pdb, sid, info));
}

NTSTATUS pdb_add_aliasmem(const DOM_SID *alias, const DOM_SID *member)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->add_aliasmem(pdb, alias, member);
}

NTSTATUS pdb_del_aliasmem(const DOM_SID *alias, const DOM_SID *member)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->del_aliasmem(pdb, alias, member);
}

NTSTATUS pdb_enum_aliasmem(const DOM_SID *alias,
			   DOM_SID **pp_members, size_t *p_num_members)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->enum_aliasmem(pdb, alias, pp_members, p_num_members);
}

NTSTATUS pdb_enum_alias_memberships(TALLOC_CTX *mem_ctx,
				    const DOM_SID *domain_sid,
				    const DOM_SID *members, size_t num_members,
				    uint32 **pp_alias_rids,
				    size_t *p_num_alias_rids)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->enum_alias_memberships(pdb, mem_ctx,
						       domain_sid,
						       members, num_members,
						       pp_alias_rids,
						       p_num_alias_rids);
}

NTSTATUS pdb_lookup_rids(const DOM_SID *domain_sid,
			 int num_rids,
			 uint32 *rids,
			 const char **names,
			 uint32 *attrs)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->lookup_rids(pdb, domain_sid,
					    num_rids, rids, names, attrs);
}

NTSTATUS pdb_lookup_names(const DOM_SID *domain_sid,
			  int num_names,
			  const char **names,
			  uint32 *rids,
			  uint32 *attrs)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->lookup_names(pdb, domain_sid,
					     num_names, names, rids, attrs);
}

BOOL pdb_get_account_policy(int policy_index, uint32 *value)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return NT_STATUS_IS_OK(pdb->get_account_policy(pdb, policy_index, value));
}

BOOL pdb_set_account_policy(int policy_index, uint32 value)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return NT_STATUS_IS_OK(pdb->set_account_policy(pdb, policy_index, value));
}

BOOL pdb_get_seq_num(time_t *seq_num)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return NT_STATUS_IS_OK(pdb->get_seq_num(pdb, seq_num));
}

BOOL pdb_uid_to_rid(uid_t uid, uint32 *rid)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->uid_to_rid(pdb, uid, rid);
}

BOOL pdb_gid_to_sid(gid_t gid, DOM_SID *sid)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->gid_to_sid(pdb, gid, sid);
}

BOOL pdb_sid_to_id(const DOM_SID *sid, union unid_t *id,
		   enum SID_NAME_USE *type)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->sid_to_id(pdb, sid, id, type);
}

BOOL pdb_rid_algorithm(void)
{
	struct pdb_methods *pdb = pdb_get_methods();
	return pdb->rid_algorithm(pdb);
}

/********************************************************************
 Allocate a new RID from the passdb backend.  Verify that it is free
 by calling lookup_global_sam_rid() to verify that the RID is not
 in use.  This handles servers that have existing users or groups
 with add RIDs (assigned from previous algorithmic mappings)
********************************************************************/

BOOL pdb_new_rid(uint32 *rid)
{
	struct pdb_methods *pdb = pdb_get_methods();
	const char *name = NULL;
	enum SID_NAME_USE type;
	uint32 allocated_rid = 0;
	int i;
	TALLOC_CTX *ctx;

	if (pdb_rid_algorithm()) {
		DEBUG(0, ("Trying to allocate a RID when algorithmic RIDs "
			  "are active\n"));
		return False;
	}

	if (algorithmic_rid_base() != BASE_RID) {
		DEBUG(0, ("'algorithmic rid base' is set but a passdb backend "
			  "without algorithmic RIDs is chosen.\n"));
		DEBUGADD(0, ("Please map all used groups using 'net groupmap "
			     "add', set the maximum used RID using\n"));
		DEBUGADD(0, ("'net setmaxrid' and remove the parameter\n"));
		return False;
	}

	if ( (ctx = talloc_init("pdb_new_rid")) == NULL ) {
		DEBUG(0,("pdb_new_rid: Talloc initialization failure\n"));
		return False;
	}

	/* Attempt to get an unused RID (max tires is 250...yes that it is 
	   and arbitrary number I pulkled out of my head).   -- jerry */

	for ( i=0; allocated_rid==0 && i<250; i++ ) {
		/* get a new RID */

		if ( !pdb->new_rid(pdb, &allocated_rid) ) {
			return False;
		}

		/* validate that the RID is not in use */

		if ( lookup_global_sam_rid( ctx, allocated_rid, &name, &type, NULL ) ) {
			allocated_rid = 0;
		}
	}

	TALLOC_FREE( ctx );

	if ( allocated_rid == 0 ) {
		DEBUG(0,("pdb_new_rid: Failed to find unused RID\n"));
		return False;
	}

	*rid = allocated_rid;

	return True;
}

/***************************************************************
  Initialize the static context (at smbd startup etc). 

  If uninitialised, context will auto-init on first use.
 ***************************************************************/

BOOL initialize_password_db(BOOL reload)
{	
	return (pdb_get_methods_reload(reload) != NULL);
}


/***************************************************************************
  Default implementations of some functions.
 ****************************************************************************/

static NTSTATUS pdb_default_getsampwnam (struct pdb_methods *methods, struct samu *user, const char *sname)
{
	return NT_STATUS_NO_SUCH_USER;
}

static NTSTATUS pdb_default_getsampwsid(struct pdb_methods *my_methods, struct samu * user, const DOM_SID *sid)
{
	return NT_STATUS_NO_SUCH_USER;
}

static NTSTATUS pdb_default_add_sam_account (struct pdb_methods *methods, struct samu *newpwd)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS pdb_default_update_sam_account (struct pdb_methods *methods, struct samu *newpwd)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS pdb_default_delete_sam_account (struct pdb_methods *methods, struct samu *pwd)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS pdb_default_rename_sam_account (struct pdb_methods *methods, struct samu *pwd, const char *newname)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS pdb_default_update_login_attempts (struct pdb_methods *methods, struct samu *newpwd, BOOL success)
{
	return NT_STATUS_OK;
}

static NTSTATUS pdb_default_setsampwent(struct pdb_methods *methods, BOOL update, uint32 acb_mask)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS pdb_default_getsampwent(struct pdb_methods *methods, struct samu *user)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

static void pdb_default_endsampwent(struct pdb_methods *methods)
{
	return; /* NT_STATUS_NOT_IMPLEMENTED; */
}

static NTSTATUS pdb_default_get_account_policy(struct pdb_methods *methods, int policy_index, uint32 *value)
{
	return account_policy_get(policy_index, value) ? NT_STATUS_OK : NT_STATUS_UNSUCCESSFUL;
}

static NTSTATUS pdb_default_set_account_policy(struct pdb_methods *methods, int policy_index, uint32 value)
{
	return account_policy_set(policy_index, value) ? NT_STATUS_OK : NT_STATUS_UNSUCCESSFUL;
}

static NTSTATUS pdb_default_get_seq_num(struct pdb_methods *methods, time_t *seq_num)
{
	*seq_num = time(NULL);
	return NT_STATUS_OK;
}

static BOOL pdb_default_uid_to_rid(struct pdb_methods *methods, uid_t uid,
				   uint32 *rid)
{
	struct samu *sampw = NULL;
	struct passwd *unix_pw;
	BOOL ret;
	
	unix_pw = sys_getpwuid( uid );

	if ( !unix_pw ) {
		DEBUG(4,("pdb_default_uid_to_rid: host has no idea of uid "
			 "%lu\n", (unsigned long)uid));
		return False;
	}
	
	if ( !(sampw = samu_new( NULL )) ) {
		DEBUG(0,("pdb_default_uid_to_rid: samu_new() failed!\n"));
		return False;
	}

	become_root();
	ret = NT_STATUS_IS_OK(
		methods->getsampwnam(methods, sampw, unix_pw->pw_name ));
	unbecome_root();

	if (!ret) {
		DEBUG(5, ("pdb_default_uid_to_rid: Did not find user "
			  "%s (%d)\n", unix_pw->pw_name, uid));
		TALLOC_FREE(sampw);
		return False;
	}

	ret = sid_peek_check_rid(get_global_sam_sid(),
				 pdb_get_user_sid(sampw), rid);

	if (!ret) {
		DEBUG(1, ("Could not peek rid out of sid %s\n",
			  sid_string_static(pdb_get_user_sid(sampw))));
	}

	TALLOC_FREE(sampw);
	return ret;
}

static BOOL pdb_default_gid_to_sid(struct pdb_methods *methods, gid_t gid,
				   DOM_SID *sid)
{
	GROUP_MAP map;

	if (!NT_STATUS_IS_OK(methods->getgrgid(methods, &map, gid))) {
		return False;
	}

	sid_copy(sid, &map.sid);
	return True;
}

static BOOL pdb_default_sid_to_id(struct pdb_methods *methods,
				  const DOM_SID *sid,
				  union unid_t *id, enum SID_NAME_USE *type)
{
	TALLOC_CTX *mem_ctx;
	BOOL ret = False;
	const char *name;
	uint32 rid;

	mem_ctx = talloc_new(NULL);

	if (mem_ctx == NULL) {
		DEBUG(0, ("talloc_new failed\n"));
		return False;
	}

	if (sid_peek_check_rid(get_global_sam_sid(), sid, &rid)) {
		/* Here we might have users as well as groups and aliases */
		ret = lookup_global_sam_rid(mem_ctx, rid, &name, type, id);
		goto done;
	}

	if (sid_peek_check_rid(&global_sid_Builtin, sid, &rid)) {
		/* Here we only have aliases */
		GROUP_MAP map;
		if (!NT_STATUS_IS_OK(methods->getgrsid(methods, &map, *sid))) {
			DEBUG(10, ("Could not find map for sid %s\n",
				   sid_string_static(sid)));
			goto done;
		}
		if ((map.sid_name_use != SID_NAME_ALIAS) &&
		    (map.sid_name_use != SID_NAME_WKN_GRP)) {
			DEBUG(10, ("Map for sid %s is a %s, expected an "
				   "alias\n", sid_string_static(sid),
				   sid_type_lookup(map.sid_name_use)));
			goto done;
		}

		id->gid = map.gid;
		*type = SID_NAME_ALIAS;
		ret = True;
		goto done;
	}

	DEBUG(5, ("Sid %s is neither ours nor builtin, don't know it\n",
		  sid_string_static(sid)));

 done:

	TALLOC_FREE(mem_ctx);
	return ret;
}

static void add_uid_to_array_unique(TALLOC_CTX *mem_ctx,
				    uid_t uid, uid_t **pp_uids, size_t *p_num)
{
	size_t i;

	for (i=0; i<*p_num; i++) {
		if ((*pp_uids)[i] == uid)
			return;
	}
	
	*pp_uids = TALLOC_REALLOC_ARRAY(mem_ctx, *pp_uids, uid_t, *p_num+1);

	if (*pp_uids == NULL)
		return;

	(*pp_uids)[*p_num] = uid;
	*p_num += 1;
}

static BOOL get_memberuids(TALLOC_CTX *mem_ctx, gid_t gid, uid_t **pp_uids, size_t *p_num)
{
	struct group *grp;
	char **gr;
	struct passwd *pwd;
	BOOL winbind_env;
 
	*pp_uids = NULL;
	*p_num = 0;

	/* We only look at our own sam, so don't care about imported stuff */
	winbind_env = winbind_env_set();
	winbind_off();

	if ((grp = getgrgid(gid)) == NULL) {
		/* allow winbindd lookups, but only if they weren't already disabled */
		if (!winbind_env) {
			winbind_on();
		}

		return False;
	}

	/* Primary group members */

	setpwent();
	while ((pwd = getpwent()) != NULL) {
		if (pwd->pw_gid == gid) {
			add_uid_to_array_unique(mem_ctx, pwd->pw_uid,
						pp_uids, p_num);
		}
	}
	endpwent();

	/* Secondary group members */

	for (gr = grp->gr_mem; (*gr != NULL) && ((*gr)[0] != '\0'); gr += 1) {
		struct passwd *pw = getpwnam(*gr);

		if (pw == NULL)
			continue;
		add_uid_to_array_unique(mem_ctx, pw->pw_uid, pp_uids, p_num);
	}

	/* allow winbindd lookups, but only if they weren't already disabled */
	if (!winbind_env) {
		winbind_on();
	}

	return True;
}

NTSTATUS pdb_default_enum_group_members(struct pdb_methods *methods,
					TALLOC_CTX *mem_ctx,
					const DOM_SID *group,
					uint32 **pp_member_rids,
					size_t *p_num_members)
{
	gid_t gid;
	uid_t *uids;
	size_t i, num_uids;

	*pp_member_rids = NULL;
	*p_num_members = 0;

	if (!sid_to_gid(group, &gid))
		return NT_STATUS_NO_SUCH_GROUP;

	if(!get_memberuids(mem_ctx, gid, &uids, &num_uids))
		return NT_STATUS_NO_SUCH_GROUP;

	if (num_uids == 0)
		return NT_STATUS_OK;

	*pp_member_rids = TALLOC_ZERO_ARRAY(mem_ctx, uint32, num_uids);

	for (i=0; i<num_uids; i++) {
		DOM_SID sid;

		uid_to_sid(&sid, uids[i]);

		if (!sid_check_is_in_our_domain(&sid)) {
			DEBUG(5, ("Inconsistent SAM -- group member uid not "
				  "in our domain\n"));
			continue;
		}

		sid_peek_rid(&sid, &(*pp_member_rids)[*p_num_members]);
		*p_num_members += 1;
	}

	return NT_STATUS_OK;
}

NTSTATUS pdb_default_enum_group_memberships(struct pdb_methods *methods,
					    TALLOC_CTX *mem_ctx,
					    struct samu *user,
					    DOM_SID **pp_sids,
					    gid_t **pp_gids,
					    size_t *p_num_groups)
{
	size_t i;
	gid_t gid;
	struct passwd *pw;
	const char *username = pdb_get_username(user);
	

	/* Ignore the primary group SID.  Honor the real Unix primary group.
	   The primary group SID is only of real use to Windows clients */
	   
	if ( !(pw = getpwnam_alloc(mem_ctx, username)) ) {
		return NT_STATUS_NO_SUCH_USER;
	}
	
	gid = pw->pw_gid;
	
	TALLOC_FREE( pw );

	if (!getgroups_unix_user(mem_ctx, username, gid, pp_gids, p_num_groups)) {
		return NT_STATUS_NO_SUCH_USER;
	}

	if (*p_num_groups == 0) {
		smb_panic("primary group missing");
	}

	*pp_sids = TALLOC_ARRAY(mem_ctx, DOM_SID, *p_num_groups);

	if (*pp_sids == NULL) {
		TALLOC_FREE(*pp_gids);
		return NT_STATUS_NO_MEMORY;
	}

	for (i=0; i<*p_num_groups; i++) {
		gid_to_sid(&(*pp_sids)[i], (*pp_gids)[i]);
	}

	return NT_STATUS_OK;
}

/*******************************************************************
 Look up a rid in the SAM we're responsible for (i.e. passdb)
 ********************************************************************/

static BOOL lookup_global_sam_rid(TALLOC_CTX *mem_ctx, uint32 rid,
				  const char **name,
				  enum SID_NAME_USE *psid_name_use,
				  union unid_t *unix_id)
{
	struct samu *sam_account = NULL;
	GROUP_MAP map;
	BOOL ret;
	DOM_SID sid;

	*psid_name_use = SID_NAME_UNKNOWN;
	
	DEBUG(5,("lookup_global_sam_rid: looking up RID %u.\n",
		 (unsigned int)rid));

	sid_copy(&sid, get_global_sam_sid());
	sid_append_rid(&sid, rid);
	
	/* see if the passdb can help us with the name of the user */

	if ( !(sam_account = samu_new( NULL )) ) {
		return False;
	}

	/* BEING ROOT BLLOCK */
	become_root();
	if (pdb_getsampwsid(sam_account, &sid)) {
		struct passwd *pw;

		unbecome_root();		/* -----> EXIT BECOME_ROOT() */
		*name = talloc_strdup(mem_ctx, pdb_get_username(sam_account));
		if (!*name) {
			TALLOC_FREE(sam_account);
			return False;
		}

		*psid_name_use = SID_NAME_USER;

		TALLOC_FREE(sam_account);

		if (unix_id == NULL) {
			return True;
		}

		pw = Get_Pwnam(*name);
		if (pw == NULL) {
			return False;
		}
		unix_id->uid = pw->pw_uid;
		return True;
	}
	TALLOC_FREE(sam_account);
	
	ret = pdb_getgrsid(&map, sid);
	unbecome_root();
	/* END BECOME_ROOT BLOCK */
  
	/* do not resolve SIDs to a name unless there is a valid 
	   gid associated with it */
		   
	if ( ret && (map.gid != (gid_t)-1) ) {
		*name = talloc_strdup(mem_ctx, map.nt_name);
		*psid_name_use = map.sid_name_use;

		if ( unix_id ) {
			unix_id->gid = map.gid;
		}

		return True;
	}
	
	/* Windows will always map RID 513 to something.  On a non-domain 
	   controller, this gets mapped to SERVER\None. */

	if ( unix_id ) {
		DEBUG(5, ("Can't find a unix id for an unmapped group\n"));
		return False;
	}
	
	if ( rid == DOMAIN_GROUP_RID_USERS ) {
		*name = talloc_strdup(mem_ctx, "None" );
		*psid_name_use = SID_NAME_DOM_GRP;
		
		return True;
	}

	return False;
}

NTSTATUS pdb_default_lookup_rids(struct pdb_methods *methods,
				 const DOM_SID *domain_sid,
				 int num_rids,
				 uint32 *rids,
				 const char **names,
				 uint32 *attrs)
{
	int i;
	NTSTATUS result;
	BOOL have_mapped = False;
	BOOL have_unmapped = False;

	if (sid_check_is_builtin(domain_sid)) {

		for (i=0; i<num_rids; i++) {
			const char *name;

			if (lookup_builtin_rid(names, rids[i], &name)) {
				attrs[i] = SID_NAME_ALIAS;
				names[i] = name;
				DEBUG(5,("lookup_rids: %s:%d\n",
					 names[i], attrs[i]));
				have_mapped = True;
			} else {
				have_unmapped = True;
				attrs[i] = SID_NAME_UNKNOWN;
			}
		}
		goto done;
	}

	/* Should not happen, but better check once too many */
	if (!sid_check_is_domain(domain_sid)) {
		return NT_STATUS_INVALID_HANDLE;
	}

	for (i = 0; i < num_rids; i++) {
		const char *name;

		if (lookup_global_sam_rid(names, rids[i], &name, &attrs[i],
					  NULL)) {
			if (name == NULL) {
				return NT_STATUS_NO_MEMORY;
			}
			names[i] = name;
			DEBUG(5,("lookup_rids: %s:%d\n", names[i], attrs[i]));
			have_mapped = True;
		} else {
			have_unmapped = True;
			attrs[i] = SID_NAME_UNKNOWN;
		}
	}

 done:

	result = NT_STATUS_NONE_MAPPED;

	if (have_mapped)
		result = have_unmapped ? STATUS_SOME_UNMAPPED : NT_STATUS_OK;

	return result;
}

NTSTATUS pdb_default_lookup_names(struct pdb_methods *methods,
				  const DOM_SID *domain_sid,
				  int num_names,
				  const char **names,
				  uint32 *rids,
				  uint32 *attrs)
{
	int i;
	NTSTATUS result;
	BOOL have_mapped = False;
	BOOL have_unmapped = False;

	if (sid_check_is_builtin(domain_sid)) {

		for (i=0; i<num_names; i++) {
			uint32 rid;

			if (lookup_builtin_name(names[i], &rid)) {
				attrs[i] = SID_NAME_ALIAS;
				rids[i] = rid;
				DEBUG(5,("lookup_rids: %s:%d\n",
					 names[i], attrs[i]));
				have_mapped = True;
			} else {
				have_unmapped = True;
				attrs[i] = SID_NAME_UNKNOWN;
			}
		}
		goto done;
	}

	/* Should not happen, but better check once too many */
	if (!sid_check_is_domain(domain_sid)) {
		return NT_STATUS_INVALID_HANDLE;
	}

	for (i = 0; i < num_names; i++) {
		if (lookup_global_sam_name(names[i], 0, &rids[i], &attrs[i])) {
			DEBUG(5,("lookup_names: %s-> %d:%d\n", names[i],
				 rids[i], attrs[i]));
			have_mapped = True;
		} else {
			have_unmapped = True;
			attrs[i] = SID_NAME_UNKNOWN;
		}
	}

 done:

	result = NT_STATUS_NONE_MAPPED;

	if (have_mapped)
		result = have_unmapped ? STATUS_SOME_UNMAPPED : NT_STATUS_OK;

	return result;
}

static struct pdb_search *pdb_search_init(enum pdb_search_type type)
{
	TALLOC_CTX *mem_ctx;
	struct pdb_search *result;

	mem_ctx = talloc_init("pdb_search");
	if (mem_ctx == NULL) {
		DEBUG(0, ("talloc_init failed\n"));
		return NULL;
	}

	result = TALLOC_P(mem_ctx, struct pdb_search);
	if (result == NULL) {
		DEBUG(0, ("talloc failed\n"));
		return NULL;
	}

	result->mem_ctx = mem_ctx;
	result->type = type;
	result->cache = NULL;
	result->num_entries = 0;
	result->cache_size = 0;
	result->search_ended = False;

	/* Segfault appropriately if not initialized */
	result->next_entry = NULL;
	result->search_end = NULL;

	return result;
}

static void fill_displayentry(TALLOC_CTX *mem_ctx, uint32 rid,
			      uint16 acct_flags,
			      const char *account_name,
			      const char *fullname,
			      const char *description,
			      struct samr_displayentry *entry)
{
	entry->rid = rid;
	entry->acct_flags = acct_flags;

	if (account_name != NULL)
		entry->account_name = talloc_strdup(mem_ctx, account_name);
	else
		entry->account_name = "";

	if (fullname != NULL)
		entry->fullname = talloc_strdup(mem_ctx, fullname);
	else
		entry->fullname = "";

	if (description != NULL)
		entry->description = talloc_strdup(mem_ctx, description);
	else
		entry->description = "";
}

static BOOL user_search_in_progress = False;
struct user_search {
	uint16 acct_flags;
};

static BOOL next_entry_users(struct pdb_search *s,
			     struct samr_displayentry *entry)
{
	struct user_search *state = s->private_data;
	struct samu *user = NULL;

 next:
	if ( !(user = samu_new( NULL )) ) {
		DEBUG(0, ("next_entry_users: samu_new() failed!\n"));
		return False;
	}

	if (!pdb_getsampwent(user)) {
		TALLOC_FREE(user);
		return False;
	}

 	if ((state->acct_flags != 0) &&
	    ((pdb_get_acct_ctrl(user) & state->acct_flags) == 0)) {
		TALLOC_FREE(user);
		goto next;
	}

	fill_displayentry(s->mem_ctx, pdb_get_user_rid(user),
			  pdb_get_acct_ctrl(user), pdb_get_username(user),
			  pdb_get_fullname(user), pdb_get_acct_desc(user),
			  entry);

	TALLOC_FREE(user);
	return True;
}

static void search_end_users(struct pdb_search *search)
{
	pdb_endsampwent();
	user_search_in_progress = False;
}

static BOOL pdb_default_search_users(struct pdb_methods *methods,
				     struct pdb_search *search,
				     uint32 acct_flags)
{
	struct user_search *state;

	if (user_search_in_progress) {
		DEBUG(1, ("user search in progress\n"));
		return False;
	}

	if (!pdb_setsampwent(False, acct_flags)) {
		DEBUG(5, ("Could not start search\n"));
		return False;
	}

	user_search_in_progress = True;

	state = TALLOC_P(search->mem_ctx, struct user_search);
	if (state == NULL) {
		DEBUG(0, ("talloc failed\n"));
		return False;
	}

	state->acct_flags = acct_flags;

	search->private_data = state;
	search->next_entry = next_entry_users;
	search->search_end = search_end_users;
	return True;
}

struct group_search {
	GROUP_MAP *groups;
	size_t num_groups, current_group;
};

static BOOL next_entry_groups(struct pdb_search *s,
			      struct samr_displayentry *entry)
{
	struct group_search *state = s->private_data;
	uint32 rid;
	GROUP_MAP *map = &state->groups[state->current_group];

	if (state->current_group == state->num_groups)
		return False;

	sid_peek_rid(&map->sid, &rid);

	fill_displayentry(s->mem_ctx, rid, 0, map->nt_name, NULL, map->comment,
			  entry);

	state->current_group += 1;
	return True;
}

static void search_end_groups(struct pdb_search *search)
{
	struct group_search *state = search->private_data;
	SAFE_FREE(state->groups);
}

static BOOL pdb_search_grouptype(struct pdb_search *search,
				 const DOM_SID *sid, enum SID_NAME_USE type)
{
	struct group_search *state;

	state = TALLOC_P(search->mem_ctx, struct group_search);
	if (state == NULL) {
		DEBUG(0, ("talloc failed\n"));
		return False;
	}

	if (!pdb_enum_group_mapping(sid, type, &state->groups, &state->num_groups,
				    True)) {
		DEBUG(0, ("Could not enum groups\n"));
		return False;
	}

	state->current_group = 0;
	search->private_data = state;
	search->next_entry = next_entry_groups;
	search->search_end = search_end_groups;
	return True;
}

static BOOL pdb_default_search_groups(struct pdb_methods *methods,
				      struct pdb_search *search)
{
	return pdb_search_grouptype(search, get_global_sam_sid(), SID_NAME_DOM_GRP);
}

static BOOL pdb_default_search_aliases(struct pdb_methods *methods,
				       struct pdb_search *search,
				       const DOM_SID *sid)
{

	return pdb_search_grouptype(search, sid, SID_NAME_ALIAS);
}

static struct samr_displayentry *pdb_search_getentry(struct pdb_search *search,
						     uint32 idx)
{
	if (idx < search->num_entries)
		return &search->cache[idx];

	if (search->search_ended)
		return NULL;

	while (idx >= search->num_entries) {
		struct samr_displayentry entry;

		if (!search->next_entry(search, &entry)) {
			search->search_end(search);
			search->search_ended = True;
			break;
		}

		ADD_TO_LARGE_ARRAY(search->mem_ctx, struct samr_displayentry,
				   entry, &search->cache, &search->num_entries,
				   &search->cache_size);
	}

	return (search->num_entries > idx) ? &search->cache[idx] : NULL;
}

struct pdb_search *pdb_search_users(uint32 acct_flags)
{
	struct pdb_methods *pdb = pdb_get_methods();
	struct pdb_search *result;

	result = pdb_search_init(PDB_USER_SEARCH);
	if (result == NULL) {
		return NULL;
	}

	if (!pdb->search_users(pdb, result, acct_flags)) {
		talloc_destroy(result->mem_ctx);
		return NULL;
	}
	return result;
}

struct pdb_search *pdb_search_groups(void)
{
	struct pdb_methods *pdb = pdb_get_methods();
	struct pdb_search *result;

	result = pdb_search_init(PDB_GROUP_SEARCH);
	if (result == NULL) {
		 return NULL;
	}

	if (!pdb->search_groups(pdb, result)) {
		talloc_destroy(result->mem_ctx);
		return NULL;
	}
	return result;
}

struct pdb_search *pdb_search_aliases(const DOM_SID *sid)
{
	struct pdb_methods *pdb = pdb_get_methods();
	struct pdb_search *result;

	if (pdb == NULL) return NULL;

	result = pdb_search_init(PDB_ALIAS_SEARCH);
	if (result == NULL) return NULL;

	if (!pdb->search_aliases(pdb, result, sid)) {
		talloc_destroy(result->mem_ctx);
		return NULL;
	}
	return result;
}

uint32 pdb_search_entries(struct pdb_search *search,
			  uint32 start_idx, uint32 max_entries,
			  struct samr_displayentry **result)
{
	struct samr_displayentry *end_entry;
	uint32 end_idx = start_idx+max_entries-1;

	/* The first entry needs to be searched after the last. Otherwise the
	 * first entry might have moved due to a realloc during the search for
	 * the last entry. */

	end_entry = pdb_search_getentry(search, end_idx);
	*result = pdb_search_getentry(search, start_idx);

	if (end_entry != NULL)
		return max_entries;

	if (start_idx >= search->num_entries)
		return 0;

	return search->num_entries - start_idx;
}

void pdb_search_destroy(struct pdb_search *search)
{
	if (search == NULL)
		return;

	if (!search->search_ended)
		search->search_end(search);

	talloc_destroy(search->mem_ctx);
}

/*******************************************************************
 Create a pdb_methods structure and initialize it with the default
 operations.  In this way a passdb module can simply implement
 the functionality it cares about.  However, normally this is done 
 in groups of related functions.
*******************************************************************/

NTSTATUS make_pdb_method( struct pdb_methods **methods ) 
{
	/* allocate memory for the structure as its own talloc CTX */

	if ( !(*methods = TALLOC_ZERO_P(NULL, struct pdb_methods) ) ) {
		return NT_STATUS_NO_MEMORY;
	}

	(*methods)->setsampwent = pdb_default_setsampwent;
	(*methods)->endsampwent = pdb_default_endsampwent;
	(*methods)->getsampwent = pdb_default_getsampwent;
	(*methods)->getsampwnam = pdb_default_getsampwnam;
	(*methods)->getsampwsid = pdb_default_getsampwsid;
	(*methods)->create_user = pdb_default_create_user;
	(*methods)->delete_user = pdb_default_delete_user;
	(*methods)->add_sam_account = pdb_default_add_sam_account;
	(*methods)->update_sam_account = pdb_default_update_sam_account;
	(*methods)->delete_sam_account = pdb_default_delete_sam_account;
	(*methods)->rename_sam_account = pdb_default_rename_sam_account;
	(*methods)->update_login_attempts = pdb_default_update_login_attempts;

	(*methods)->getgrsid = pdb_default_getgrsid;
	(*methods)->getgrgid = pdb_default_getgrgid;
	(*methods)->getgrnam = pdb_default_getgrnam;
	(*methods)->create_dom_group = pdb_default_create_dom_group;
	(*methods)->delete_dom_group = pdb_default_delete_dom_group;
	(*methods)->add_group_mapping_entry = pdb_default_add_group_mapping_entry;
	(*methods)->update_group_mapping_entry = pdb_default_update_group_mapping_entry;
	(*methods)->delete_group_mapping_entry = pdb_default_delete_group_mapping_entry;
	(*methods)->enum_group_mapping = pdb_default_enum_group_mapping;
	(*methods)->enum_group_members = pdb_default_enum_group_members;
	(*methods)->enum_group_memberships = pdb_default_enum_group_memberships;
	(*methods)->set_unix_primary_group = pdb_default_set_unix_primary_group;
	(*methods)->add_groupmem = pdb_default_add_groupmem;
	(*methods)->del_groupmem = pdb_default_del_groupmem;
	(*methods)->find_alias = pdb_default_find_alias;
	(*methods)->create_alias = pdb_default_create_alias;
	(*methods)->delete_alias = pdb_default_delete_alias;
	(*methods)->get_aliasinfo = pdb_default_get_aliasinfo;
	(*methods)->set_aliasinfo = pdb_default_set_aliasinfo;
	(*methods)->add_aliasmem = pdb_default_add_aliasmem;
	(*methods)->del_aliasmem = pdb_default_del_aliasmem;
	(*methods)->enum_aliasmem = pdb_default_enum_aliasmem;
	(*methods)->enum_alias_memberships = pdb_default_alias_memberships;
	(*methods)->lookup_rids = pdb_default_lookup_rids;
	(*methods)->get_account_policy = pdb_default_get_account_policy;
	(*methods)->set_account_policy = pdb_default_set_account_policy;
	(*methods)->get_seq_num = pdb_default_get_seq_num;
	(*methods)->uid_to_rid = pdb_default_uid_to_rid;
	(*methods)->gid_to_sid = pdb_default_gid_to_sid;
	(*methods)->sid_to_id = pdb_default_sid_to_id;

	(*methods)->search_users = pdb_default_search_users;
	(*methods)->search_groups = pdb_default_search_groups;
	(*methods)->search_aliases = pdb_default_search_aliases;

	return NT_STATUS_OK;
}
