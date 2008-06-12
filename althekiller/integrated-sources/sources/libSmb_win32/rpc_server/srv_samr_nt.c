/*
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell                   1992-1997,
 *  Copyright (C) Luke Kenneth Casson Leighton      1996-1997,
 *  Copyright (C) Paul Ashton                       1997,
 *  Copyright (C) Marc Jacobsen			    1999,
 *  Copyright (C) Jeremy Allison                    2001-2005,
 *  Copyright (C) Jean François Micouleau           1998-2001,
 *  Copyright (C) Jim McDonough <jmcd@us.ibm.com>   2002,
 *  Copyright (C) Gerald (Jerry) Carter             2003-2004,
 *  Copyright (C) Simo Sorce                        2003.
 *  Copyright (C) Volker Lendecke		    2005.
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

/*
 * This is the implementation of the SAMR code.
 */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

#define SAMR_USR_RIGHTS_WRITE_PW \
		( READ_CONTROL_ACCESS		| \
		  SA_RIGHT_USER_CHANGE_PASSWORD	| \
		  SA_RIGHT_USER_SET_LOC_COM )

#define DISP_INFO_CACHE_TIMEOUT 10

typedef struct disp_info {
	struct disp_info *next, *prev;
	TALLOC_CTX *mem_ctx;
	DOM_SID sid; /* identify which domain this is. */
	BOOL builtin_domain; /* Quick flag to check if this is the builtin domain. */
	struct pdb_search *users; /* querydispinfo 1 and 4 */
	struct pdb_search *machines; /* querydispinfo 2 */
	struct pdb_search *groups; /* querydispinfo 3 and 5, enumgroups */
	struct pdb_search *aliases; /* enumaliases */

	uint16 enum_acb_mask;
	struct pdb_search *enum_users; /* enumusers with a mask */


	smb_event_id_t di_cache_timeout_event; /* cache idle timeout handler. */
} DISP_INFO;

/* We keep a static list of these by SID as modern clients close down
   all resources between each request in a complete enumeration. */

static DISP_INFO *disp_info_list;

struct samr_info {
	/* for use by the \PIPE\samr policy */
	DOM_SID sid;
	BOOL builtin_domain; /* Quick flag to check if this is the builtin domain. */
	uint32 status; /* some sort of flag.  best to record it.  comes from opnum 0x39 */
	uint32 acc_granted;
	DISP_INFO *disp_info;
	TALLOC_CTX *mem_ctx;
};

static struct generic_mapping sam_generic_mapping = {
	GENERIC_RIGHTS_SAM_READ,
	GENERIC_RIGHTS_SAM_WRITE,
	GENERIC_RIGHTS_SAM_EXECUTE,
	GENERIC_RIGHTS_SAM_ALL_ACCESS};
static struct generic_mapping dom_generic_mapping = {
	GENERIC_RIGHTS_DOMAIN_READ,
	GENERIC_RIGHTS_DOMAIN_WRITE,
	GENERIC_RIGHTS_DOMAIN_EXECUTE,
	GENERIC_RIGHTS_DOMAIN_ALL_ACCESS};
static struct generic_mapping usr_generic_mapping = {
	GENERIC_RIGHTS_USER_READ,
	GENERIC_RIGHTS_USER_WRITE,
	GENERIC_RIGHTS_USER_EXECUTE,
	GENERIC_RIGHTS_USER_ALL_ACCESS};
static struct generic_mapping grp_generic_mapping = {
	GENERIC_RIGHTS_GROUP_READ,
	GENERIC_RIGHTS_GROUP_WRITE,
	GENERIC_RIGHTS_GROUP_EXECUTE,
	GENERIC_RIGHTS_GROUP_ALL_ACCESS};
static struct generic_mapping ali_generic_mapping = {
	GENERIC_RIGHTS_ALIAS_READ,
	GENERIC_RIGHTS_ALIAS_WRITE,
	GENERIC_RIGHTS_ALIAS_EXECUTE,
	GENERIC_RIGHTS_ALIAS_ALL_ACCESS};

/*******************************************************************
*******************************************************************/

static NTSTATUS make_samr_object_sd( TALLOC_CTX *ctx, SEC_DESC **psd, size_t *sd_size,
                                     struct generic_mapping *map,
				     DOM_SID *sid, uint32 sid_access )
{
	DOM_SID domadmin_sid;
	SEC_ACE ace[5];		/* at most 5 entries */
	SEC_ACCESS mask;
	size_t i = 0;

	SEC_ACL *psa = NULL;

	/* basic access for Everyone */

	init_sec_access(&mask, map->generic_execute | map->generic_read );
	init_sec_ace(&ace[i++], &global_sid_World, SEC_ACE_TYPE_ACCESS_ALLOWED, mask, 0);

	/* add Full Access 'BUILTIN\Administrators' and 'BUILTIN\Account Operators */

	init_sec_access(&mask, map->generic_all);
	
	init_sec_ace(&ace[i++], &global_sid_Builtin_Administrators, SEC_ACE_TYPE_ACCESS_ALLOWED, mask, 0);
	init_sec_ace(&ace[i++], &global_sid_Builtin_Account_Operators, SEC_ACE_TYPE_ACCESS_ALLOWED, mask, 0);

	/* Add Full Access for Domain Admins if we are a DC */
	
	if ( IS_DC ) {
		sid_copy( &domadmin_sid, get_global_sam_sid() );
		sid_append_rid( &domadmin_sid, DOMAIN_GROUP_RID_ADMINS );
		init_sec_ace(&ace[i++], &domadmin_sid, SEC_ACE_TYPE_ACCESS_ALLOWED, mask, 0);
	}

	/* if we have a sid, give it some special access */

	if ( sid ) {
		init_sec_access( &mask, sid_access );
		init_sec_ace(&ace[i++], sid, SEC_ACE_TYPE_ACCESS_ALLOWED, mask, 0);
	}

	/* create the security descriptor */

	if ((psa = make_sec_acl(ctx, NT4_ACL_REVISION, i, ace)) == NULL)
		return NT_STATUS_NO_MEMORY;

	if ((*psd = make_sec_desc(ctx, SEC_DESC_REVISION, SEC_DESC_SELF_RELATIVE, NULL, NULL, NULL, psa, sd_size)) == NULL)
		return NT_STATUS_NO_MEMORY;

	return NT_STATUS_OK;
}

/*******************************************************************
 Checks if access to an object should be granted, and returns that
 level of access for further checks.
********************************************************************/

static NTSTATUS access_check_samr_object( SEC_DESC *psd, NT_USER_TOKEN *token, 
                                          SE_PRIV *rights, uint32 rights_mask,
                                          uint32 des_access, uint32 *acc_granted, 
					  const char *debug )
{
	NTSTATUS status = NT_STATUS_ACCESS_DENIED;
	uint32 saved_mask = 0;

	/* check privileges; certain SAM access bits should be overridden 
	   by privileges (mostly having to do with creating/modifying/deleting 
	   users and groups) */
	
	if ( rights && user_has_any_privilege( token, rights ) ) {
	
		saved_mask = (des_access & rights_mask);
		des_access &= ~saved_mask;
		
		DEBUG(4,("access_check_samr_object: user rights access mask [0x%x]\n",
			rights_mask));
	}
		
	
	/* check the security descriptor first */
	
	if ( se_access_check(psd, token, des_access, acc_granted, &status) )
		goto done;
	
	/* give root a free pass */
	
	if ( geteuid() == sec_initial_uid() ) {
	
		DEBUG(4,("%s: ACCESS should be DENIED  (requested: %#010x)\n", debug, des_access));
		DEBUGADD(4,("but overritten by euid == sec_initial_uid()\n"));
		
		*acc_granted = des_access;
		
		status = NT_STATUS_OK;
		goto done;
	}
	
	
done:
	/* add in any bits saved during the privilege check (only 
	   matters is status is ok) */
	
	*acc_granted |= rights_mask;

	DEBUG(4,("%s: access %s (requested: 0x%08x, granted: 0x%08x)\n", 
		debug, NT_STATUS_IS_OK(status) ? "GRANTED" : "DENIED", 
		des_access, *acc_granted));
	
	return status;
}

/*******************************************************************
 Checks if access to a function can be granted
********************************************************************/

static NTSTATUS access_check_samr_function(uint32 acc_granted, uint32 acc_required, const char *debug)
{
	DEBUG(5,("%s: access check ((granted: %#010x;  required: %#010x)\n",  
		debug, acc_granted, acc_required));

	/* check the security descriptor first */
	
	if ( (acc_granted&acc_required) == acc_required )
		return NT_STATUS_OK;
		
	/* give root a free pass */

	if (geteuid() == sec_initial_uid()) {
	
		DEBUG(4,("%s: ACCESS should be DENIED (granted: %#010x;  required: %#010x)\n",
			debug, acc_granted, acc_required));
		DEBUGADD(4,("but overwritten by euid == 0\n"));
		
		return NT_STATUS_OK;
	}
	
	DEBUG(2,("%s: ACCESS DENIED (granted: %#010x;  required: %#010x)\n", 
		debug, acc_granted, acc_required));
		
	return NT_STATUS_ACCESS_DENIED;
}

/*******************************************************************
 Fetch or create a dispinfo struct.
********************************************************************/

static DISP_INFO *get_samr_dispinfo_by_sid(DOM_SID *psid, const char *sid_str)
{
	TALLOC_CTX *mem_ctx;
	DISP_INFO *dpi;

	/* There are two cases to consider here:
	   1) The SID is a domain SID and we look for an equality match, or
	   2) This is an account SID and so we return the DISP_INFO* for our 
	      domain */

	if ( psid && sid_check_is_in_our_domain( psid ) ) {
		DEBUG(10,("get_samr_dispinfo_by_sid: Replacing %s with our domain SID\n",
			sid_str));
		psid = get_global_sam_sid();
	}

	for (dpi = disp_info_list; dpi; dpi = dpi->next) {
		if (sid_equal(psid, &dpi->sid)) {
			return dpi;
		}
	}

	/* This struct is never free'd - I'm using talloc so we
	   can get a list out of smbd using smbcontrol. There will
	   be one of these per SID we're authorative for. JRA. */

	mem_ctx = talloc_init("DISP_INFO for domain sid %s", sid_str);

	if ((dpi = TALLOC_ZERO_P(mem_ctx, DISP_INFO)) == NULL)
		return NULL;

	dpi->mem_ctx = mem_ctx;

	if (psid) {
		sid_copy( &dpi->sid, psid);
		dpi->builtin_domain = sid_check_is_builtin(psid);
	} else {
		dpi->builtin_domain = False;
	}

	DLIST_ADD(disp_info_list, dpi);

	return dpi;
}

/*******************************************************************
 Create a samr_info struct.
********************************************************************/

static struct samr_info *get_samr_info_by_sid(DOM_SID *psid)
{
	struct samr_info *info;
	fstring sid_str;
	TALLOC_CTX *mem_ctx;
	
	if (psid) {
		sid_to_string(sid_str, psid);
	} else {
		fstrcpy(sid_str,"(NULL)");
	}

	mem_ctx = talloc_init("samr_info for domain sid %s", sid_str);

	if ((info = TALLOC_ZERO_P(mem_ctx, struct samr_info)) == NULL)
		return NULL;

	DEBUG(10,("get_samr_info_by_sid: created new info for sid %s\n", sid_str));
	if (psid) {
		sid_copy( &info->sid, psid);
		info->builtin_domain = sid_check_is_builtin(psid);
	} else {
		DEBUG(10,("get_samr_info_by_sid: created new info for NULL sid.\n"));
		info->builtin_domain = False;
	}
	info->mem_ctx = mem_ctx;

	info->disp_info = get_samr_dispinfo_by_sid(psid, sid_str);

	if (!info->disp_info) {
		talloc_destroy(mem_ctx);
		return NULL;
	}

	return info;
}

/*******************************************************************
 Function to free the per SID data.
 ********************************************************************/

static void free_samr_cache(DISP_INFO *disp_info, const char *sid_str)
{
	DEBUG(10,("free_samr_cache: deleting cache for SID %s\n", sid_str));

	/* We need to become root here because the paged search might have to
	 * tell the LDAP server we're not interested in the rest anymore. */

	become_root();

	if (disp_info->users) {
		DEBUG(10,("free_samr_cache: deleting users cache\n"));
		pdb_search_destroy(disp_info->users);
		disp_info->users = NULL;
	}
	if (disp_info->machines) {
		DEBUG(10,("free_samr_cache: deleting machines cache\n"));
		pdb_search_destroy(disp_info->machines);
		disp_info->machines = NULL;
	}
	if (disp_info->groups) {
		DEBUG(10,("free_samr_cache: deleting groups cache\n"));
		pdb_search_destroy(disp_info->groups);
		disp_info->groups = NULL;
	}
	if (disp_info->aliases) {
		DEBUG(10,("free_samr_cache: deleting aliases cache\n"));
		pdb_search_destroy(disp_info->aliases);
		disp_info->aliases = NULL;
	}
	if (disp_info->enum_users) {
		DEBUG(10,("free_samr_cache: deleting enum_users cache\n"));
		pdb_search_destroy(disp_info->enum_users);
		disp_info->enum_users = NULL;
	}
	disp_info->enum_acb_mask = 0;

	unbecome_root();
}

/*******************************************************************
 Function to free the per handle data.
 ********************************************************************/

static void free_samr_info(void *ptr)
{
	struct samr_info *info=(struct samr_info *) ptr;

	/* Only free the dispinfo cache if no one bothered to set up
	   a timeout. */

	if (info->disp_info && info->disp_info->di_cache_timeout_event == (smb_event_id_t)0) {
		fstring sid_str;
		sid_to_string(sid_str, &info->disp_info->sid);
		free_samr_cache(info->disp_info, sid_str);
	}

	talloc_destroy(info->mem_ctx);
}

/*******************************************************************
 Idle event handler. Throw away the disp info cache.
 ********************************************************************/

static void disp_info_cache_idle_timeout_handler(void **private_data,
					time_t *ev_interval,
					time_t ev_now)
{
	fstring sid_str;
	DISP_INFO *disp_info = (DISP_INFO *)(*private_data);

	sid_to_string(sid_str, &disp_info->sid);

	free_samr_cache(disp_info, sid_str);

	/* Remove the event. */
	smb_unregister_idle_event(disp_info->di_cache_timeout_event);
	disp_info->di_cache_timeout_event = (smb_event_id_t)0;

	DEBUG(10,("disp_info_cache_idle_timeout_handler: caching timed out for SID %s at %u\n",
		sid_str, (unsigned int)ev_now));
}

/*******************************************************************
 Setup cache removal idle event handler.
 ********************************************************************/

static void set_disp_info_cache_timeout(DISP_INFO *disp_info, time_t secs_fromnow)
{
	fstring sid_str;

	sid_to_string(sid_str, &disp_info->sid);

	/* Remove any pending timeout and update. */

	if (disp_info->di_cache_timeout_event) {
		smb_unregister_idle_event(disp_info->di_cache_timeout_event);
		disp_info->di_cache_timeout_event = (smb_event_id_t)0;
	}

	DEBUG(10,("set_disp_info_cache_timeout: caching enumeration for SID %s for %u seconds\n",
		sid_str, (unsigned int)secs_fromnow ));

	disp_info->di_cache_timeout_event =
		smb_register_idle_event(disp_info_cache_idle_timeout_handler,
					disp_info,
					secs_fromnow);
}

/*******************************************************************
 Force flush any cache. We do this on any samr_set_xxx call.
 We must also remove the timeout handler.
 ********************************************************************/

static void force_flush_samr_cache(DISP_INFO *disp_info)
{
	if (disp_info) {
		fstring sid_str;

		sid_to_string(sid_str, &disp_info->sid);
		if (disp_info->di_cache_timeout_event) {
			smb_unregister_idle_event(disp_info->di_cache_timeout_event);
			disp_info->di_cache_timeout_event = (smb_event_id_t)0;
			DEBUG(10,("force_flush_samr_cache: clearing idle event for SID %s\n",
				sid_str));
		}
		free_samr_cache(disp_info, sid_str);
	}
}

/*******************************************************************
 Ensure password info is never given out. Paranioa... JRA.
 ********************************************************************/

static void samr_clear_sam_passwd(struct samu *sam_pass)
{
	
	if (!sam_pass)
		return;

	/* These now zero out the old password */

	pdb_set_lanman_passwd(sam_pass, NULL, PDB_DEFAULT);
	pdb_set_nt_passwd(sam_pass, NULL, PDB_DEFAULT);
}

static uint32 count_sam_users(struct disp_info *info, uint32 acct_flags)
{
	struct samr_displayentry *entry;

	if (info->builtin_domain) {
		/* No users in builtin. */
		return 0;
	}

	if (info->users == NULL) {
		info->users = pdb_search_users(acct_flags);
		if (info->users == NULL) {
			return 0;
		}
	}
	/* Fetch the last possible entry, thus trigger an enumeration */
	pdb_search_entries(info->users, 0xffffffff, 1, &entry);

	/* Ensure we cache this enumeration. */
	set_disp_info_cache_timeout(info, DISP_INFO_CACHE_TIMEOUT);

	return info->users->num_entries;
}

static uint32 count_sam_groups(struct disp_info *info)
{
	struct samr_displayentry *entry;

	if (info->builtin_domain) {
		/* No groups in builtin. */
		return 0;
	}

	if (info->groups == NULL) {
		info->groups = pdb_search_groups();
		if (info->groups == NULL) {
			return 0;
		}
	}
	/* Fetch the last possible entry, thus trigger an enumeration */
	pdb_search_entries(info->groups, 0xffffffff, 1, &entry);

	/* Ensure we cache this enumeration. */
	set_disp_info_cache_timeout(info, DISP_INFO_CACHE_TIMEOUT);

	return info->groups->num_entries;
}

static uint32 count_sam_aliases(struct disp_info *info)
{
	struct samr_displayentry *entry;

	if (info->aliases == NULL) {
		info->aliases = pdb_search_aliases(&info->sid);
		if (info->aliases == NULL) {
			return 0;
		}
	}
	/* Fetch the last possible entry, thus trigger an enumeration */
	pdb_search_entries(info->aliases, 0xffffffff, 1, &entry);

	/* Ensure we cache this enumeration. */
	set_disp_info_cache_timeout(info, DISP_INFO_CACHE_TIMEOUT);

	return info->aliases->num_entries;
}

/*******************************************************************
 _samr_close_hnd
 ********************************************************************/

NTSTATUS _samr_close_hnd(pipes_struct *p, SAMR_Q_CLOSE_HND *q_u, SAMR_R_CLOSE_HND *r_u)
{
	r_u->status = NT_STATUS_OK;

	/* close the policy handle */
	if (!close_policy_hnd(p, &q_u->pol))
		return NT_STATUS_OBJECT_NAME_INVALID;

	DEBUG(5,("samr_reply_close_hnd: %d\n", __LINE__));

	return r_u->status;
}

/*******************************************************************
 samr_reply_open_domain
 ********************************************************************/

NTSTATUS _samr_open_domain(pipes_struct *p, SAMR_Q_OPEN_DOMAIN *q_u, SAMR_R_OPEN_DOMAIN *r_u)
{
	struct    samr_info *info;
	SEC_DESC *psd = NULL;
	uint32    acc_granted;
	uint32    des_access = q_u->flags;
	NTSTATUS  status;
	size_t    sd_size;
	SE_PRIV se_rights;

	r_u->status = NT_STATUS_OK;

	/* find the connection policy handle. */
	
	if ( !find_policy_by_hnd(p, &q_u->pol, (void**)(void *)&info) )
		return NT_STATUS_INVALID_HANDLE;

	status = access_check_samr_function( info->acc_granted, 
		SA_RIGHT_SAM_OPEN_DOMAIN, "_samr_open_domain" );
		
	if ( !NT_STATUS_IS_OK(status) )
		return status;

	/*check if access can be granted as requested by client. */
	
	make_samr_object_sd( p->mem_ctx, &psd, &sd_size, &dom_generic_mapping, NULL, 0 );
	se_map_generic( &des_access, &dom_generic_mapping );
	
	se_priv_copy( &se_rights, &se_machine_account );
	se_priv_add( &se_rights, &se_add_users );

	status = access_check_samr_object( psd, p->pipe_user.nt_user_token, 
		&se_rights, GENERIC_RIGHTS_DOMAIN_WRITE, des_access, 
		&acc_granted, "_samr_open_domain" );
		
	if ( !NT_STATUS_IS_OK(status) )
		return status;

	if (!sid_check_is_domain(&q_u->dom_sid.sid) &&
	    !sid_check_is_builtin(&q_u->dom_sid.sid)) {
		return NT_STATUS_NO_SUCH_DOMAIN;
	}

	/* associate the domain SID with the (unique) handle. */
	if ((info = get_samr_info_by_sid(&q_u->dom_sid.sid))==NULL)
		return NT_STATUS_NO_MEMORY;
	info->acc_granted = acc_granted;

	/* get a (unique) handle.  open a policy on it. */
	if (!create_policy_hnd(p, &r_u->domain_pol, free_samr_info, (void *)info))
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;

	DEBUG(5,("samr_open_domain: %d\n", __LINE__));

	return r_u->status;
}

/*******************************************************************
 _samr_get_usrdom_pwinfo
 ********************************************************************/

NTSTATUS _samr_get_usrdom_pwinfo(pipes_struct *p, SAMR_Q_GET_USRDOM_PWINFO *q_u, SAMR_R_GET_USRDOM_PWINFO *r_u)
{
	struct samr_info *info = NULL;

	r_u->status = NT_STATUS_OK;

	/* find the policy handle.  open a policy on it. */
	if (!find_policy_by_hnd(p, &q_u->user_pol, (void **)(void *)&info))
		return NT_STATUS_INVALID_HANDLE;

	if (!sid_check_is_in_our_domain(&info->sid))
		return NT_STATUS_OBJECT_TYPE_MISMATCH;

	init_samr_r_get_usrdom_pwinfo(r_u, NT_STATUS_OK);

	DEBUG(5,("_samr_get_usrdom_pwinfo: %d\n", __LINE__));

	/* 
	 * NT sometimes return NT_STATUS_ACCESS_DENIED
	 * I don't know yet why.
	 */

	return r_u->status;
}

/*******************************************************************
 _samr_set_sec_obj
 ********************************************************************/

NTSTATUS _samr_set_sec_obj(pipes_struct *p, SAMR_Q_SET_SEC_OBJ *q_u, SAMR_R_SET_SEC_OBJ *r_u)
{
	DEBUG(0,("_samr_set_sec_obj: Not yet implemented!\n"));
	return NT_STATUS_NOT_IMPLEMENTED;
}

/*******************************************************************
********************************************************************/

static BOOL get_lsa_policy_samr_sid( pipes_struct *p, POLICY_HND *pol, 
					DOM_SID *sid, uint32 *acc_granted,
					DISP_INFO **ppdisp_info)
{
	struct samr_info *info = NULL;

	/* find the policy handle.  open a policy on it. */
	if (!find_policy_by_hnd(p, pol, (void **)(void *)&info))
		return False;

	if (!info)
		return False;

	*sid = info->sid;
	*acc_granted = info->acc_granted;
	if (ppdisp_info) {
		*ppdisp_info = info->disp_info;
	}

	return True;
}

/*******************************************************************
 _samr_query_sec_obj
 ********************************************************************/

NTSTATUS _samr_query_sec_obj(pipes_struct *p, SAMR_Q_QUERY_SEC_OBJ *q_u, SAMR_R_QUERY_SEC_OBJ *r_u)
{
	DOM_SID pol_sid;
	fstring str_sid;
	SEC_DESC * psd = NULL;
	uint32 acc_granted;
	size_t sd_size;

	r_u->status = NT_STATUS_OK;

	/* Get the SID. */
	if (!get_lsa_policy_samr_sid(p, &q_u->user_pol, &pol_sid, &acc_granted, NULL))
		return NT_STATUS_INVALID_HANDLE;

	DEBUG(10,("_samr_query_sec_obj: querying security on SID: %s\n", sid_to_string(str_sid, &pol_sid)));

	/* Check what typ of SID is beeing queried (e.g Domain SID, User SID, Group SID) */

	/* To query the security of the SAM it self an invalid SID with S-0-0 is passed to this function */
	if (pol_sid.sid_rev_num == 0) {
		DEBUG(5,("_samr_query_sec_obj: querying security on SAM\n"));
		r_u->status = make_samr_object_sd(p->mem_ctx, &psd, &sd_size, &sam_generic_mapping, NULL, 0);
	} else if (sid_equal(&pol_sid,get_global_sam_sid())) { 
		/* check if it is our domain SID */
		DEBUG(5,("_samr_query_sec_obj: querying security on Domain with SID: %s\n", sid_to_string(str_sid, &pol_sid)));
		r_u->status = make_samr_object_sd(p->mem_ctx, &psd, &sd_size, &dom_generic_mapping, NULL, 0);
	} else if (sid_equal(&pol_sid,&global_sid_Builtin)) {
		/* check if it is the Builtin  Domain */
		/* TODO: Builtin probably needs a different SD with restricted write access*/
		DEBUG(5,("_samr_query_sec_obj: querying security on Builtin Domain with SID: %s\n", sid_to_string(str_sid, &pol_sid)));
		r_u->status = make_samr_object_sd(p->mem_ctx, &psd, &sd_size, &dom_generic_mapping, NULL, 0);
	} else if (sid_check_is_in_our_domain(&pol_sid) ||
	    	 sid_check_is_in_builtin(&pol_sid)) {
		/* TODO: different SDs have to be generated for aliases groups and users.
		         Currently all three get a default user SD  */
		DEBUG(10,("_samr_query_sec_obj: querying security on Object with SID: %s\n", sid_to_string(str_sid, &pol_sid)));
		r_u->status = make_samr_object_sd(p->mem_ctx, &psd, &sd_size, &usr_generic_mapping, &pol_sid, SAMR_USR_RIGHTS_WRITE_PW);
	} else {
		return NT_STATUS_OBJECT_TYPE_MISMATCH;
	}

	if ((r_u->buf = make_sec_desc_buf(p->mem_ctx, sd_size, psd)) == NULL)
		return NT_STATUS_NO_MEMORY;

	if (NT_STATUS_IS_OK(r_u->status))
		r_u->ptr = 1;

	return r_u->status;
}

/*******************************************************************
makes a SAM_ENTRY / UNISTR2* structure from a user list.
********************************************************************/

static NTSTATUS make_user_sam_entry_list(TALLOC_CTX *ctx, SAM_ENTRY **sam_pp,
					 UNISTR2 **uni_name_pp,
					 uint32 num_entries, uint32 start_idx,
					 struct samr_displayentry *entries)
{
	uint32 i;
	SAM_ENTRY *sam;
	UNISTR2 *uni_name;
	
	*sam_pp = NULL;
	*uni_name_pp = NULL;

	if (num_entries == 0)
		return NT_STATUS_OK;

	sam = TALLOC_ZERO_ARRAY(ctx, SAM_ENTRY, num_entries);

	uni_name = TALLOC_ZERO_ARRAY(ctx, UNISTR2, num_entries);

	if (sam == NULL || uni_name == NULL) {
		DEBUG(0, ("make_user_sam_entry_list: talloc_zero failed!\n"));
		return NT_STATUS_NO_MEMORY;
	}

	for (i = 0; i < num_entries; i++) {
		UNISTR2 uni_temp_name;
		/*
		 * usrmgr expects a non-NULL terminated string with
		 * trust relationships
		 */
		if (entries[i].acct_flags & ACB_DOMTRUST) {
			init_unistr2(&uni_temp_name, entries[i].account_name,
				     UNI_FLAGS_NONE);
		} else {
			init_unistr2(&uni_temp_name, entries[i].account_name,
				     UNI_STR_TERMINATE);
		}

		init_sam_entry(&sam[i], &uni_temp_name, entries[i].rid);
		copy_unistr2(&uni_name[i], &uni_temp_name);
	}

	*sam_pp = sam;
	*uni_name_pp = uni_name;
	return NT_STATUS_OK;
}

/*******************************************************************
 samr_reply_enum_dom_users
 ********************************************************************/

NTSTATUS _samr_enum_dom_users(pipes_struct *p, SAMR_Q_ENUM_DOM_USERS *q_u, 
			      SAMR_R_ENUM_DOM_USERS *r_u)
{
	struct samr_info *info = NULL;
	int num_account;
	uint32 enum_context=q_u->start_idx;
	enum remote_arch_types ra_type = get_remote_arch();
	int max_sam_entries = (ra_type == RA_WIN95) ? MAX_SAM_ENTRIES_W95 : MAX_SAM_ENTRIES_W2K;
	uint32 max_entries = max_sam_entries;
	struct samr_displayentry *entries = NULL;
	
	r_u->status = NT_STATUS_OK;

	/* find the policy handle.  open a policy on it. */
	if (!find_policy_by_hnd(p, &q_u->pol, (void **)(void *)&info))
		return NT_STATUS_INVALID_HANDLE;

 	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(info->acc_granted, 
					SA_RIGHT_DOMAIN_ENUM_ACCOUNTS, 
					"_samr_enum_dom_users"))) {
 		return r_u->status;
	}
 	
	DEBUG(5,("_samr_enum_dom_users: %d\n", __LINE__));

	if (info->builtin_domain) {
		/* No users in builtin. */
		init_samr_r_enum_dom_users(r_u, q_u->start_idx, 0);
		DEBUG(5,("_samr_enum_dom_users: No users in BUILTIN\n"));
		return r_u->status;
	}

	become_root();

	/* AS ROOT !!!! */

	if ((info->disp_info->enum_users != NULL) &&
	    (info->disp_info->enum_acb_mask != q_u->acb_mask)) {
		pdb_search_destroy(info->disp_info->enum_users);
		info->disp_info->enum_users = NULL;
	}

	if (info->disp_info->enum_users == NULL) {
		info->disp_info->enum_users = pdb_search_users(q_u->acb_mask);
		info->disp_info->enum_acb_mask = q_u->acb_mask;
	}

	if (info->disp_info->enum_users == NULL) {
		/* END AS ROOT !!!! */
		unbecome_root();
		return NT_STATUS_ACCESS_DENIED;
	}

	num_account = pdb_search_entries(info->disp_info->enum_users,
					 enum_context, max_entries,
					 &entries);

	/* END AS ROOT !!!! */

	unbecome_root();

	if (num_account == 0) {
		DEBUG(5, ("_samr_enum_dom_users: enumeration handle over "
			  "total entries\n"));
		return NT_STATUS_OK;
	}

	r_u->status = make_user_sam_entry_list(p->mem_ctx, &r_u->sam,
					       &r_u->uni_acct_name, 
					       num_account, enum_context,
					       entries);

	if (!NT_STATUS_IS_OK(r_u->status))
		return r_u->status;

	if (max_entries <= num_account) {
		r_u->status = STATUS_MORE_ENTRIES;
	} else {
		r_u->status = NT_STATUS_OK;
	}

	/* Ensure we cache this enumeration. */
	set_disp_info_cache_timeout(info->disp_info, DISP_INFO_CACHE_TIMEOUT);

	DEBUG(5, ("_samr_enum_dom_users: %d\n", __LINE__));

	init_samr_r_enum_dom_users(r_u, q_u->start_idx + num_account,
				   num_account);

	DEBUG(5,("_samr_enum_dom_users: %d\n", __LINE__));

	return r_u->status;
}

/*******************************************************************
makes a SAM_ENTRY / UNISTR2* structure from a group list.
********************************************************************/

static void make_group_sam_entry_list(TALLOC_CTX *ctx, SAM_ENTRY **sam_pp,
				      UNISTR2 **uni_name_pp,
				      uint32 num_sam_entries,
				      struct samr_displayentry *entries)
{
	uint32 i;
	SAM_ENTRY *sam;
	UNISTR2 *uni_name;

	*sam_pp = NULL;
	*uni_name_pp = NULL;

	if (num_sam_entries == 0)
		return;

	sam = TALLOC_ZERO_ARRAY(ctx, SAM_ENTRY, num_sam_entries);
	uni_name = TALLOC_ZERO_ARRAY(ctx, UNISTR2, num_sam_entries);

	if (sam == NULL || uni_name == NULL) {
		DEBUG(0, ("NULL pointers in SAMR_R_QUERY_DISPINFO\n"));
		return;
	}

	for (i = 0; i < num_sam_entries; i++) {
		/*
		 * JRA. I think this should include the null. TNG does not.
		 */
		init_unistr2(&uni_name[i], entries[i].account_name,
			     UNI_STR_TERMINATE);
		init_sam_entry(&sam[i], &uni_name[i], entries[i].rid);
	}

	*sam_pp = sam;
	*uni_name_pp = uni_name;
}

/*******************************************************************
 samr_reply_enum_dom_groups
 ********************************************************************/

NTSTATUS _samr_enum_dom_groups(pipes_struct *p, SAMR_Q_ENUM_DOM_GROUPS *q_u, SAMR_R_ENUM_DOM_GROUPS *r_u)
{
	struct samr_info *info = NULL;
	struct samr_displayentry *groups;
	uint32 num_groups;

	r_u->status = NT_STATUS_OK;

	/* find the policy handle.  open a policy on it. */
	if (!find_policy_by_hnd(p, &q_u->pol, (void **)(void *)&info))
		return NT_STATUS_INVALID_HANDLE;

	r_u->status = access_check_samr_function(info->acc_granted,
						 SA_RIGHT_DOMAIN_ENUM_ACCOUNTS,
						 "_samr_enum_dom_groups");
	if (!NT_STATUS_IS_OK(r_u->status))
		return r_u->status;

	DEBUG(5,("samr_reply_enum_dom_groups: %d\n", __LINE__));

	if (info->builtin_domain) {
		/* No groups in builtin. */
		init_samr_r_enum_dom_groups(r_u, q_u->start_idx, 0);
		DEBUG(5,("_samr_enum_dom_users: No groups in BUILTIN\n"));
		return r_u->status;
	}

	/* the domain group array is being allocated in the function below */

	become_root();

	if (info->disp_info->groups == NULL) {
		info->disp_info->groups = pdb_search_groups();

		if (info->disp_info->groups == NULL) {
			unbecome_root();
			return NT_STATUS_ACCESS_DENIED;
		}
	}

	num_groups = pdb_search_entries(info->disp_info->groups, q_u->start_idx,
					MAX_SAM_ENTRIES, &groups);
	unbecome_root();
	
	/* Ensure we cache this enumeration. */
	set_disp_info_cache_timeout(info->disp_info, DISP_INFO_CACHE_TIMEOUT);

	make_group_sam_entry_list(p->mem_ctx, &r_u->sam, &r_u->uni_grp_name,
				  num_groups, groups);

	init_samr_r_enum_dom_groups(r_u, q_u->start_idx, num_groups);

	DEBUG(5,("samr_enum_dom_groups: %d\n", __LINE__));

	return r_u->status;
}

/*******************************************************************
 samr_reply_enum_dom_aliases
 ********************************************************************/

NTSTATUS _samr_enum_dom_aliases(pipes_struct *p, SAMR_Q_ENUM_DOM_ALIASES *q_u, SAMR_R_ENUM_DOM_ALIASES *r_u)
{
	struct samr_info *info;
	struct samr_displayentry *aliases;
	uint32 num_aliases = 0;

	/* find the policy handle.  open a policy on it. */
	if (!find_policy_by_hnd(p, &q_u->pol, (void **)(void *)&info))
		return NT_STATUS_INVALID_HANDLE;

	r_u->status = access_check_samr_function(info->acc_granted,
						 SA_RIGHT_DOMAIN_ENUM_ACCOUNTS,
						 "_samr_enum_dom_aliases");
	if (!NT_STATUS_IS_OK(r_u->status))
		return r_u->status;

	DEBUG(5,("samr_reply_enum_dom_aliases: sid %s\n",
		 sid_string_static(&info->sid)));

	become_root();

	if (info->disp_info->aliases == NULL) {
		info->disp_info->aliases = pdb_search_aliases(&info->sid);
		if (info->disp_info->aliases == NULL) {
			unbecome_root();
			return NT_STATUS_ACCESS_DENIED;
		}
	}

	num_aliases = pdb_search_entries(info->disp_info->aliases, q_u->start_idx,
					 MAX_SAM_ENTRIES, &aliases);
	unbecome_root();
	
	/* Ensure we cache this enumeration. */
	set_disp_info_cache_timeout(info->disp_info, DISP_INFO_CACHE_TIMEOUT);

	make_group_sam_entry_list(p->mem_ctx, &r_u->sam, &r_u->uni_grp_name,
				  num_aliases, aliases);

	init_samr_r_enum_dom_aliases(r_u, q_u->start_idx + num_aliases,
				     num_aliases);

	DEBUG(5,("samr_enum_dom_aliases: %d\n", __LINE__));

	return r_u->status;
}

/*******************************************************************
 samr_reply_query_dispinfo
 ********************************************************************/

NTSTATUS _samr_query_dispinfo(pipes_struct *p, SAMR_Q_QUERY_DISPINFO *q_u, 
			      SAMR_R_QUERY_DISPINFO *r_u)
{
	struct samr_info *info = NULL;
	uint32 struct_size=0x20; /* W2K always reply that, client doesn't care */
	
	uint32 max_entries=q_u->max_entries;
	uint32 enum_context=q_u->start_idx;
	uint32 max_size=q_u->max_size;

	SAM_DISPINFO_CTR *ctr;
	uint32 temp_size=0, total_data_size=0;
	NTSTATUS disp_ret = NT_STATUS_UNSUCCESSFUL;
	uint32 num_account = 0;
	enum remote_arch_types ra_type = get_remote_arch();
	int max_sam_entries = (ra_type == RA_WIN95) ? MAX_SAM_ENTRIES_W95 : MAX_SAM_ENTRIES_W2K;
	struct samr_displayentry *entries = NULL;

	DEBUG(5, ("samr_reply_query_dispinfo: %d\n", __LINE__));
	r_u->status = NT_STATUS_UNSUCCESSFUL;

	/* find the policy handle.  open a policy on it. */
	if (!find_policy_by_hnd(p, &q_u->domain_pol, (void **)(void *)&info))
		return NT_STATUS_INVALID_HANDLE;

	/*
	 * calculate how many entries we will return.
	 * based on 
	 * - the number of entries the client asked
	 * - our limit on that
	 * - the starting point (enumeration context)
	 * - the buffer size the client will accept
	 */

	/*
	 * We are a lot more like W2K. Instead of reading the SAM
	 * each time to find the records we need to send back,
	 * we read it once and link that copy to the sam handle.
	 * For large user list (over the MAX_SAM_ENTRIES)
	 * it's a definitive win.
	 * second point to notice: between enumerations
	 * our sam is now the same as it's a snapshoot.
	 * third point: got rid of the static SAM_USER_21 struct
	 * no more intermediate.
	 * con: it uses much more memory, as a full copy is stored
	 * in memory.
	 *
	 * If you want to change it, think twice and think
	 * of the second point , that's really important.
	 *
	 * JFM, 12/20/2001
	 */

	if ((q_u->switch_level < 1) || (q_u->switch_level > 5)) {
		DEBUG(0,("_samr_query_dispinfo: Unknown info level (%u)\n",
			 (unsigned int)q_u->switch_level ));
		return NT_STATUS_INVALID_INFO_CLASS;
	}

	/* first limit the number of entries we will return */
	if(max_entries > max_sam_entries) {
		DEBUG(5, ("samr_reply_query_dispinfo: client requested %d "
			  "entries, limiting to %d\n", max_entries,
			  max_sam_entries));
		max_entries = max_sam_entries;
	}

	/* calculate the size and limit on the number of entries we will
	 * return */

	temp_size=max_entries*struct_size;
	
	if (temp_size>max_size) {
		max_entries=MIN((max_size/struct_size),max_entries);;
		DEBUG(5, ("samr_reply_query_dispinfo: buffer size limits to "
			  "only %d entries\n", max_entries));
	}

	if (!(ctr = TALLOC_ZERO_P(p->mem_ctx,SAM_DISPINFO_CTR)))
		return NT_STATUS_NO_MEMORY;

	ZERO_STRUCTP(ctr);

	become_root();

	/* THe following done as ROOT. Don't return without unbecome_root(). */

	switch (q_u->switch_level) {
	case 0x1:
	case 0x4:
		if (info->disp_info->users == NULL) {
			info->disp_info->users = pdb_search_users(ACB_NORMAL);
			if (info->disp_info->users == NULL) {
				unbecome_root();
				return NT_STATUS_ACCESS_DENIED;
			}
			DEBUG(10,("samr_reply_query_dispinfo: starting user enumeration at index %u\n",
				(unsigned  int)enum_context ));
		} else {
			DEBUG(10,("samr_reply_query_dispinfo: using cached user enumeration at index %u\n",
				(unsigned  int)enum_context ));
		}

		num_account = pdb_search_entries(info->disp_info->users,
						 enum_context, max_entries,
						 &entries);
		break;
	case 0x2:
		if (info->disp_info->machines == NULL) {
			info->disp_info->machines =
				pdb_search_users(ACB_WSTRUST|ACB_SVRTRUST);
			if (info->disp_info->machines == NULL) {
				unbecome_root();
				return NT_STATUS_ACCESS_DENIED;
			}
			DEBUG(10,("samr_reply_query_dispinfo: starting machine enumeration at index %u\n",
				(unsigned  int)enum_context ));
		} else {
			DEBUG(10,("samr_reply_query_dispinfo: using cached machine enumeration at index %u\n",
				(unsigned  int)enum_context ));
		}

		num_account = pdb_search_entries(info->disp_info->machines,
						 enum_context, max_entries,
						 &entries);
		break;
	case 0x3:
	case 0x5:
		if (info->disp_info->groups == NULL) {
			info->disp_info->groups = pdb_search_groups();
			if (info->disp_info->groups == NULL) {
				unbecome_root();
				return NT_STATUS_ACCESS_DENIED;
			}
			DEBUG(10,("samr_reply_query_dispinfo: starting group enumeration at index %u\n",
				(unsigned  int)enum_context ));
		} else {
			DEBUG(10,("samr_reply_query_dispinfo: using cached group enumeration at index %u\n",
				(unsigned  int)enum_context ));
		}

		num_account = pdb_search_entries(info->disp_info->groups,
						 enum_context, max_entries,
						 &entries);
		break;
	default:
		unbecome_root();
		smb_panic("info class changed");
		break;
	}
	unbecome_root();

	/* Now create reply structure */
	switch (q_u->switch_level) {
	case 0x1:
		disp_ret = init_sam_dispinfo_1(p->mem_ctx, &ctr->sam.info1,
					       num_account, enum_context,
					       entries);
		break;
	case 0x2:
		disp_ret = init_sam_dispinfo_2(p->mem_ctx, &ctr->sam.info2,
					       num_account, enum_context,
					       entries);
		break;
	case 0x3:
		disp_ret = init_sam_dispinfo_3(p->mem_ctx, &ctr->sam.info3,
					       num_account, enum_context,
					       entries);
		break;
	case 0x4:
		disp_ret = init_sam_dispinfo_4(p->mem_ctx, &ctr->sam.info4,
					       num_account, enum_context,
					       entries);
		break;
	case 0x5:
		disp_ret = init_sam_dispinfo_5(p->mem_ctx, &ctr->sam.info5,
					       num_account, enum_context,
					       entries);
		break;
	default:
		smb_panic("info class changed");
		break;
	}

	if (!NT_STATUS_IS_OK(disp_ret))
		return disp_ret;

	/* calculate the total size */
	total_data_size=num_account*struct_size;

	if (num_account) {
		r_u->status = STATUS_MORE_ENTRIES;
	} else {
		r_u->status = NT_STATUS_OK;
	}

	/* Ensure we cache this enumeration. */
	set_disp_info_cache_timeout(info->disp_info, DISP_INFO_CACHE_TIMEOUT);

	DEBUG(5, ("_samr_query_dispinfo: %d\n", __LINE__));

	init_samr_r_query_dispinfo(r_u, num_account, total_data_size,
				   temp_size, q_u->switch_level, ctr,
				   r_u->status);

	return r_u->status;

}

/*******************************************************************
 samr_reply_query_aliasinfo
 ********************************************************************/

NTSTATUS _samr_query_aliasinfo(pipes_struct *p, SAMR_Q_QUERY_ALIASINFO *q_u, SAMR_R_QUERY_ALIASINFO *r_u)
{
	DOM_SID   sid;
	struct acct_info info;
	uint32    acc_granted;
	BOOL ret;

	r_u->status = NT_STATUS_OK;

	DEBUG(5,("_samr_query_aliasinfo: %d\n", __LINE__));

	/* find the policy handle.  open a policy on it. */
	if (!get_lsa_policy_samr_sid(p, &q_u->pol, &sid, &acc_granted, NULL))
		return NT_STATUS_INVALID_HANDLE;
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, SA_RIGHT_ALIAS_LOOKUP_INFO, "_samr_query_aliasinfo"))) {
		return r_u->status;
	}

	become_root();
	ret = pdb_get_aliasinfo(&sid, &info);
	unbecome_root();
	
	if ( !ret )
		return NT_STATUS_NO_SUCH_ALIAS;

	if ( !(r_u->ctr = TALLOC_ZERO_P( p->mem_ctx, ALIAS_INFO_CTR )) ) 
		return NT_STATUS_NO_MEMORY;


	switch (q_u->level ) {
	case 1:
		r_u->ctr->level = 1;
		init_samr_alias_info1(&r_u->ctr->alias.info1, info.acct_name, 1, info.acct_desc);
		break;
	case 3:
		r_u->ctr->level = 3;
		init_samr_alias_info3(&r_u->ctr->alias.info3, info.acct_desc);
		break;
	default:
		return NT_STATUS_INVALID_INFO_CLASS;
	}

	DEBUG(5,("_samr_query_aliasinfo: %d\n", __LINE__));

	return r_u->status;
}

#if 0
/*******************************************************************
 samr_reply_lookup_ids
 ********************************************************************/

 uint32 _samr_lookup_ids(pipes_struct *p, SAMR_Q_LOOKUP_IDS *q_u, SAMR_R_LOOKUP_IDS *r_u)
{
    uint32 rid[MAX_SAM_ENTRIES];
    int num_rids = q_u->num_sids1;

    r_u->status = NT_STATUS_OK;

    DEBUG(5,("_samr_lookup_ids: %d\n", __LINE__));

    if (num_rids > MAX_SAM_ENTRIES) {
        num_rids = MAX_SAM_ENTRIES;
        DEBUG(5,("_samr_lookup_ids: truncating entries to %d\n", num_rids));
    }

#if 0
    int i;
    SMB_ASSERT_ARRAY(q_u->uni_user_name, num_rids);

    for (i = 0; i < num_rids && status == 0; i++)
    {
        struct sam_passwd *sam_pass;
        fstring user_name;


        fstrcpy(user_name, unistrn2(q_u->uni_user_name[i].buffer,
                                    q_u->uni_user_name[i].uni_str_len));

        /* find the user account */
        become_root();
        sam_pass = get_smb21pwd_entry(user_name, 0);
        unbecome_root();

        if (sam_pass == NULL)
        {
            status = 0xC0000000 | NT_STATUS_NO_SUCH_USER;
            rid[i] = 0;
        }
        else
        {
            rid[i] = sam_pass->user_rid;
        }
    }
#endif

    num_rids = 1;
    rid[0] = BUILTIN_ALIAS_RID_USERS;

    init_samr_r_lookup_ids(&r_u, num_rids, rid, NT_STATUS_OK);

    DEBUG(5,("_samr_lookup_ids: %d\n", __LINE__));

    return r_u->status;
}
#endif

/*******************************************************************
 _samr_lookup_names
 ********************************************************************/

NTSTATUS _samr_lookup_names(pipes_struct *p, SAMR_Q_LOOKUP_NAMES *q_u, SAMR_R_LOOKUP_NAMES *r_u)
{
	uint32 rid[MAX_SAM_ENTRIES];
	enum SID_NAME_USE type[MAX_SAM_ENTRIES];
	int i;
	int num_rids = q_u->num_names2;
	DOM_SID pol_sid;
	fstring sid_str;
	uint32  acc_granted;

	r_u->status = NT_STATUS_OK;

	DEBUG(5,("_samr_lookup_names: %d\n", __LINE__));

	ZERO_ARRAY(rid);
	ZERO_ARRAY(type);

	if (!get_lsa_policy_samr_sid(p, &q_u->pol, &pol_sid, &acc_granted, NULL)) {
		init_samr_r_lookup_names(p->mem_ctx, r_u, 0, NULL, NULL, NT_STATUS_OBJECT_TYPE_MISMATCH);
		return r_u->status;
	}
	
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, 0, "_samr_lookup_names"))) { /* Don't know the acc_bits yet */
		return r_u->status;
	}

	if (num_rids > MAX_SAM_ENTRIES) {
		num_rids = MAX_SAM_ENTRIES;
		DEBUG(5,("_samr_lookup_names: truncating entries to %d\n", num_rids));
	}

	DEBUG(5,("_samr_lookup_names: looking name on SID %s\n", sid_to_string(sid_str, &pol_sid)));
	
	for (i = 0; i < num_rids; i++) {
		fstring name;
            	int ret;

	        r_u->status = NT_STATUS_NONE_MAPPED;
	        type[i] = SID_NAME_UNKNOWN;

	        rid [i] = 0xffffffff;

		ret = rpcstr_pull(name, q_u->uni_name[i].buffer, sizeof(name), q_u->uni_name[i].uni_str_len*2, 0);

		if (ret <= 0) {
			continue;
		}

		if (sid_check_is_builtin(&pol_sid)) {
			if (lookup_builtin_name(name, &rid[i])) {
				type[i] = SID_NAME_ALIAS;
			}
		} else {
			lookup_global_sam_name(name, 0, &rid[i], &type[i]);
		}

		if (type[i] != SID_NAME_UNKNOWN) {
			r_u->status = NT_STATUS_OK;
		}
	}

	init_samr_r_lookup_names(p->mem_ctx, r_u, num_rids, rid, (uint32 *)type, r_u->status);

	DEBUG(5,("_samr_lookup_names: %d\n", __LINE__));

	return r_u->status;
}

/*******************************************************************
 _samr_chgpasswd_user
 ********************************************************************/

NTSTATUS _samr_chgpasswd_user(pipes_struct *p, SAMR_Q_CHGPASSWD_USER *q_u, SAMR_R_CHGPASSWD_USER *r_u)
{
	fstring user_name;
	fstring wks;

	DEBUG(5,("_samr_chgpasswd_user: %d\n", __LINE__));

	r_u->status = NT_STATUS_OK;

	rpcstr_pull(user_name, q_u->uni_user_name.buffer, sizeof(user_name), q_u->uni_user_name.uni_str_len*2, 0);
	rpcstr_pull(wks, q_u->uni_dest_host.buffer, sizeof(wks), q_u->uni_dest_host.uni_str_len*2,0);

	DEBUG(5,("samr_chgpasswd_user: user: %s wks: %s\n", user_name, wks));

	/*
	 * Pass the user through the NT -> unix user mapping
	 * function.
	 */
 
	(void)map_username(user_name);
 
	/*
	 * UNIX username case mangling not required, pass_oem_change 
	 * is case insensitive.
	 */

	r_u->status = pass_oem_change(user_name, q_u->lm_newpass.pass, q_u->lm_oldhash.hash,
				q_u->nt_newpass.pass, q_u->nt_oldhash.hash, NULL);

	init_samr_r_chgpasswd_user(r_u, r_u->status);

	DEBUG(5,("_samr_chgpasswd_user: %d\n", __LINE__));

	return r_u->status;
}

/*******************************************************************
 _samr_chgpasswd_user3
 ********************************************************************/

NTSTATUS _samr_chgpasswd_user3(pipes_struct *p, SAMR_Q_CHGPASSWD_USER3 *q_u, SAMR_R_CHGPASSWD_USER3 *r_u)
{
	fstring user_name;
	fstring wks;
	uint32 reject_reason;
	SAM_UNK_INFO_1 *info = NULL;
	SAMR_CHANGE_REJECT *reject = NULL;

	DEBUG(5,("_samr_chgpasswd_user3: %d\n", __LINE__));

	rpcstr_pull(user_name, q_u->uni_user_name.buffer, sizeof(user_name), q_u->uni_user_name.uni_str_len*2, 0);
	rpcstr_pull(wks, q_u->uni_dest_host.buffer, sizeof(wks), q_u->uni_dest_host.uni_str_len*2,0);

	DEBUG(5,("_samr_chgpasswd_user3: user: %s wks: %s\n", user_name, wks));

	/*
	 * Pass the user through the NT -> unix user mapping
	 * function.
	 */
 
	(void)map_username(user_name);
 
	/*
	 * UNIX username case mangling not required, pass_oem_change 
	 * is case insensitive.
	 */

	r_u->status = pass_oem_change(user_name, q_u->lm_newpass.pass, q_u->lm_oldhash.hash,
				      q_u->nt_newpass.pass, q_u->nt_oldhash.hash, &reject_reason);

	if (NT_STATUS_EQUAL(r_u->status, NT_STATUS_PASSWORD_RESTRICTION) || 
	    NT_STATUS_EQUAL(r_u->status, NT_STATUS_ACCOUNT_RESTRICTION)) {

		uint32 min_pass_len,pass_hist,password_properties;
		time_t u_expire, u_min_age;
		NTTIME nt_expire, nt_min_age;
		uint32 account_policy_temp;

		if ((info = TALLOC_ZERO_P(p->mem_ctx, SAM_UNK_INFO_1)) == NULL) {
			return NT_STATUS_NO_MEMORY;
		}

		if ((reject = TALLOC_ZERO_P(p->mem_ctx, SAMR_CHANGE_REJECT)) == NULL) {
			return NT_STATUS_NO_MEMORY;
		}

		ZERO_STRUCTP(info);
		ZERO_STRUCTP(reject);

		become_root();

		/* AS ROOT !!! */

		pdb_get_account_policy(AP_MIN_PASSWORD_LEN, &account_policy_temp);
		min_pass_len = account_policy_temp;

		pdb_get_account_policy(AP_PASSWORD_HISTORY, &account_policy_temp);
		pass_hist = account_policy_temp;

		pdb_get_account_policy(AP_USER_MUST_LOGON_TO_CHG_PASS, &account_policy_temp);
		password_properties = account_policy_temp;

		pdb_get_account_policy(AP_MAX_PASSWORD_AGE, &account_policy_temp);
		u_expire = account_policy_temp;

		pdb_get_account_policy(AP_MIN_PASSWORD_AGE, &account_policy_temp);
		u_min_age = account_policy_temp;

		/* !AS ROOT */
		
		unbecome_root();

		unix_to_nt_time_abs(&nt_expire, u_expire);
		unix_to_nt_time_abs(&nt_min_age, u_min_age);

		init_unk_info1(info, (uint16)min_pass_len, (uint16)pass_hist, 
		               password_properties, nt_expire, nt_min_age);

		reject->reject_reason = reject_reason;
	}
	
	init_samr_r_chgpasswd_user3(r_u, r_u->status, reject, info);

	DEBUG(5,("_samr_chgpasswd_user3: %d\n", __LINE__));

	return r_u->status;
}

/*******************************************************************
makes a SAMR_R_LOOKUP_RIDS structure.
********************************************************************/

static BOOL make_samr_lookup_rids(TALLOC_CTX *ctx, uint32 num_names,
				  const char **names, UNIHDR **pp_hdr_name,
				  UNISTR2 **pp_uni_name)
{
	uint32 i;
	UNIHDR *hdr_name=NULL;
	UNISTR2 *uni_name=NULL;

	*pp_uni_name = NULL;
	*pp_hdr_name = NULL;

	if (num_names != 0) {
		hdr_name = TALLOC_ZERO_ARRAY(ctx, UNIHDR, num_names);
		if (hdr_name == NULL)
			return False;

		uni_name = TALLOC_ZERO_ARRAY(ctx,UNISTR2, num_names);
		if (uni_name == NULL)
			return False;
	}

	for (i = 0; i < num_names; i++) {
		DEBUG(10, ("names[%d]:%s\n", i, names[i] && *names[i] ? names[i] : ""));
		init_unistr2(&uni_name[i], names[i], UNI_FLAGS_NONE);
		init_uni_hdr(&hdr_name[i], &uni_name[i]);
	}

	*pp_uni_name = uni_name;
	*pp_hdr_name = hdr_name;

	return True;
}

/*******************************************************************
 _samr_lookup_rids
 ********************************************************************/

NTSTATUS _samr_lookup_rids(pipes_struct *p, SAMR_Q_LOOKUP_RIDS *q_u, SAMR_R_LOOKUP_RIDS *r_u)
{
	const char **names;
	uint32 *attrs = NULL;
	UNIHDR *hdr_name = NULL;
	UNISTR2 *uni_name = NULL;
	DOM_SID pol_sid;
	int num_rids = q_u->num_rids1;
	uint32 acc_granted;
	
	r_u->status = NT_STATUS_OK;

	DEBUG(5,("_samr_lookup_rids: %d\n", __LINE__));

	/* find the policy handle.  open a policy on it. */
	if (!get_lsa_policy_samr_sid(p, &q_u->pol, &pol_sid, &acc_granted, NULL))
		return NT_STATUS_INVALID_HANDLE;

	if (num_rids > 1000) {
		DEBUG(0, ("Got asked for %d rids (more than 1000) -- according "
			  "to samba4 idl this is not possible\n", num_rids));
		return NT_STATUS_UNSUCCESSFUL;
	}

	names = TALLOC_ZERO_ARRAY(p->mem_ctx, const char *, num_rids);
	attrs = TALLOC_ZERO_ARRAY(p->mem_ctx, uint32, num_rids);

	if ((num_rids != 0) && ((names == NULL) || (attrs == NULL)))
		return NT_STATUS_NO_MEMORY;

	become_root();  /* lookup_sid can require root privs */
	r_u->status = pdb_lookup_rids(&pol_sid, num_rids, q_u->rid,
				      names, attrs);
	unbecome_root();

	if ( NT_STATUS_EQUAL(r_u->status, NT_STATUS_NONE_MAPPED) && (num_rids == 0) ) {
		r_u->status = NT_STATUS_OK;
	}

	if(!make_samr_lookup_rids(p->mem_ctx, num_rids, names,
				  &hdr_name, &uni_name))
		return NT_STATUS_NO_MEMORY;

	init_samr_r_lookup_rids(r_u, num_rids, hdr_name, uni_name, attrs);

	DEBUG(5,("_samr_lookup_rids: %d\n", __LINE__));

	return r_u->status;
}

/*******************************************************************
 _samr_open_user. Safe - gives out no passwd info.
 ********************************************************************/

NTSTATUS _samr_open_user(pipes_struct *p, SAMR_Q_OPEN_USER *q_u, SAMR_R_OPEN_USER *r_u)
{
	struct samu *sampass=NULL;
	DOM_SID sid;
	POLICY_HND domain_pol = q_u->domain_pol;
	POLICY_HND *user_pol = &r_u->user_pol;
	struct samr_info *info = NULL;
	SEC_DESC *psd = NULL;
	uint32    acc_granted;
	uint32    des_access = q_u->access_mask;
	size_t    sd_size;
	BOOL ret;
	NTSTATUS nt_status;
	SE_PRIV se_rights;

	r_u->status = NT_STATUS_OK;

	/* find the domain policy handle and get domain SID / access bits in the domain policy. */
	
	if ( !get_lsa_policy_samr_sid(p, &domain_pol, &sid, &acc_granted, NULL) )
		return NT_STATUS_INVALID_HANDLE;
	
	nt_status = access_check_samr_function( acc_granted, 
		SA_RIGHT_DOMAIN_OPEN_ACCOUNT, "_samr_open_user" );
		
	if ( !NT_STATUS_IS_OK(nt_status) )
		return nt_status;

	if ( !(sampass = samu_new( p->mem_ctx )) ) {
		return NT_STATUS_NO_MEMORY;
	}

	/* append the user's RID to it */
	
	if (!sid_append_rid(&sid, q_u->user_rid))
		return NT_STATUS_NO_SUCH_USER;
	
	/* check if access can be granted as requested by client. */
	
	make_samr_object_sd(p->mem_ctx, &psd, &sd_size, &usr_generic_mapping, &sid, SAMR_USR_RIGHTS_WRITE_PW);
	se_map_generic(&des_access, &usr_generic_mapping);
	
	se_priv_copy( &se_rights, &se_machine_account );
	se_priv_add( &se_rights, &se_add_users );
	
	nt_status = access_check_samr_object(psd, p->pipe_user.nt_user_token, 
		&se_rights, GENERIC_RIGHTS_USER_WRITE, des_access, 
		&acc_granted, "_samr_open_user");
		
	if ( !NT_STATUS_IS_OK(nt_status) )
		return nt_status;

	become_root();
	ret=pdb_getsampwsid(sampass, &sid);
	unbecome_root();

	/* check that the SID exists in our domain. */
	if (ret == False) {
        	return NT_STATUS_NO_SUCH_USER;
	}

	TALLOC_FREE(sampass);

	/* associate the user's SID and access bits with the new handle. */
	if ((info = get_samr_info_by_sid(&sid)) == NULL)
		return NT_STATUS_NO_MEMORY;
	info->acc_granted = acc_granted;

	/* get a (unique) handle.  open a policy on it. */
	if (!create_policy_hnd(p, user_pol, free_samr_info, (void *)info))
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;

	return r_u->status;
}

/*************************************************************************
 get_user_info_7. Safe. Only gives out account_name.
 *************************************************************************/

static NTSTATUS get_user_info_7(TALLOC_CTX *mem_ctx, SAM_USER_INFO_7 *id7, DOM_SID *user_sid)
{
	struct samu *smbpass=NULL;
	BOOL ret;

	if ( !(smbpass = samu_new( mem_ctx )) ) {
		return NT_STATUS_NO_MEMORY;
	}
	
	become_root();
	ret = pdb_getsampwsid(smbpass, user_sid);
	unbecome_root();

	if ( !ret ) {
		DEBUG(4,("User %s not found\n", sid_string_static(user_sid)));
		return NT_STATUS_NO_SUCH_USER;
	}

	DEBUG(3,("User:[%s]\n", pdb_get_username(smbpass) ));

	ZERO_STRUCTP(id7);
	init_sam_user_info7(id7, pdb_get_username(smbpass) );

	TALLOC_FREE(smbpass);

	return NT_STATUS_OK;
}

/*************************************************************************
 get_user_info_9. Only gives out primary group SID.
 *************************************************************************/
static NTSTATUS get_user_info_9(TALLOC_CTX *mem_ctx, SAM_USER_INFO_9 * id9, DOM_SID *user_sid)
{
	struct samu *smbpass=NULL;
	BOOL ret;

	if ( !(smbpass = samu_new( mem_ctx )) ) {
		return NT_STATUS_NO_MEMORY;
	}

	become_root();
	ret = pdb_getsampwsid(smbpass, user_sid);
	unbecome_root();

	if (ret==False) {
		DEBUG(4,("User %s not found\n", sid_string_static(user_sid)));
		return NT_STATUS_NO_SUCH_USER;
	}

	DEBUG(3,("User:[%s]\n", pdb_get_username(smbpass) ));

	ZERO_STRUCTP(id9);
	init_sam_user_info9(id9, pdb_get_group_rid(smbpass) );

	TALLOC_FREE(smbpass);

	return NT_STATUS_OK;
}

/*************************************************************************
 get_user_info_16. Safe. Only gives out acb bits.
 *************************************************************************/

static NTSTATUS get_user_info_16(TALLOC_CTX *mem_ctx, SAM_USER_INFO_16 *id16, DOM_SID *user_sid)
{
	struct samu *smbpass=NULL;
	BOOL ret;

	if ( !(smbpass = samu_new( mem_ctx )) ) {
		return NT_STATUS_NO_MEMORY;
	}

	become_root();
	ret = pdb_getsampwsid(smbpass, user_sid);
	unbecome_root();

	if (ret==False) {
		DEBUG(4,("User %s not found\n", sid_string_static(user_sid)));
		return NT_STATUS_NO_SUCH_USER;
	}

	DEBUG(3,("User:[%s]\n", pdb_get_username(smbpass) ));

	ZERO_STRUCTP(id16);
	init_sam_user_info16(id16, pdb_get_acct_ctrl(smbpass) );

	TALLOC_FREE(smbpass);

	return NT_STATUS_OK;
}

/*************************************************************************
 get_user_info_18. OK - this is the killer as it gives out password info.
 Ensure that this is only allowed on an encrypted connection with a root
 user. JRA. 
 *************************************************************************/

static NTSTATUS get_user_info_18(pipes_struct *p, TALLOC_CTX *mem_ctx, SAM_USER_INFO_18 * id18, DOM_SID *user_sid)
{
	struct samu *smbpass=NULL;
	BOOL ret;

	if (p->auth.auth_type != PIPE_AUTH_TYPE_NTLMSSP || p->auth.auth_type != PIPE_AUTH_TYPE_SPNEGO_NTLMSSP) {
		return NT_STATUS_ACCESS_DENIED;
	}

	if (p->auth.auth_level != PIPE_AUTH_LEVEL_PRIVACY) {
		return NT_STATUS_ACCESS_DENIED;
	}

	/*
	 * Do *NOT* do become_root()/unbecome_root() here ! JRA.
	 */

	if ( !(smbpass = samu_new( mem_ctx )) ) {
		return NT_STATUS_NO_MEMORY;
	}

	ret = pdb_getsampwsid(smbpass, user_sid);

	if (ret == False) {
		DEBUG(4, ("User %s not found\n", sid_string_static(user_sid)));
		TALLOC_FREE(smbpass);
		return (geteuid() == (uid_t)0) ? NT_STATUS_NO_SUCH_USER : NT_STATUS_ACCESS_DENIED;
	}

	DEBUG(3,("User:[%s] 0x%x\n", pdb_get_username(smbpass), pdb_get_acct_ctrl(smbpass) ));

	if ( pdb_get_acct_ctrl(smbpass) & ACB_DISABLED) {
		TALLOC_FREE(smbpass);
		return NT_STATUS_ACCOUNT_DISABLED;
	}

	ZERO_STRUCTP(id18);
	init_sam_user_info18(id18, pdb_get_lanman_passwd(smbpass), pdb_get_nt_passwd(smbpass));
	
	TALLOC_FREE(smbpass);

	return NT_STATUS_OK;
}

/*************************************************************************
 get_user_info_20
 *************************************************************************/

static NTSTATUS get_user_info_20(TALLOC_CTX *mem_ctx, SAM_USER_INFO_20 *id20, DOM_SID *user_sid)
{
	struct samu *sampass=NULL;
	BOOL ret;

	if ( !(sampass = samu_new( mem_ctx )) ) {
		return NT_STATUS_NO_MEMORY;
	}

	become_root();
	ret = pdb_getsampwsid(sampass, user_sid);
	unbecome_root();

	if (ret == False) {
		DEBUG(4,("User %s not found\n", sid_string_static(user_sid)));
		return NT_STATUS_NO_SUCH_USER;
	}

	samr_clear_sam_passwd(sampass);

	DEBUG(3,("User:[%s]\n",  pdb_get_username(sampass) ));

	ZERO_STRUCTP(id20);
	init_sam_user_info20A(id20, sampass);
	
	TALLOC_FREE(sampass);

	return NT_STATUS_OK;
}

/*************************************************************************
 get_user_info_21
 *************************************************************************/

static NTSTATUS get_user_info_21(TALLOC_CTX *mem_ctx, SAM_USER_INFO_21 *id21, 
				 DOM_SID *user_sid, DOM_SID *domain_sid)
{
	struct samu *sampass=NULL;
	BOOL ret;
	NTSTATUS nt_status;

	if ( !(sampass = samu_new( mem_ctx )) ) {
		return NT_STATUS_NO_MEMORY;
	}

	become_root();
	ret = pdb_getsampwsid(sampass, user_sid);
	unbecome_root();

	if (ret == False) {
		DEBUG(4,("User %s not found\n", sid_string_static(user_sid)));
		return NT_STATUS_NO_SUCH_USER;
	}

	samr_clear_sam_passwd(sampass);

	DEBUG(3,("User:[%s]\n",  pdb_get_username(sampass) ));

	ZERO_STRUCTP(id21);
	nt_status = init_sam_user_info21A(id21, sampass, domain_sid);
	
	TALLOC_FREE(sampass);

	return nt_status;
}

/*******************************************************************
 _samr_query_userinfo
 ********************************************************************/

NTSTATUS _samr_query_userinfo(pipes_struct *p, SAMR_Q_QUERY_USERINFO *q_u, SAMR_R_QUERY_USERINFO *r_u)
{
	SAM_USERINFO_CTR *ctr;
	struct samr_info *info = NULL;
	DOM_SID domain_sid;
	uint32 rid;
	
	r_u->status=NT_STATUS_OK;

	/* search for the handle */
	if (!find_policy_by_hnd(p, &q_u->pol, (void **)(void *)&info))
		return NT_STATUS_INVALID_HANDLE;

	domain_sid = info->sid;

	sid_split_rid(&domain_sid, &rid);

	if (!sid_check_is_in_our_domain(&info->sid))
		return NT_STATUS_OBJECT_TYPE_MISMATCH;

	DEBUG(5,("_samr_query_userinfo: sid:%s\n", sid_string_static(&info->sid)));

	ctr = TALLOC_ZERO_P(p->mem_ctx, SAM_USERINFO_CTR);
	if (!ctr)
		return NT_STATUS_NO_MEMORY;

	ZERO_STRUCTP(ctr);

	/* ok!  user info levels (lots: see MSDEV help), off we go... */
	ctr->switch_value = q_u->switch_value;

	DEBUG(5,("_samr_query_userinfo: user info level: %d\n", q_u->switch_value));

	switch (q_u->switch_value) {
	case 7:
		ctr->info.id7 = TALLOC_ZERO_P(p->mem_ctx, SAM_USER_INFO_7);
		if (ctr->info.id7 == NULL)
			return NT_STATUS_NO_MEMORY;

		if (!NT_STATUS_IS_OK(r_u->status = get_user_info_7(p->mem_ctx, ctr->info.id7, &info->sid)))
			return r_u->status;
		break;
	case 9:
		ctr->info.id9 = TALLOC_ZERO_P(p->mem_ctx, SAM_USER_INFO_9);
		if (ctr->info.id9 == NULL)
			return NT_STATUS_NO_MEMORY;

		if (!NT_STATUS_IS_OK(r_u->status = get_user_info_9(p->mem_ctx, ctr->info.id9, &info->sid)))
			return r_u->status;
		break;
	case 16:
		ctr->info.id16 = TALLOC_ZERO_P(p->mem_ctx, SAM_USER_INFO_16);
		if (ctr->info.id16 == NULL)
			return NT_STATUS_NO_MEMORY;

		if (!NT_STATUS_IS_OK(r_u->status = get_user_info_16(p->mem_ctx, ctr->info.id16, &info->sid)))
			return r_u->status;
		break;

	case 18:
		ctr->info.id18 = TALLOC_ZERO_P(p->mem_ctx, SAM_USER_INFO_18);
		if (ctr->info.id18 == NULL)
			return NT_STATUS_NO_MEMORY;

		if (!NT_STATUS_IS_OK(r_u->status = get_user_info_18(p, p->mem_ctx, ctr->info.id18, &info->sid)))
			return r_u->status;
		break;
		
	case 20:
		ctr->info.id20 = TALLOC_ZERO_P(p->mem_ctx,SAM_USER_INFO_20);
		if (ctr->info.id20 == NULL)
			return NT_STATUS_NO_MEMORY;
		if (!NT_STATUS_IS_OK(r_u->status = get_user_info_20(p->mem_ctx, ctr->info.id20, &info->sid)))
			return r_u->status;
		break;

	case 21:
		ctr->info.id21 = TALLOC_ZERO_P(p->mem_ctx,SAM_USER_INFO_21);
		if (ctr->info.id21 == NULL)
			return NT_STATUS_NO_MEMORY;
		if (!NT_STATUS_IS_OK(r_u->status = get_user_info_21(p->mem_ctx, ctr->info.id21, 
								    &info->sid, &domain_sid)))
			return r_u->status;
		break;

	default:
		return NT_STATUS_INVALID_INFO_CLASS;
	}

	init_samr_r_query_userinfo(r_u, ctr, r_u->status);

	DEBUG(5,("_samr_query_userinfo: %d\n", __LINE__));
	
	return r_u->status;
}

/*******************************************************************
 samr_reply_query_usergroups
 ********************************************************************/

NTSTATUS _samr_query_usergroups(pipes_struct *p, SAMR_Q_QUERY_USERGROUPS *q_u, SAMR_R_QUERY_USERGROUPS *r_u)
{
	struct samu *sam_pass=NULL;
	DOM_SID  sid;
	DOM_SID *sids;
	DOM_GID dom_gid;
	DOM_GID *gids = NULL;
	uint32 primary_group_rid;
	size_t num_groups = 0;
	gid_t *unix_gids;
	size_t i, num_gids;
	uint32 acc_granted;
	BOOL ret;
	NTSTATUS result;

	/*
	 * from the SID in the request:
	 * we should send back the list of DOMAIN GROUPS
	 * the user is a member of
	 *
	 * and only the DOMAIN GROUPS
	 * no ALIASES !!! neither aliases of the domain
	 * nor aliases of the builtin SID
	 *
	 * JFM, 12/2/2001
	 */

	r_u->status = NT_STATUS_OK;

	DEBUG(5,("_samr_query_usergroups: %d\n", __LINE__));

	/* find the policy handle.  open a policy on it. */
	if (!get_lsa_policy_samr_sid(p, &q_u->pol, &sid, &acc_granted, NULL))
		return NT_STATUS_INVALID_HANDLE;
	
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, SA_RIGHT_USER_GET_GROUPS, "_samr_query_usergroups"))) {
		return r_u->status;
	}

	if (!sid_check_is_in_our_domain(&sid))
		return NT_STATUS_OBJECT_TYPE_MISMATCH;

        if ( !(sam_pass = samu_new( p->mem_ctx )) ) {
                return NT_STATUS_NO_MEMORY;
        }

	become_root();
	ret = pdb_getsampwsid(sam_pass, &sid);
	unbecome_root();

	if (!ret) {
		DEBUG(10, ("pdb_getsampwsid failed for %s\n",
			   sid_string_static(&sid)));
		return NT_STATUS_NO_SUCH_USER;
	}

	sids = NULL;

	become_root();
	result = pdb_enum_group_memberships(p->mem_ctx, sam_pass,
					    &sids, &unix_gids, &num_groups);
	unbecome_root();

	if (!NT_STATUS_IS_OK(result)) {
		DEBUG(10, ("pdb_enum_group_memberships failed for %s\n",
			   sid_string_static(&sid)));
		return result;
	}

	gids = NULL;
	num_gids = 0;

	dom_gid.attr = (SE_GROUP_MANDATORY|SE_GROUP_ENABLED_BY_DEFAULT|
			SE_GROUP_ENABLED);

	if (!sid_peek_check_rid(get_global_sam_sid(),
				pdb_get_group_sid(sam_pass),
				&primary_group_rid)) {
		DEBUG(5, ("Group sid %s for user %s not in our domain\n",
			  sid_string_static(pdb_get_group_sid(sam_pass)),
			  pdb_get_username(sam_pass)));
		TALLOC_FREE(sam_pass);
		return NT_STATUS_INTERNAL_DB_CORRUPTION;
	}

	dom_gid.g_rid = primary_group_rid;

	ADD_TO_ARRAY(p->mem_ctx, DOM_GID, dom_gid, &gids, &num_gids);

	for (i=0; i<num_groups; i++) {

		if (!sid_peek_check_rid(get_global_sam_sid(),
					&(sids[i]), &dom_gid.g_rid)) {
			DEBUG(10, ("Found sid %s not in our domain\n",
				   sid_string_static(&sids[i])));
			continue;
		}

		if (dom_gid.g_rid == primary_group_rid) {
			/* We added the primary group directly from the
			 * sam_account. The other SIDs are unique from
			 * enum_group_memberships */
			continue;
		}

		ADD_TO_ARRAY(p->mem_ctx, DOM_GID, dom_gid, &gids, &num_gids);
	}
	
	/* construct the response.  lkclXXXX: gids are not copied! */
	init_samr_r_query_usergroups(r_u, num_gids, gids, r_u->status);
	
	DEBUG(5,("_samr_query_usergroups: %d\n", __LINE__));
	
	return r_u->status;
}

/*******************************************************************
 _samr_query_domain_info
 ********************************************************************/

NTSTATUS _samr_query_domain_info(pipes_struct *p, 
				 SAMR_Q_QUERY_DOMAIN_INFO *q_u, 
				 SAMR_R_QUERY_DOMAIN_INFO *r_u)
{
	struct samr_info *info = NULL;
	SAM_UNK_CTR *ctr;
	uint32 min_pass_len,pass_hist,password_properties;
	time_t u_expire, u_min_age;
	NTTIME nt_expire, nt_min_age;

	time_t u_lock_duration, u_reset_time;
	NTTIME nt_lock_duration, nt_reset_time;
	uint32 lockout;
	time_t u_logout;
	NTTIME nt_logout;

	uint32 account_policy_temp;

	time_t seq_num;
	uint32 server_role;

	uint32 num_users=0, num_groups=0, num_aliases=0;

	if ((ctr = TALLOC_ZERO_P(p->mem_ctx, SAM_UNK_CTR)) == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	ZERO_STRUCTP(ctr);

	r_u->status = NT_STATUS_OK;
	
	DEBUG(5,("_samr_query_domain_info: %d\n", __LINE__));
	
	/* find the policy handle.  open a policy on it. */
	if (!find_policy_by_hnd(p, &q_u->domain_pol, (void **)(void *)&info)) {
		return NT_STATUS_INVALID_HANDLE;
	}
	
	switch (q_u->switch_value) {
		case 0x01:
			
			become_root();

			/* AS ROOT !!! */

			pdb_get_account_policy(AP_MIN_PASSWORD_LEN, &account_policy_temp);
			min_pass_len = account_policy_temp;

			pdb_get_account_policy(AP_PASSWORD_HISTORY, &account_policy_temp);
			pass_hist = account_policy_temp;

			pdb_get_account_policy(AP_USER_MUST_LOGON_TO_CHG_PASS, &account_policy_temp);
			password_properties = account_policy_temp;

			pdb_get_account_policy(AP_MAX_PASSWORD_AGE, &account_policy_temp);
			u_expire = account_policy_temp;

			pdb_get_account_policy(AP_MIN_PASSWORD_AGE, &account_policy_temp);
			u_min_age = account_policy_temp;

			/* !AS ROOT */
			
			unbecome_root();

			unix_to_nt_time_abs(&nt_expire, u_expire);
			unix_to_nt_time_abs(&nt_min_age, u_min_age);

			init_unk_info1(&ctr->info.inf1, (uint16)min_pass_len, (uint16)pass_hist, 
			               password_properties, nt_expire, nt_min_age);
			break;
		case 0x02:

			become_root();

			/* AS ROOT !!! */

			num_users = count_sam_users(info->disp_info, ACB_NORMAL);
			num_groups = count_sam_groups(info->disp_info);
			num_aliases = count_sam_aliases(info->disp_info);

			pdb_get_account_policy(AP_TIME_TO_LOGOUT, &account_policy_temp);
			u_logout = account_policy_temp;

			unix_to_nt_time_abs(&nt_logout, u_logout);

			if (!pdb_get_seq_num(&seq_num))
				seq_num = time(NULL);

			/* !AS ROOT */
			
			unbecome_root();

			server_role = ROLE_DOMAIN_PDC;
			if (lp_server_role() == ROLE_DOMAIN_BDC)
				server_role = ROLE_DOMAIN_BDC;

			init_unk_info2(&ctr->info.inf2, lp_serverstring(), lp_workgroup(), global_myname(), seq_num, 
				       num_users, num_groups, num_aliases, nt_logout, server_role);
			break;
		case 0x03:

			become_root();

			/* AS ROOT !!! */

			{
				uint32 ul;
				pdb_get_account_policy(AP_TIME_TO_LOGOUT, &ul);
				u_logout = (time_t)ul;
			}

			/* !AS ROOT */
			
			unbecome_root();

			unix_to_nt_time_abs(&nt_logout, u_logout);
			
			init_unk_info3(&ctr->info.inf3, nt_logout);
			break;
		case 0x04:
			init_unk_info4(&ctr->info.inf4, lp_serverstring());
			break;
		case 0x05:
			init_unk_info5(&ctr->info.inf5, get_global_sam_name());
			break;
		case 0x06:
			/* NT returns its own name when a PDC. win2k and later
			 * only the name of the PDC if itself is a BDC (samba4
			 * idl) */
			init_unk_info6(&ctr->info.inf6, global_myname());
			break;
		case 0x07:
			server_role = ROLE_DOMAIN_PDC;
			if (lp_server_role() == ROLE_DOMAIN_BDC)
				server_role = ROLE_DOMAIN_BDC;

			init_unk_info7(&ctr->info.inf7, server_role);
			break;
		case 0x08:

			become_root();

			/* AS ROOT !!! */

			if (!pdb_get_seq_num(&seq_num)) {
				seq_num = time(NULL);
			}

			/* !AS ROOT */
			
			unbecome_root();

			init_unk_info8(&ctr->info.inf8, (uint32) seq_num);
			break;
		case 0x0c:

			become_root();

			/* AS ROOT !!! */

			pdb_get_account_policy(AP_LOCK_ACCOUNT_DURATION, &account_policy_temp);
			u_lock_duration = account_policy_temp;
			if (u_lock_duration != -1) {
				u_lock_duration *= 60;
			}

			pdb_get_account_policy(AP_RESET_COUNT_TIME, &account_policy_temp);
			u_reset_time = account_policy_temp * 60;

			pdb_get_account_policy(AP_BAD_ATTEMPT_LOCKOUT, &account_policy_temp);
			lockout = account_policy_temp;

			/* !AS ROOT */
			
			unbecome_root();

			unix_to_nt_time_abs(&nt_lock_duration, u_lock_duration);
			unix_to_nt_time_abs(&nt_reset_time, u_reset_time);
	
            		init_unk_info12(&ctr->info.inf12, nt_lock_duration, nt_reset_time, (uint16)lockout);
            		break;
        	default:
            		return NT_STATUS_INVALID_INFO_CLASS;
		}
	

	init_samr_r_query_domain_info(r_u, q_u->switch_value, ctr, NT_STATUS_OK);
	
	DEBUG(5,("_samr_query_domain_info: %d\n", __LINE__));
	
	return r_u->status;
}

/* W2k3 seems to use the same check for all 3 objects that can be created via
 * SAMR, if you try to create for example "Dialup" as an alias it says
 * "NT_STATUS_USER_EXISTS". This is racy, but we can't really lock the user
 * database. */

static NTSTATUS can_create(TALLOC_CTX *mem_ctx, const char *new_name)
{
	enum SID_NAME_USE type;
	BOOL result;

	DEBUG(10, ("Checking whether [%s] can be created\n", new_name));

	become_root();
	/* Lookup in our local databases (only LOOKUP_NAME_ISOLATED set)
	 * whether the name already exists */
	result = lookup_name(mem_ctx, new_name, LOOKUP_NAME_ISOLATED,
			     NULL, NULL, NULL, &type);
	unbecome_root();

	if (!result) {
		DEBUG(10, ("%s does not exist, can create it\n", new_name));
		return NT_STATUS_OK;
	}

	DEBUG(5, ("trying to create %s, exists as %s\n",
		  new_name, sid_type_lookup(type)));

	if (type == SID_NAME_DOM_GRP) {
		return NT_STATUS_GROUP_EXISTS;
	}
	if (type == SID_NAME_ALIAS) {
		return NT_STATUS_ALIAS_EXISTS;
	}

	/* Yes, the default is NT_STATUS_USER_EXISTS */
	return NT_STATUS_USER_EXISTS;
}

/*******************************************************************
 _samr_create_user
 Create an account, can be either a normal user or a machine.
 This funcion will need to be updated for bdc/domain trusts.
 ********************************************************************/

NTSTATUS _samr_create_user(pipes_struct *p, SAMR_Q_CREATE_USER *q_u,
			   SAMR_R_CREATE_USER *r_u)
{
	char *account;
	DOM_SID sid;
	POLICY_HND dom_pol = q_u->domain_pol;
	uint16 acb_info = q_u->acb_info;
	POLICY_HND *user_pol = &r_u->user_pol;
	struct samr_info *info = NULL;
	NTSTATUS nt_status;
	uint32 acc_granted;
	SEC_DESC *psd;
	size_t    sd_size;
	/* check this, when giving away 'add computer to domain' privs */
	uint32    des_access = GENERIC_RIGHTS_USER_ALL_ACCESS;
	BOOL can_add_account = False;
	SE_PRIV se_rights;
	DISP_INFO *disp_info = NULL;

	/* Get the domain SID stored in the domain policy */
	if (!get_lsa_policy_samr_sid(p, &dom_pol, &sid, &acc_granted,
				     &disp_info))
		return NT_STATUS_INVALID_HANDLE;

	nt_status = access_check_samr_function(acc_granted,
					       SA_RIGHT_DOMAIN_CREATE_USER,
					       "_samr_create_user");
	if (!NT_STATUS_IS_OK(nt_status)) {
		return nt_status;
	}

	if (!(acb_info == ACB_NORMAL || acb_info == ACB_DOMTRUST ||
	      acb_info == ACB_WSTRUST || acb_info == ACB_SVRTRUST)) { 
		/* Match Win2k, and return NT_STATUS_INVALID_PARAMETER if 
		   this parameter is not an account type */
		return NT_STATUS_INVALID_PARAMETER;
	}

	account = rpcstr_pull_unistr2_talloc(p->mem_ctx, &q_u->uni_name);
	if (account == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	nt_status = can_create(p->mem_ctx, account);
	if (!NT_STATUS_IS_OK(nt_status)) {
		return nt_status;
	}

	/* determine which user right we need to check based on the acb_info */
	
	if ( acb_info & ACB_WSTRUST )
	{
		se_priv_copy( &se_rights, &se_machine_account );
		can_add_account = user_has_privileges(
			p->pipe_user.nt_user_token, &se_rights );
	} 
	/* usrmgr.exe (and net rpc trustdom grant) creates a normal user 
	   account for domain trusts and changes the ACB flags later */
	else if ( acb_info & ACB_NORMAL &&
		  (account[strlen(account)-1] != '$') )
	{
		se_priv_copy( &se_rights, &se_add_users );
		can_add_account = user_has_privileges(
			p->pipe_user.nt_user_token, &se_rights );
	} 
	else 	/* implicit assumption of a BDC or domain trust account here
		 * (we already check the flags earlier) */
	{
		if ( lp_enable_privileges() ) {
			/* only Domain Admins can add a BDC or domain trust */
			se_priv_copy( &se_rights, &se_priv_none );
			can_add_account = nt_token_check_domain_rid(
				p->pipe_user.nt_user_token,
				DOMAIN_GROUP_RID_ADMINS );
		}
	}
		
	DEBUG(5, ("_samr_create_user: %s can add this account : %s\n",
		p->pipe_user_name, can_add_account ? "True":"False" ));
		
	/********** BEGIN Admin BLOCK **********/

	if ( can_add_account )
		become_root();

	nt_status = pdb_create_user(p->mem_ctx, account, acb_info,
				    &r_u->user_rid);

	if ( can_add_account )
		unbecome_root();

	/********** END Admin BLOCK **********/
	
	/* now check for failure */
	
	if ( !NT_STATUS_IS_OK(nt_status) )
		return nt_status;
			
	/* Get the user's SID */

	sid_compose(&sid, get_global_sam_sid(), r_u->user_rid);
	
	make_samr_object_sd(p->mem_ctx, &psd, &sd_size, &usr_generic_mapping,
			    &sid, SAMR_USR_RIGHTS_WRITE_PW);
	se_map_generic(&des_access, &usr_generic_mapping);
	
	nt_status = access_check_samr_object(psd, p->pipe_user.nt_user_token, 
		&se_rights, GENERIC_RIGHTS_USER_WRITE, des_access, 
		&acc_granted, "_samr_create_user");
		
	if ( !NT_STATUS_IS_OK(nt_status) ) {
		return nt_status;
	}

	/* associate the user's SID with the new handle. */
	if ((info = get_samr_info_by_sid(&sid)) == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	ZERO_STRUCTP(info);
	info->sid = sid;
	info->acc_granted = acc_granted;

	/* get a (unique) handle.  open a policy on it. */
	if (!create_policy_hnd(p, user_pol, free_samr_info, (void *)info)) {
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;
	}

	/* After a "set" ensure we have no cached display info. */
	force_flush_samr_cache(info->disp_info);

	r_u->access_granted = acc_granted;

	return NT_STATUS_OK;
}

/*******************************************************************
 samr_reply_connect_anon
 ********************************************************************/

NTSTATUS _samr_connect_anon(pipes_struct *p, SAMR_Q_CONNECT_ANON *q_u, SAMR_R_CONNECT_ANON *r_u)
{
	struct samr_info *info = NULL;
	uint32    des_access = q_u->access_mask;

	/* Access check */

	if (!pipe_access_check(p)) {
		DEBUG(3, ("access denied to samr_connect_anon\n"));
		r_u->status = NT_STATUS_ACCESS_DENIED;
		return r_u->status;
	}

	/* set up the SAMR connect_anon response */

	r_u->status = NT_STATUS_OK;

	/* associate the user's SID with the new handle. */
	if ((info = get_samr_info_by_sid(NULL)) == NULL)
		return NT_STATUS_NO_MEMORY;

	/* don't give away the farm but this is probably ok.  The SA_RIGHT_SAM_ENUM_DOMAINS
	   was observed from a win98 client trying to enumerate users (when configured  
	   user level access control on shares)   --jerry */
	   
	if (des_access == MAXIMUM_ALLOWED_ACCESS) {
		/* Map to max possible knowing we're filtered below. */
		des_access = GENERIC_ALL_ACCESS;
	}

	se_map_generic( &des_access, &sam_generic_mapping );
	info->acc_granted = des_access & (SA_RIGHT_SAM_ENUM_DOMAINS|SA_RIGHT_SAM_OPEN_DOMAIN);
	
	info->status = q_u->unknown_0;

	/* get a (unique) handle.  open a policy on it. */
	if (!create_policy_hnd(p, &r_u->connect_pol, free_samr_info, (void *)info))
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;

	return r_u->status;
}

/*******************************************************************
 samr_reply_connect
 ********************************************************************/

NTSTATUS _samr_connect(pipes_struct *p, SAMR_Q_CONNECT *q_u, SAMR_R_CONNECT *r_u)
{
	struct samr_info *info = NULL;
	SEC_DESC *psd = NULL;
	uint32    acc_granted;
	uint32    des_access = q_u->access_mask;
	NTSTATUS  nt_status;
	size_t    sd_size;


	DEBUG(5,("_samr_connect: %d\n", __LINE__));

	/* Access check */

	if (!pipe_access_check(p)) {
		DEBUG(3, ("access denied to samr_connect\n"));
		r_u->status = NT_STATUS_ACCESS_DENIED;
		return r_u->status;
	}

	make_samr_object_sd(p->mem_ctx, &psd, &sd_size, &sam_generic_mapping, NULL, 0);
	se_map_generic(&des_access, &sam_generic_mapping);
	
	nt_status = access_check_samr_object(psd, p->pipe_user.nt_user_token, 
		NULL, 0, des_access, &acc_granted, "_samr_connect");
	
	if ( !NT_STATUS_IS_OK(nt_status) ) 
		return nt_status;

	r_u->status = NT_STATUS_OK;

	/* associate the user's SID and access granted with the new handle. */
	if ((info = get_samr_info_by_sid(NULL)) == NULL)
		return NT_STATUS_NO_MEMORY;

	info->acc_granted = acc_granted;
	info->status = q_u->access_mask;

	/* get a (unique) handle.  open a policy on it. */
	if (!create_policy_hnd(p, &r_u->connect_pol, free_samr_info, (void *)info))
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;

	DEBUG(5,("_samr_connect: %d\n", __LINE__));

	return r_u->status;
}

/*******************************************************************
 samr_connect4
 ********************************************************************/

NTSTATUS _samr_connect4(pipes_struct *p, SAMR_Q_CONNECT4 *q_u, SAMR_R_CONNECT4 *r_u)
{
	struct samr_info *info = NULL;
	SEC_DESC *psd = NULL;
	uint32    acc_granted;
	uint32    des_access = q_u->access_mask;
	NTSTATUS  nt_status;
	size_t    sd_size;


	DEBUG(5,("_samr_connect4: %d\n", __LINE__));

	/* Access check */

	if (!pipe_access_check(p)) {
		DEBUG(3, ("access denied to samr_connect4\n"));
		r_u->status = NT_STATUS_ACCESS_DENIED;
		return r_u->status;
	}

	make_samr_object_sd(p->mem_ctx, &psd, &sd_size, &sam_generic_mapping, NULL, 0);
	se_map_generic(&des_access, &sam_generic_mapping);
	
	nt_status = access_check_samr_object(psd, p->pipe_user.nt_user_token, 
		NULL, 0, des_access, &acc_granted, "_samr_connect4");
	
	if ( !NT_STATUS_IS_OK(nt_status) ) 
		return nt_status;

	r_u->status = NT_STATUS_OK;

	/* associate the user's SID and access granted with the new handle. */
	if ((info = get_samr_info_by_sid(NULL)) == NULL)
		return NT_STATUS_NO_MEMORY;

	info->acc_granted = acc_granted;
	info->status = q_u->access_mask;

	/* get a (unique) handle.  open a policy on it. */
	if (!create_policy_hnd(p, &r_u->connect_pol, free_samr_info, (void *)info))
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;

	DEBUG(5,("_samr_connect: %d\n", __LINE__));

	return r_u->status;
}

/*******************************************************************
 samr_connect5
 ********************************************************************/

NTSTATUS _samr_connect5(pipes_struct *p, SAMR_Q_CONNECT5 *q_u, SAMR_R_CONNECT5 *r_u)
{
	struct samr_info *info = NULL;
	SEC_DESC *psd = NULL;
	uint32    acc_granted;
	uint32    des_access = q_u->access_mask;
	NTSTATUS  nt_status;
	POLICY_HND pol;
	size_t    sd_size;


	DEBUG(5,("_samr_connect5: %d\n", __LINE__));

	ZERO_STRUCTP(r_u);

	/* Access check */

	if (!pipe_access_check(p)) {
		DEBUG(3, ("access denied to samr_connect5\n"));
		r_u->status = NT_STATUS_ACCESS_DENIED;
		return r_u->status;
	}

	make_samr_object_sd(p->mem_ctx, &psd, &sd_size, &sam_generic_mapping, NULL, 0);
	se_map_generic(&des_access, &sam_generic_mapping);
	
	nt_status = access_check_samr_object(psd, p->pipe_user.nt_user_token, 
		NULL, 0, des_access, &acc_granted, "_samr_connect5");
	
	if ( !NT_STATUS_IS_OK(nt_status) ) 
		return nt_status;

	/* associate the user's SID and access granted with the new handle. */
	if ((info = get_samr_info_by_sid(NULL)) == NULL)
		return NT_STATUS_NO_MEMORY;

	info->acc_granted = acc_granted;
	info->status = q_u->access_mask;

	/* get a (unique) handle.  open a policy on it. */
	if (!create_policy_hnd(p, &pol, free_samr_info, (void *)info))
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;

	DEBUG(5,("_samr_connect: %d\n", __LINE__));

	init_samr_r_connect5(r_u, &pol, NT_STATUS_OK);

	return r_u->status;
}

/**********************************************************************
 api_samr_lookup_domain
 **********************************************************************/

NTSTATUS _samr_lookup_domain(pipes_struct *p, SAMR_Q_LOOKUP_DOMAIN *q_u, SAMR_R_LOOKUP_DOMAIN *r_u)
{
	struct samr_info *info;
	fstring domain_name;
	DOM_SID sid;

	r_u->status = NT_STATUS_OK;

	if (!find_policy_by_hnd(p, &q_u->connect_pol, (void**)(void *)&info))
		return NT_STATUS_INVALID_HANDLE;

	/* win9x user manager likes to use SA_RIGHT_SAM_ENUM_DOMAINS here.  
	   Reverted that change so we will work with RAS servers again */

	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(info->acc_granted, 
		SA_RIGHT_SAM_OPEN_DOMAIN, "_samr_lookup_domain"))) 
	{
		return r_u->status;
	}

	rpcstr_pull(domain_name, q_u->uni_domain.buffer, sizeof(domain_name), q_u->uni_domain.uni_str_len*2, 0);

	ZERO_STRUCT(sid);

	if (strequal(domain_name, builtin_domain_name())) {
		sid_copy(&sid, &global_sid_Builtin);
	} else {
		if (!secrets_fetch_domain_sid(domain_name, &sid)) {
			r_u->status = NT_STATUS_NO_SUCH_DOMAIN;
		}
	}

	DEBUG(2,("Returning domain sid for domain %s -> %s\n", domain_name, sid_string_static(&sid)));

	init_samr_r_lookup_domain(r_u, &sid, r_u->status);

	return r_u->status;
}

/******************************************************************
makes a SAMR_R_ENUM_DOMAINS structure.
********************************************************************/

static BOOL make_enum_domains(TALLOC_CTX *ctx, SAM_ENTRY **pp_sam,
			UNISTR2 **pp_uni_name, uint32 num_sam_entries, fstring doms[])
{
	uint32 i;
	SAM_ENTRY *sam;
	UNISTR2 *uni_name;

	DEBUG(5, ("make_enum_domains\n"));

	*pp_sam = NULL;
	*pp_uni_name = NULL;

	if (num_sam_entries == 0)
		return True;

	sam = TALLOC_ZERO_ARRAY(ctx, SAM_ENTRY, num_sam_entries);
	uni_name = TALLOC_ZERO_ARRAY(ctx, UNISTR2, num_sam_entries);

	if (sam == NULL || uni_name == NULL)
		return False;

	for (i = 0; i < num_sam_entries; i++) {
		init_unistr2(&uni_name[i], doms[i], UNI_FLAGS_NONE);
		init_sam_entry(&sam[i], &uni_name[i], 0);
	}

	*pp_sam = sam;
	*pp_uni_name = uni_name;

	return True;
}

/**********************************************************************
 api_samr_enum_domains
 **********************************************************************/

NTSTATUS _samr_enum_domains(pipes_struct *p, SAMR_Q_ENUM_DOMAINS *q_u, SAMR_R_ENUM_DOMAINS *r_u)
{
	struct samr_info *info;
	uint32 num_entries = 2;
	fstring dom[2];
	const char *name;

	r_u->status = NT_STATUS_OK;
	
	if (!find_policy_by_hnd(p, &q_u->pol, (void**)(void *)&info))
		return NT_STATUS_INVALID_HANDLE;
	
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(info->acc_granted, SA_RIGHT_SAM_ENUM_DOMAINS, "_samr_enum_domains"))) {
		return r_u->status;
	}

	name = get_global_sam_name();

	fstrcpy(dom[0],name);
	strupper_m(dom[0]);
	fstrcpy(dom[1],"Builtin");

	if (!make_enum_domains(p->mem_ctx, &r_u->sam, &r_u->uni_dom_name, num_entries, dom))
		return NT_STATUS_NO_MEMORY;

	init_samr_r_enum_domains(r_u, q_u->start_idx + num_entries, num_entries);

	return r_u->status;
}

/*******************************************************************
 api_samr_open_alias
 ********************************************************************/

NTSTATUS _samr_open_alias(pipes_struct *p, SAMR_Q_OPEN_ALIAS *q_u, SAMR_R_OPEN_ALIAS *r_u)
{
	DOM_SID sid;
	POLICY_HND domain_pol = q_u->dom_pol;
	uint32 alias_rid = q_u->rid_alias;
	POLICY_HND *alias_pol = &r_u->pol;
	struct    samr_info *info = NULL;
	SEC_DESC *psd = NULL;
	uint32    acc_granted;
	uint32    des_access = q_u->access_mask;
	size_t    sd_size;
	NTSTATUS  status;
	SE_PRIV se_rights;

	r_u->status = NT_STATUS_OK;

	/* find the domain policy and get the SID / access bits stored in the domain policy */
	
	if ( !get_lsa_policy_samr_sid(p, &domain_pol, &sid, &acc_granted, NULL) )
		return NT_STATUS_INVALID_HANDLE;
	
	status = access_check_samr_function(acc_granted, 
		SA_RIGHT_DOMAIN_OPEN_ACCOUNT, "_samr_open_alias");
		
	if ( !NT_STATUS_IS_OK(status) ) 
		return status;

	/* append the alias' RID to it */
	
	if (!sid_append_rid(&sid, alias_rid))
		return NT_STATUS_NO_SUCH_ALIAS;
		
	/*check if access can be granted as requested by client. */
	
	make_samr_object_sd(p->mem_ctx, &psd, &sd_size, &ali_generic_mapping, NULL, 0);
	se_map_generic(&des_access,&ali_generic_mapping);
	
	se_priv_copy( &se_rights, &se_add_users );
	
	
	status = access_check_samr_object(psd, p->pipe_user.nt_user_token, 
		&se_rights, GENERIC_RIGHTS_ALIAS_WRITE, des_access, 
		&acc_granted, "_samr_open_alias");
		
	if ( !NT_STATUS_IS_OK(status) )
		return status;

	{
		/* Check we actually have the requested alias */
		enum SID_NAME_USE type;
		BOOL result;
		gid_t gid;

		become_root();
		result = lookup_sid(NULL, &sid, NULL, NULL, &type);
		unbecome_root();

		if (!result || (type != SID_NAME_ALIAS)) {
			return NT_STATUS_NO_SUCH_ALIAS;
		}

		/* make sure there is a mapping */
		
		if ( !sid_to_gid( &sid, &gid ) ) {
			return NT_STATUS_NO_SUCH_ALIAS;
		}

	}

	/* associate the alias SID with the new handle. */
	if ((info = get_samr_info_by_sid(&sid)) == NULL)
		return NT_STATUS_NO_MEMORY;
		
	info->acc_granted = acc_granted;

	/* get a (unique) handle.  open a policy on it. */
	if (!create_policy_hnd(p, alias_pol, free_samr_info, (void *)info))
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;

	return r_u->status;
}

/*******************************************************************
 set_user_info_7
 ********************************************************************/
static NTSTATUS set_user_info_7(TALLOC_CTX *mem_ctx,
				const SAM_USER_INFO_7 *id7, struct samu *pwd)
{
	fstring new_name;
	NTSTATUS rc;

	if (id7 == NULL) {
		DEBUG(5, ("set_user_info_7: NULL id7\n"));
		TALLOC_FREE(pwd);
		return NT_STATUS_ACCESS_DENIED;
	}

	if(!rpcstr_pull(new_name, id7->uni_name.buffer, sizeof(new_name), id7->uni_name.uni_str_len*2, 0)) {
	        DEBUG(5, ("set_user_info_7: failed to get new username\n"));
		TALLOC_FREE(pwd);
		return NT_STATUS_ACCESS_DENIED;
	}

	/* check to see if the new username already exists.  Note: we can't
	   reliably lock all backends, so there is potentially the 
	   possibility that a user can be created in between this check and
	   the rename.  The rename should fail, but may not get the
	   exact same failure status code.  I think this is small enough
	   of a window for this type of operation and the results are
	   simply that the rename fails with a slightly different status
	   code (like UNSUCCESSFUL instead of ALREADY_EXISTS). */

	rc = can_create(mem_ctx, new_name);
	if (!NT_STATUS_IS_OK(rc)) {
		return rc;
	}

	rc = pdb_rename_sam_account(pwd, new_name);

	TALLOC_FREE(pwd);
	return rc;
}

/*******************************************************************
 set_user_info_16
 ********************************************************************/

static BOOL set_user_info_16(const SAM_USER_INFO_16 *id16, struct samu *pwd)
{
	if (id16 == NULL) {
		DEBUG(5, ("set_user_info_16: NULL id16\n"));
		TALLOC_FREE(pwd);
		return False;
	}
	
	/* FIX ME: check if the value is really changed --metze */
	if (!pdb_set_acct_ctrl(pwd, id16->acb_info, PDB_CHANGED)) {
		TALLOC_FREE(pwd);
		return False;
	}

	if(!NT_STATUS_IS_OK(pdb_update_sam_account(pwd))) {
		TALLOC_FREE(pwd);
		return False;
	}

	TALLOC_FREE(pwd);

	return True;
}

/*******************************************************************
 set_user_info_18
 ********************************************************************/

static BOOL set_user_info_18(SAM_USER_INFO_18 *id18, struct samu *pwd)
{

	if (id18 == NULL) {
		DEBUG(2, ("set_user_info_18: id18 is NULL\n"));
		TALLOC_FREE(pwd);
		return False;
	}
 
	if (!pdb_set_lanman_passwd (pwd, id18->lm_pwd, PDB_CHANGED)) {
		TALLOC_FREE(pwd);
		return False;
	}
	if (!pdb_set_nt_passwd     (pwd, id18->nt_pwd, PDB_CHANGED)) {
		TALLOC_FREE(pwd);
		return False;
	}
 	if (!pdb_set_pass_changed_now (pwd)) {
		TALLOC_FREE(pwd);
		return False; 
	}
 
	if(!NT_STATUS_IS_OK(pdb_update_sam_account(pwd))) {
		TALLOC_FREE(pwd);
		return False;
 	}

	TALLOC_FREE(pwd);
	return True;
}

/*******************************************************************
 set_user_info_20
 ********************************************************************/

static BOOL set_user_info_20(SAM_USER_INFO_20 *id20, struct samu *pwd)
{
	if (id20 == NULL) {
		DEBUG(5, ("set_user_info_20: NULL id20\n"));
		return False;
	}
 
	copy_id20_to_sam_passwd(pwd, id20);

	/* write the change out */
	if(!NT_STATUS_IS_OK(pdb_update_sam_account(pwd))) {
		TALLOC_FREE(pwd);
		return False;
 	}

	TALLOC_FREE(pwd);

	return True;
}
/*******************************************************************
 set_user_info_21
 ********************************************************************/

static NTSTATUS set_user_info_21(TALLOC_CTX *mem_ctx, SAM_USER_INFO_21 *id21,
				 struct samu *pwd)
{
	fstring new_name;
	NTSTATUS status;
	
	if (id21 == NULL) {
		DEBUG(5, ("set_user_info_21: NULL id21\n"));
		return NT_STATUS_INVALID_PARAMETER;
	}

	/* we need to separately check for an account rename first */
	if (rpcstr_pull(new_name, id21->uni_user_name.buffer, 
			sizeof(new_name), id21->uni_user_name.uni_str_len*2, 0) && 
	   (!strequal(new_name, pdb_get_username(pwd)))) {

		/* check to see if the new username already exists.  Note: we can't
		   reliably lock all backends, so there is potentially the 
		   possibility that a user can be created in between this check and
		   the rename.  The rename should fail, but may not get the
		   exact same failure status code.  I think this is small enough
		   of a window for this type of operation and the results are
		   simply that the rename fails with a slightly different status
		   code (like UNSUCCESSFUL instead of ALREADY_EXISTS). */

		status = can_create(mem_ctx, new_name);
		if (!NT_STATUS_IS_OK(status)) {
			return status;
		}

		status = pdb_rename_sam_account(pwd, new_name);

		if (!NT_STATUS_IS_OK(status)) {
			DEBUG(0,("set_user_info_21: failed to rename account: %s\n", 
				nt_errstr(status)));
			TALLOC_FREE(pwd);
			return status;
		}

		/* set the new username so that later 
		   functions can work on the new account */
		pdb_set_username(pwd, new_name, PDB_SET);
	}

	copy_id21_to_sam_passwd(pwd, id21);
 
	/*
	 * The funny part about the previous two calls is
	 * that pwd still has the password hashes from the
	 * passdb entry.  These have not been updated from
	 * id21.  I don't know if they need to be set.    --jerry
	 */
 
	if ( IS_SAM_CHANGED(pwd, PDB_GROUPSID) ) {
		status = pdb_set_unix_primary_group(mem_ctx, pwd);
		if ( !NT_STATUS_IS_OK(status) ) {
			return status;
		}
	}
	
	/* Don't worry about writing out the user account since the
	   primary group SID is generated solely from the user's Unix 
	   primary group. */

	/* write the change out */
	if(!NT_STATUS_IS_OK(status = pdb_update_sam_account(pwd))) {
		TALLOC_FREE(pwd);
		return status;
 	}

	TALLOC_FREE(pwd);

	return NT_STATUS_OK;
}

/*******************************************************************
 set_user_info_23
 ********************************************************************/

static NTSTATUS set_user_info_23(TALLOC_CTX *mem_ctx, SAM_USER_INFO_23 *id23,
				 struct samu *pwd)
{
	pstring plaintext_buf;
	uint32 len;
	uint16 acct_ctrl;
	NTSTATUS status;
 
	if (id23 == NULL) {
		DEBUG(5, ("set_user_info_23: NULL id23\n"));
		return NT_STATUS_INVALID_PARAMETER;
	}
 
	DEBUG(5, ("Attempting administrator password change (level 23) for user %s\n",
		  pdb_get_username(pwd)));

	acct_ctrl = pdb_get_acct_ctrl(pwd);

	if (!decode_pw_buffer(id23->pass, plaintext_buf, 256, &len, STR_UNICODE)) {
		TALLOC_FREE(pwd);
		return NT_STATUS_INVALID_PARAMETER;
 	}
  
	if (!pdb_set_plaintext_passwd (pwd, plaintext_buf)) {
		TALLOC_FREE(pwd);
		return NT_STATUS_ACCESS_DENIED;
	}
 
	copy_id23_to_sam_passwd(pwd, id23);
 
	/* if it's a trust account, don't update /etc/passwd */
	if (    ( (acct_ctrl &  ACB_DOMTRUST) == ACB_DOMTRUST ) ||
		( (acct_ctrl &  ACB_WSTRUST) ==  ACB_WSTRUST) ||
		( (acct_ctrl &  ACB_SVRTRUST) ==  ACB_SVRTRUST) ) {
		DEBUG(5, ("Changing trust account.  Not updating /etc/passwd\n"));
	} else  {
		/* update the UNIX password */
		if (lp_unix_password_sync() ) {
			struct passwd *passwd;
			if (pdb_get_username(pwd) == NULL) {
				DEBUG(1, ("chgpasswd: User without name???\n"));
				TALLOC_FREE(pwd);
				return NT_STATUS_ACCESS_DENIED;
			}

			if ((passwd = Get_Pwnam(pdb_get_username(pwd))) == NULL) {
				DEBUG(1, ("chgpasswd: Username does not exist in system !?!\n"));
			}
			
			if(!chgpasswd(pdb_get_username(pwd), passwd, "", plaintext_buf, True)) {
				TALLOC_FREE(pwd);
				return NT_STATUS_ACCESS_DENIED;
			}
		}
	}
 
	ZERO_STRUCT(plaintext_buf);
 
	if (IS_SAM_CHANGED(pwd, PDB_GROUPSID) &&
	    (!NT_STATUS_IS_OK(status =  pdb_set_unix_primary_group(mem_ctx,
								   pwd)))) {
		TALLOC_FREE(pwd);
		return status;
	}

	if(!NT_STATUS_IS_OK(status = pdb_update_sam_account(pwd))) {
		TALLOC_FREE(pwd);
		return status;
	}
 
	TALLOC_FREE(pwd);

	return NT_STATUS_OK;
}

/*******************************************************************
 set_user_info_pw
 ********************************************************************/

static BOOL set_user_info_pw(uint8 *pass, struct samu *pwd)
{
	uint32 len;
	pstring plaintext_buf;
	uint32 acct_ctrl;
 
	DEBUG(5, ("Attempting administrator password change for user %s\n",
		  pdb_get_username(pwd)));

	acct_ctrl = pdb_get_acct_ctrl(pwd);

	ZERO_STRUCT(plaintext_buf);
 
	if (!decode_pw_buffer(pass, plaintext_buf, 256, &len, STR_UNICODE)) {
		TALLOC_FREE(pwd);
		return False;
 	}

	if (!pdb_set_plaintext_passwd (pwd, plaintext_buf)) {
		TALLOC_FREE(pwd);
		return False;
	}
 
	/* if it's a trust account, don't update /etc/passwd */
	if ( ( (acct_ctrl &  ACB_DOMTRUST) == ACB_DOMTRUST ) ||
		( (acct_ctrl &  ACB_WSTRUST) ==  ACB_WSTRUST) ||
		( (acct_ctrl &  ACB_SVRTRUST) ==  ACB_SVRTRUST) ) {
		DEBUG(5, ("Changing trust account or non-unix-user password, not updating /etc/passwd\n"));
	} else {
		/* update the UNIX password */
		if (lp_unix_password_sync()) {
			struct passwd *passwd;

			if (pdb_get_username(pwd) == NULL) {
				DEBUG(1, ("chgpasswd: User without name???\n"));
				TALLOC_FREE(pwd);
				return False;
			}

			if ((passwd = Get_Pwnam(pdb_get_username(pwd))) == NULL) {
				DEBUG(1, ("chgpasswd: Username does not exist in system !?!\n"));
			}
			
			if(!chgpasswd(pdb_get_username(pwd), passwd, "", plaintext_buf, True)) {
				TALLOC_FREE(pwd);
				return False;
			}
		}
	}
 
	ZERO_STRUCT(plaintext_buf);
 
	DEBUG(5,("set_user_info_pw: pdb_update_pwd()\n"));
 
	/* update the SAMBA password */
	if(!NT_STATUS_IS_OK(pdb_update_sam_account(pwd))) {
		TALLOC_FREE(pwd);
		return False;
 	}

	TALLOC_FREE(pwd);

	return True;
}

/*******************************************************************
 set_user_info_25
 ********************************************************************/

static NTSTATUS set_user_info_25(TALLOC_CTX *mem_ctx, SAM_USER_INFO_25 *id25,
				 struct samu *pwd)
{
	NTSTATUS status;
	
	if (id25 == NULL) {
		DEBUG(5, ("set_user_info_25: NULL id25\n"));
		return NT_STATUS_INVALID_PARAMETER;
	}

	copy_id25_to_sam_passwd(pwd, id25);
 
	/*
	 * The funny part about the previous two calls is
	 * that pwd still has the password hashes from the
	 * passdb entry.  These have not been updated from
	 * id21.  I don't know if they need to be set.    --jerry
	 */
 
	if ( IS_SAM_CHANGED(pwd, PDB_GROUPSID) ) {
		status = pdb_set_unix_primary_group(mem_ctx, pwd);
		if ( !NT_STATUS_IS_OK(status) ) {
			return status;
		}
	}
	
	/* Don't worry about writing out the user account since the
	   primary group SID is generated solely from the user's Unix 
	   primary group. */

	/* write the change out */
	if(!NT_STATUS_IS_OK(status = pdb_update_sam_account(pwd))) {
		TALLOC_FREE(pwd);
		return status;
 	}

	/* WARNING: No TALLOC_FREE(pwd), we are about to set the password
	 * hereafter! */

	return NT_STATUS_OK;
}

/*******************************************************************
 samr_reply_set_userinfo
 ********************************************************************/

NTSTATUS _samr_set_userinfo(pipes_struct *p, SAMR_Q_SET_USERINFO *q_u, SAMR_R_SET_USERINFO *r_u)
{
	struct samu *pwd = NULL;
	DOM_SID sid;
	POLICY_HND *pol = &q_u->pol;
	uint16 switch_value = q_u->switch_value;
	SAM_USERINFO_CTR *ctr = q_u->ctr;
	uint32 acc_granted;
	uint32 acc_required;
	BOOL ret;
	BOOL has_enough_rights = False;
	uint32 acb_info;
	DISP_INFO *disp_info = NULL;

	DEBUG(5, ("_samr_set_userinfo: %d\n", __LINE__));

	r_u->status = NT_STATUS_OK;

	/* find the policy handle.  open a policy on it. */
	if (!get_lsa_policy_samr_sid(p, pol, &sid, &acc_granted, &disp_info))
		return NT_STATUS_INVALID_HANDLE;

	/* This is tricky.  A WinXP domain join sets 
	  (SA_RIGHT_USER_SET_PASSWORD|SA_RIGHT_USER_SET_ATTRIBUTES|SA_RIGHT_USER_ACCT_FLAGS_EXPIRY)
	  The MMC lusrmgr plugin includes these perms and more in the SamrOpenUser().  But the 
	  standard Win32 API calls just ask for SA_RIGHT_USER_SET_PASSWORD in the SamrOpenUser().  
	  This should be enough for levels 18, 24, 25,& 26.  Info level 23 can set more so 
	  we'll use the set from the WinXP join as the basis. */
	
	switch (switch_value) {
	case 18:
	case 24:
	case 25:
	case 26:
		acc_required = SA_RIGHT_USER_SET_PASSWORD;
		break;
	default:
		acc_required = SA_RIGHT_USER_SET_PASSWORD | SA_RIGHT_USER_SET_ATTRIBUTES | SA_RIGHT_USER_ACCT_FLAGS_EXPIRY;
		break;
	}
	
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, acc_required, "_samr_set_userinfo"))) {
		return r_u->status;
	}

	DEBUG(5, ("_samr_set_userinfo: sid:%s, level:%d\n", sid_string_static(&sid), switch_value));

	if (ctr == NULL) {
		DEBUG(5, ("_samr_set_userinfo: NULL info level\n"));
		return NT_STATUS_INVALID_INFO_CLASS;
	}
	
 	if ( !(pwd = samu_new( NULL )) ) {
		return NT_STATUS_NO_MEMORY;
	}
	
	become_root();
	ret = pdb_getsampwsid(pwd, &sid);
	unbecome_root();
	
	if ( !ret ) {
		TALLOC_FREE(pwd);
		return NT_STATUS_NO_SUCH_USER;
 	}
	
	/* deal with machine password changes differently from userinfo changes */
	/* check to see if we have the sufficient rights */
	
	acb_info = pdb_get_acct_ctrl(pwd);
	if ( acb_info & ACB_WSTRUST ) 
		has_enough_rights = user_has_privileges( p->pipe_user.nt_user_token, &se_machine_account);
	else if ( acb_info & ACB_NORMAL )
		has_enough_rights = user_has_privileges( p->pipe_user.nt_user_token, &se_add_users );
	else if ( acb_info & (ACB_SVRTRUST|ACB_DOMTRUST) ) {
		if ( lp_enable_privileges() )
			has_enough_rights = nt_token_check_domain_rid( p->pipe_user.nt_user_token, DOMAIN_GROUP_RID_ADMINS );
	}
	
	DEBUG(5, ("_samr_set_userinfo: %s does%s possess sufficient rights\n",
		p->pipe_user_name, has_enough_rights ? "" : " not"));

	/* ================ BEGIN SeMachineAccountPrivilege BLOCK ================ */
	
	if ( has_enough_rights )				
		become_root(); 
	
	/* ok!  user info levels (lots: see MSDEV help), off we go... */

	switch (switch_value) {
		case 18:
			if (!set_user_info_18(ctr->info.id18, pwd))
				r_u->status = NT_STATUS_ACCESS_DENIED;
			break;

		case 24:
			if (!p->session_key.length) {
				r_u->status = NT_STATUS_NO_USER_SESSION_KEY;
			}
			SamOEMhashBlob(ctr->info.id24->pass, 516, &p->session_key);

			dump_data(100, (char *)ctr->info.id24->pass, 516);

			if (!set_user_info_pw(ctr->info.id24->pass, pwd))
				r_u->status = NT_STATUS_ACCESS_DENIED;
			break;

		case 25:
			if (!p->session_key.length) {
				r_u->status = NT_STATUS_NO_USER_SESSION_KEY;
			}
			encode_or_decode_arc4_passwd_buffer(ctr->info.id25->pass, &p->session_key);

			dump_data(100, (char *)ctr->info.id25->pass, 532);

			r_u->status = set_user_info_25(p->mem_ctx,
						       ctr->info.id25, pwd);
			if (!NT_STATUS_IS_OK(r_u->status)) {
				goto done;
			}
			if (!set_user_info_pw(ctr->info.id25->pass, pwd))
				r_u->status = NT_STATUS_ACCESS_DENIED;
			break;

		case 26:
			if (!p->session_key.length) {
				r_u->status = NT_STATUS_NO_USER_SESSION_KEY;
			}
			encode_or_decode_arc4_passwd_buffer(ctr->info.id26->pass, &p->session_key);

			dump_data(100, (char *)ctr->info.id26->pass, 516);

			if (!set_user_info_pw(ctr->info.id26->pass, pwd))
				r_u->status = NT_STATUS_ACCESS_DENIED;
			break;

		case 23:
			if (!p->session_key.length) {
				r_u->status = NT_STATUS_NO_USER_SESSION_KEY;
			}
			SamOEMhashBlob(ctr->info.id23->pass, 516, &p->session_key);

			dump_data(100, (char *)ctr->info.id23->pass, 516);

			r_u->status = set_user_info_23(p->mem_ctx,
						       ctr->info.id23, pwd);
			break;

		default:
			r_u->status = NT_STATUS_INVALID_INFO_CLASS;
	}

 done:
	
	if ( has_enough_rights )				
		unbecome_root();
		
	/* ================ END SeMachineAccountPrivilege BLOCK ================ */

	if (NT_STATUS_IS_OK(r_u->status)) {
		force_flush_samr_cache(disp_info);
	}

	return r_u->status;
}

/*******************************************************************
 samr_reply_set_userinfo2
 ********************************************************************/

NTSTATUS _samr_set_userinfo2(pipes_struct *p, SAMR_Q_SET_USERINFO2 *q_u, SAMR_R_SET_USERINFO2 *r_u)
{
	struct samu *pwd = NULL;
	DOM_SID sid;
	SAM_USERINFO_CTR *ctr = q_u->ctr;
	POLICY_HND *pol = &q_u->pol;
	uint16 switch_value = q_u->switch_value;
	uint32 acc_granted;
	uint32 acc_required;
	BOOL ret;
	BOOL has_enough_rights = False;
	uint32 acb_info;
	DISP_INFO *disp_info = NULL;

	DEBUG(5, ("samr_reply_set_userinfo2: %d\n", __LINE__));

	r_u->status = NT_STATUS_OK;

	/* find the policy handle.  open a policy on it. */
	if (!get_lsa_policy_samr_sid(p, pol, &sid, &acc_granted, &disp_info))
		return NT_STATUS_INVALID_HANDLE;

		
#if 0 	/* this really should be applied on a per info level basis   --jerry */

	/* observed when joining XP client to Samba domain */
	acc_required = SA_RIGHT_USER_SET_PASSWORD | SA_RIGHT_USER_SET_ATTRIBUTES | SA_RIGHT_USER_ACCT_FLAGS_EXPIRY;
#else
	acc_required = SA_RIGHT_USER_SET_ATTRIBUTES;
#endif
	
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, acc_required, "_samr_set_userinfo2"))) {
		return r_u->status;
	}

	DEBUG(5, ("samr_reply_set_userinfo2: sid:%s\n", sid_string_static(&sid)));

	if (ctr == NULL) {
		DEBUG(5, ("samr_reply_set_userinfo2: NULL info level\n"));
		return NT_STATUS_INVALID_INFO_CLASS;
	}

	switch_value=ctr->switch_value;

	if ( !(pwd = samu_new( NULL )) ) {
		return NT_STATUS_NO_MEMORY;
	}

	become_root();
	ret = pdb_getsampwsid(pwd, &sid);
	unbecome_root();
	
	if ( !ret ) {
		TALLOC_FREE(pwd);
		return NT_STATUS_NO_SUCH_USER;
 	}
	
	acb_info = pdb_get_acct_ctrl(pwd);
	if ( acb_info & ACB_WSTRUST ) 
		has_enough_rights = user_has_privileges( p->pipe_user.nt_user_token, &se_machine_account);
	else if ( acb_info & ACB_NORMAL )
		has_enough_rights = user_has_privileges( p->pipe_user.nt_user_token, &se_add_users );
	else if ( acb_info & (ACB_SVRTRUST|ACB_DOMTRUST) ) {
		if ( lp_enable_privileges() )
			has_enough_rights = nt_token_check_domain_rid( p->pipe_user.nt_user_token, DOMAIN_GROUP_RID_ADMINS );
	}
	
	DEBUG(5, ("_samr_set_userinfo2: %s does%s possess sufficient rights\n",
		p->pipe_user_name, has_enough_rights ? "" : " not"));

	/* ================ BEGIN SeMachineAccountPrivilege BLOCK ================ */
	
	if ( has_enough_rights )				
		become_root(); 
	
	/* ok!  user info levels (lots: see MSDEV help), off we go... */
	
	switch (switch_value) {
		case 7:
			r_u->status = set_user_info_7(p->mem_ctx,
						      ctr->info.id7, pwd);
			break;
		case 16:
			if (!set_user_info_16(ctr->info.id16, pwd))
				r_u->status = NT_STATUS_ACCESS_DENIED;
			break;
		case 18:
			/* Used by AS/U JRA. */
			if (!set_user_info_18(ctr->info.id18, pwd))
				r_u->status = NT_STATUS_ACCESS_DENIED;
			break;
		case 20:
			if (!set_user_info_20(ctr->info.id20, pwd))
				r_u->status = NT_STATUS_ACCESS_DENIED;
			break;
		case 21:
			r_u->status = set_user_info_21(p->mem_ctx,
						       ctr->info.id21, pwd);
			break;
		case 23:
			if (!p->session_key.length) {
				r_u->status = NT_STATUS_NO_USER_SESSION_KEY;
			}
			SamOEMhashBlob(ctr->info.id23->pass, 516, &p->session_key);

			dump_data(100, (char *)ctr->info.id23->pass, 516);

			r_u->status = set_user_info_23(p->mem_ctx,
						       ctr->info.id23, pwd);
			break;
		case 26:
			if (!p->session_key.length) {
				r_u->status = NT_STATUS_NO_USER_SESSION_KEY;
			}
			encode_or_decode_arc4_passwd_buffer(ctr->info.id26->pass, &p->session_key);

			dump_data(100, (char *)ctr->info.id26->pass, 516);

			if (!set_user_info_pw(ctr->info.id26->pass, pwd))
				r_u->status = NT_STATUS_ACCESS_DENIED;
			break;
		default:
			r_u->status = NT_STATUS_INVALID_INFO_CLASS;
	}

	if ( has_enough_rights )				
		unbecome_root();
		
	/* ================ END SeMachineAccountPrivilege BLOCK ================ */

	if (NT_STATUS_IS_OK(r_u->status)) {
		force_flush_samr_cache(disp_info);
	}

	return r_u->status;
}

/*********************************************************************
 _samr_query_aliasmem
*********************************************************************/

NTSTATUS _samr_query_useraliases(pipes_struct *p, SAMR_Q_QUERY_USERALIASES *q_u, SAMR_R_QUERY_USERALIASES *r_u)
{
	size_t num_alias_rids;
	uint32 *alias_rids;
	struct samr_info *info = NULL;
	size_t i;
		
	NTSTATUS ntstatus1;
	NTSTATUS ntstatus2;

	DOM_SID *members;

	r_u->status = NT_STATUS_OK;

	DEBUG(5,("_samr_query_useraliases: %d\n", __LINE__));

	/* find the policy handle.  open a policy on it. */
	if (!find_policy_by_hnd(p, &q_u->pol, (void **)(void *)&info))
		return NT_STATUS_INVALID_HANDLE;
		
	ntstatus1 = access_check_samr_function(info->acc_granted, SA_RIGHT_DOMAIN_LOOKUP_ALIAS_BY_MEM, "_samr_query_useraliases");
	ntstatus2 = access_check_samr_function(info->acc_granted, SA_RIGHT_DOMAIN_OPEN_ACCOUNT, "_samr_query_useraliases");
	
	if (!NT_STATUS_IS_OK(ntstatus1) || !NT_STATUS_IS_OK(ntstatus2)) {
		if (!(NT_STATUS_EQUAL(ntstatus1,NT_STATUS_ACCESS_DENIED) && NT_STATUS_IS_OK(ntstatus2)) &&
		    !(NT_STATUS_EQUAL(ntstatus1,NT_STATUS_ACCESS_DENIED) && NT_STATUS_IS_OK(ntstatus1))) {
			return (NT_STATUS_IS_OK(ntstatus1)) ? ntstatus2 : ntstatus1;
		}
	}		

	if (!sid_check_is_domain(&info->sid) &&
	    !sid_check_is_builtin(&info->sid))
		return NT_STATUS_OBJECT_TYPE_MISMATCH;

	members = TALLOC_ARRAY(p->mem_ctx, DOM_SID, q_u->num_sids1);

	if (members == NULL)
		return NT_STATUS_NO_MEMORY;

	for (i=0; i<q_u->num_sids1; i++)
		sid_copy(&members[i], &q_u->sid[i].sid);

	alias_rids = NULL;
	num_alias_rids = 0;

	become_root();
	ntstatus1 = pdb_enum_alias_memberships(p->mem_ctx, &info->sid, members,
					       q_u->num_sids1,
					       &alias_rids, &num_alias_rids);
	unbecome_root();

	if (!NT_STATUS_IS_OK(ntstatus1)) {
		return ntstatus1;
	}

	init_samr_r_query_useraliases(r_u, num_alias_rids, alias_rids,
				      NT_STATUS_OK);
	return NT_STATUS_OK;
}

/*********************************************************************
 _samr_query_aliasmem
*********************************************************************/

NTSTATUS _samr_query_aliasmem(pipes_struct *p, SAMR_Q_QUERY_ALIASMEM *q_u, SAMR_R_QUERY_ALIASMEM *r_u)
{
	NTSTATUS status;
	size_t i;
	size_t num_sids = 0;
	DOM_SID2 *sid;
	DOM_SID *sids=NULL;

	DOM_SID alias_sid;

	uint32 acc_granted;

	/* find the policy handle.  open a policy on it. */
	if (!get_lsa_policy_samr_sid(p, &q_u->alias_pol, &alias_sid, &acc_granted, NULL)) 
		return NT_STATUS_INVALID_HANDLE;
	
	if (!NT_STATUS_IS_OK(r_u->status = 
		access_check_samr_function(acc_granted, SA_RIGHT_ALIAS_GET_MEMBERS, "_samr_query_aliasmem"))) {
		return r_u->status;
	}

	DEBUG(10, ("sid is %s\n", sid_string_static(&alias_sid)));

	become_root();
	status = pdb_enum_aliasmem(&alias_sid, &sids, &num_sids);
	unbecome_root();

	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}

	sid = TALLOC_ZERO_ARRAY(p->mem_ctx, DOM_SID2, num_sids);	
	if (num_sids!=0 && sid == NULL) {
		SAFE_FREE(sids);
		return NT_STATUS_NO_MEMORY;
	}

	for (i = 0; i < num_sids; i++) {
		init_dom_sid2(&sid[i], &sids[i]);
	}

	init_samr_r_query_aliasmem(r_u, num_sids, sid, NT_STATUS_OK);

	SAFE_FREE(sids);

	return NT_STATUS_OK;
}

/*********************************************************************
 _samr_query_groupmem
*********************************************************************/

NTSTATUS _samr_query_groupmem(pipes_struct *p, SAMR_Q_QUERY_GROUPMEM *q_u, SAMR_R_QUERY_GROUPMEM *r_u)
{
	DOM_SID group_sid;
	fstring group_sid_str;
	size_t i, num_members;

	uint32 *rid=NULL;
	uint32 *attr=NULL;

	uint32 acc_granted;

	NTSTATUS result;

	/* find the policy handle.  open a policy on it. */
	if (!get_lsa_policy_samr_sid(p, &q_u->group_pol, &group_sid, &acc_granted, NULL)) 
		return NT_STATUS_INVALID_HANDLE;
		
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, SA_RIGHT_GROUP_GET_MEMBERS, "_samr_query_groupmem"))) {
		return r_u->status;
	}
		
	sid_to_string(group_sid_str, &group_sid);
	DEBUG(10, ("sid is %s\n", group_sid_str));

	if (!sid_check_is_in_our_domain(&group_sid)) {
		DEBUG(3, ("sid %s is not in our domain\n", group_sid_str));
		return NT_STATUS_NO_SUCH_GROUP;
	}

	DEBUG(10, ("lookup on Domain SID\n"));

	become_root();
	result = pdb_enum_group_members(p->mem_ctx, &group_sid,
					&rid, &num_members);
	unbecome_root();

	if (!NT_STATUS_IS_OK(result))
		return result;

	attr=TALLOC_ZERO_ARRAY(p->mem_ctx, uint32, num_members);
	
	if ((num_members!=0) && (attr==NULL))
		return NT_STATUS_NO_MEMORY;
	
	for (i=0; i<num_members; i++)
		attr[i] = SID_NAME_USER;

	init_samr_r_query_groupmem(r_u, num_members, rid, attr, NT_STATUS_OK);

	return NT_STATUS_OK;
}

/*********************************************************************
 _samr_add_aliasmem
*********************************************************************/

NTSTATUS _samr_add_aliasmem(pipes_struct *p, SAMR_Q_ADD_ALIASMEM *q_u, SAMR_R_ADD_ALIASMEM *r_u)
{
	DOM_SID alias_sid;
	uint32 acc_granted;
	SE_PRIV se_rights;
	BOOL can_add_accounts;
	NTSTATUS ret;
	DISP_INFO *disp_info = NULL;

	/* Find the policy handle. Open a policy on it. */
	if (!get_lsa_policy_samr_sid(p, &q_u->alias_pol, &alias_sid, &acc_granted, &disp_info)) 
		return NT_STATUS_INVALID_HANDLE;
	
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, SA_RIGHT_ALIAS_ADD_MEMBER, "_samr_add_aliasmem"))) {
		return r_u->status;
	}
		
	DEBUG(10, ("sid is %s\n", sid_string_static(&alias_sid)));
	
	se_priv_copy( &se_rights, &se_add_users );
	can_add_accounts = user_has_privileges( p->pipe_user.nt_user_token, &se_rights );

	/******** BEGIN SeAddUsers BLOCK *********/
	
	if ( can_add_accounts )
		become_root();
	
	ret = pdb_add_aliasmem(&alias_sid, &q_u->sid.sid);
	
	if ( can_add_accounts )
		unbecome_root();
		
	/******** END SeAddUsers BLOCK *********/
	
	if (NT_STATUS_IS_OK(ret)) {
		force_flush_samr_cache(disp_info);
	}

	return ret;
}

/*********************************************************************
 _samr_del_aliasmem
*********************************************************************/

NTSTATUS _samr_del_aliasmem(pipes_struct *p, SAMR_Q_DEL_ALIASMEM *q_u, SAMR_R_DEL_ALIASMEM *r_u)
{
	DOM_SID alias_sid;
	uint32 acc_granted;
	SE_PRIV se_rights;
	BOOL can_add_accounts;
	NTSTATUS ret;
	DISP_INFO *disp_info = NULL;

	/* Find the policy handle. Open a policy on it. */
	if (!get_lsa_policy_samr_sid(p, &q_u->alias_pol, &alias_sid, &acc_granted, &disp_info)) 
		return NT_STATUS_INVALID_HANDLE;
	
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, SA_RIGHT_ALIAS_REMOVE_MEMBER, "_samr_del_aliasmem"))) {
		return r_u->status;
	}
	
	DEBUG(10, ("_samr_del_aliasmem:sid is %s\n",
		   sid_string_static(&alias_sid)));

	se_priv_copy( &se_rights, &se_add_users );
	can_add_accounts = user_has_privileges( p->pipe_user.nt_user_token, &se_rights );

	/******** BEGIN SeAddUsers BLOCK *********/
	
	if ( can_add_accounts )
		become_root();

	ret = pdb_del_aliasmem(&alias_sid, &q_u->sid.sid);
	
	if ( can_add_accounts )
		unbecome_root();
		
	/******** END SeAddUsers BLOCK *********/
	
	if (NT_STATUS_IS_OK(ret)) {
		force_flush_samr_cache(disp_info);
	}

	return ret;
}

/*********************************************************************
 _samr_add_groupmem
*********************************************************************/

NTSTATUS _samr_add_groupmem(pipes_struct *p, SAMR_Q_ADD_GROUPMEM *q_u, SAMR_R_ADD_GROUPMEM *r_u)
{
	DOM_SID group_sid;
	uint32 group_rid;
	uint32 acc_granted;
	SE_PRIV se_rights;
	BOOL can_add_accounts;
	DISP_INFO *disp_info = NULL;

	/* Find the policy handle. Open a policy on it. */
	if (!get_lsa_policy_samr_sid(p, &q_u->pol, &group_sid, &acc_granted, &disp_info)) 
		return NT_STATUS_INVALID_HANDLE;
	
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, SA_RIGHT_GROUP_ADD_MEMBER, "_samr_add_groupmem"))) {
		return r_u->status;
	}

	DEBUG(10, ("sid is %s\n", sid_string_static(&group_sid)));

	if (!sid_peek_check_rid(get_global_sam_sid(), &group_sid,
				&group_rid)) {
		return NT_STATUS_INVALID_HANDLE;
	}

	se_priv_copy( &se_rights, &se_add_users );
	can_add_accounts = user_has_privileges( p->pipe_user.nt_user_token, &se_rights );

	/******** BEGIN SeAddUsers BLOCK *********/
	
	if ( can_add_accounts )
		become_root();

	r_u->status = pdb_add_groupmem(p->mem_ctx, group_rid, q_u->rid);
		
	if ( can_add_accounts )
		unbecome_root();
		
	/******** END SeAddUsers BLOCK *********/
	
	force_flush_samr_cache(disp_info);

	return r_u->status;
}

/*********************************************************************
 _samr_del_groupmem
*********************************************************************/

NTSTATUS _samr_del_groupmem(pipes_struct *p, SAMR_Q_DEL_GROUPMEM *q_u, SAMR_R_DEL_GROUPMEM *r_u)
{
	DOM_SID group_sid;
	uint32 group_rid;
	uint32 acc_granted;
	SE_PRIV se_rights;
	BOOL can_add_accounts;
	DISP_INFO *disp_info = NULL;

	/*
	 * delete the group member named q_u->rid
	 * who is a member of the sid associated with the handle
	 * the rid is a user's rid as the group is a domain group.
	 */

	/* Find the policy handle. Open a policy on it. */
	if (!get_lsa_policy_samr_sid(p, &q_u->pol, &group_sid, &acc_granted, &disp_info)) 
		return NT_STATUS_INVALID_HANDLE;
	
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, SA_RIGHT_GROUP_REMOVE_MEMBER, "_samr_del_groupmem"))) {
		return r_u->status;
	}

	if (!sid_peek_check_rid(get_global_sam_sid(), &group_sid,
				&group_rid)) {
		return NT_STATUS_INVALID_HANDLE;
	}

	se_priv_copy( &se_rights, &se_add_users );
	can_add_accounts = user_has_privileges( p->pipe_user.nt_user_token, &se_rights );

	/******** BEGIN SeAddUsers BLOCK *********/
	
	if ( can_add_accounts )
		become_root();
		
	r_u->status = pdb_del_groupmem(p->mem_ctx, group_rid, q_u->rid);

	if ( can_add_accounts )
		unbecome_root();
		
	/******** END SeAddUsers BLOCK *********/
	
	force_flush_samr_cache(disp_info);

	return r_u->status;
}

/*********************************************************************
 _samr_delete_dom_user
*********************************************************************/

NTSTATUS _samr_delete_dom_user(pipes_struct *p, SAMR_Q_DELETE_DOM_USER *q_u, SAMR_R_DELETE_DOM_USER *r_u )
{
	DOM_SID user_sid;
	struct samu *sam_pass=NULL;
	uint32 acc_granted;
	BOOL can_add_accounts;
	uint32 acb_info;
	DISP_INFO *disp_info = NULL;
	BOOL ret;

	DEBUG(5, ("_samr_delete_dom_user: %d\n", __LINE__));

	/* Find the policy handle. Open a policy on it. */
	if (!get_lsa_policy_samr_sid(p, &q_u->user_pol, &user_sid, &acc_granted, &disp_info)) 
		return NT_STATUS_INVALID_HANDLE;
		
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, STD_RIGHT_DELETE_ACCESS, "_samr_delete_dom_user"))) {
		return r_u->status;
	}
		
	if (!sid_check_is_in_our_domain(&user_sid))
		return NT_STATUS_CANNOT_DELETE;

	/* check if the user exists before trying to delete */
	if ( !(sam_pass = samu_new( NULL )) ) {
		return NT_STATUS_NO_MEMORY;
	}

	become_root();
	ret = pdb_getsampwsid(sam_pass, &user_sid);
	unbecome_root();

	if( !ret ) {
		DEBUG(5,("_samr_delete_dom_user:User %s doesn't exist.\n", 
			sid_string_static(&user_sid)));
		TALLOC_FREE(sam_pass);
		return NT_STATUS_NO_SUCH_USER;
	}
	
	acb_info = pdb_get_acct_ctrl(sam_pass);

	/* For machine accounts it's the SeMachineAccountPrivilege that counts. */
	if ( acb_info & ACB_WSTRUST ) {
		can_add_accounts = user_has_privileges( p->pipe_user.nt_user_token, &se_machine_account );
	} else {
		can_add_accounts = user_has_privileges( p->pipe_user.nt_user_token, &se_add_users );
	} 

	/******** BEGIN SeAddUsers BLOCK *********/
	
	if ( can_add_accounts )
		become_root();

	r_u->status = pdb_delete_user(p->mem_ctx, sam_pass);

	if ( can_add_accounts )
		unbecome_root();
		
	/******** END SeAddUsers BLOCK *********/
		
	if ( !NT_STATUS_IS_OK(r_u->status) ) {
		DEBUG(5,("_samr_delete_dom_user: Failed to delete entry for "
			 "user %s: %s.\n", pdb_get_username(sam_pass),
			 nt_errstr(r_u->status)));
		TALLOC_FREE(sam_pass);
		return r_u->status;
	}


	TALLOC_FREE(sam_pass);

	if (!close_policy_hnd(p, &q_u->user_pol))
		return NT_STATUS_OBJECT_NAME_INVALID;

	force_flush_samr_cache(disp_info);

	return NT_STATUS_OK;
}

/*********************************************************************
 _samr_delete_dom_group
*********************************************************************/

NTSTATUS _samr_delete_dom_group(pipes_struct *p, SAMR_Q_DELETE_DOM_GROUP *q_u, SAMR_R_DELETE_DOM_GROUP *r_u)
{
	DOM_SID group_sid;
	uint32 group_rid;
	uint32 acc_granted;
	SE_PRIV se_rights;
	BOOL can_add_accounts;
	DISP_INFO *disp_info = NULL;

	DEBUG(5, ("samr_delete_dom_group: %d\n", __LINE__));

	/* Find the policy handle. Open a policy on it. */
	if (!get_lsa_policy_samr_sid(p, &q_u->group_pol, &group_sid, &acc_granted, &disp_info)) 
		return NT_STATUS_INVALID_HANDLE;
		
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, STD_RIGHT_DELETE_ACCESS, "_samr_delete_dom_group"))) {
		return r_u->status;
	}

	DEBUG(10, ("sid is %s\n", sid_string_static(&group_sid)));

	if (!sid_peek_check_rid(get_global_sam_sid(), &group_sid,
				&group_rid)) {
		return NT_STATUS_NO_SUCH_GROUP;
	}

	se_priv_copy( &se_rights, &se_add_users );
	can_add_accounts = user_has_privileges( p->pipe_user.nt_user_token, &se_rights );

	/******** BEGIN SeAddUsers BLOCK *********/
	
	if ( can_add_accounts )
		become_root();

	r_u->status = pdb_delete_dom_group(p->mem_ctx, group_rid);

	if ( can_add_accounts )
		unbecome_root();
		
	/******** END SeAddUsers BLOCK *********/
	
	if ( !NT_STATUS_IS_OK(r_u->status) ) {
		DEBUG(5,("_samr_delete_dom_group: Failed to delete mapping "
			 "entry for group %s: %s\n",
			 sid_string_static(&group_sid),
			 nt_errstr(r_u->status)));
		return r_u->status;
	}
	
	if (!close_policy_hnd(p, &q_u->group_pol))
		return NT_STATUS_OBJECT_NAME_INVALID;

	force_flush_samr_cache(disp_info);

	return NT_STATUS_OK;
}

/*********************************************************************
 _samr_delete_dom_alias
*********************************************************************/

NTSTATUS _samr_delete_dom_alias(pipes_struct *p, SAMR_Q_DELETE_DOM_ALIAS *q_u, SAMR_R_DELETE_DOM_ALIAS *r_u)
{
	DOM_SID alias_sid;
	uint32 acc_granted;
	SE_PRIV se_rights;
	BOOL can_add_accounts;
	BOOL ret;
	DISP_INFO *disp_info = NULL;

	DEBUG(5, ("_samr_delete_dom_alias: %d\n", __LINE__));

	/* Find the policy handle. Open a policy on it. */
	if (!get_lsa_policy_samr_sid(p, &q_u->alias_pol, &alias_sid, &acc_granted, &disp_info)) 
		return NT_STATUS_INVALID_HANDLE;
	
	/* copy the handle to the outgoing reply */

	memcpy( &r_u->pol, &q_u->alias_pol, sizeof(r_u->pol) );

	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, STD_RIGHT_DELETE_ACCESS, "_samr_delete_dom_alias"))) {
		return r_u->status;
	}

	DEBUG(10, ("sid is %s\n", sid_string_static(&alias_sid)));

	/* Don't let Windows delete builtin groups */

	if ( sid_check_is_in_builtin( &alias_sid ) ) {
		return NT_STATUS_SPECIAL_ACCOUNT;
	}

	if (!sid_check_is_in_our_domain(&alias_sid))
		return NT_STATUS_NO_SUCH_ALIAS;
		
	DEBUG(10, ("lookup on Local SID\n"));

	se_priv_copy( &se_rights, &se_add_users );
	can_add_accounts = user_has_privileges( p->pipe_user.nt_user_token, &se_rights );

	/******** BEGIN SeAddUsers BLOCK *********/
	
	if ( can_add_accounts )
		become_root();

	/* Have passdb delete the alias */
	ret = pdb_delete_alias(&alias_sid);
	
	if ( can_add_accounts )
		unbecome_root();
		
	/******** END SeAddUsers BLOCK *********/

	if ( !ret )
		return NT_STATUS_ACCESS_DENIED;

	if (!close_policy_hnd(p, &q_u->alias_pol))
		return NT_STATUS_OBJECT_NAME_INVALID;

	force_flush_samr_cache(disp_info);

	return NT_STATUS_OK;
}

/*********************************************************************
 _samr_create_dom_group
*********************************************************************/

NTSTATUS _samr_create_dom_group(pipes_struct *p, SAMR_Q_CREATE_DOM_GROUP *q_u, SAMR_R_CREATE_DOM_GROUP *r_u)
{
	DOM_SID dom_sid;
	DOM_SID info_sid;
	const char *name;
	struct samr_info *info;
	uint32 acc_granted;
	SE_PRIV se_rights;
	BOOL can_add_accounts;
	DISP_INFO *disp_info = NULL;

	/* Find the policy handle. Open a policy on it. */
	if (!get_lsa_policy_samr_sid(p, &q_u->pol, &dom_sid, &acc_granted, &disp_info)) 
		return NT_STATUS_INVALID_HANDLE;
	
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, SA_RIGHT_DOMAIN_CREATE_GROUP, "_samr_create_dom_group"))) {
		return r_u->status;
	}
		
	if (!sid_equal(&dom_sid, get_global_sam_sid()))
		return NT_STATUS_ACCESS_DENIED;

	name = rpcstr_pull_unistr2_talloc(p->mem_ctx, &q_u->uni_acct_desc);
	if (name == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	r_u->status = can_create(p->mem_ctx, name);
	if (!NT_STATUS_IS_OK(r_u->status)) {
		return r_u->status;
	}

	se_priv_copy( &se_rights, &se_add_users );
	can_add_accounts = user_has_privileges( p->pipe_user.nt_user_token, &se_rights );

	/******** BEGIN SeAddUsers BLOCK *********/
	
	if ( can_add_accounts )
		become_root();
	
	/* check that we successfully create the UNIX group */
	
	r_u->status = pdb_create_dom_group(p->mem_ctx, name, &r_u->rid);

	if ( can_add_accounts )
		unbecome_root();
		
	/******** END SeAddUsers BLOCK *********/
	
	/* check if we should bail out here */
	
	if ( !NT_STATUS_IS_OK(r_u->status) )
		return r_u->status;

	sid_compose(&info_sid, get_global_sam_sid(), r_u->rid);
	
	if ((info = get_samr_info_by_sid(&info_sid)) == NULL)
		return NT_STATUS_NO_MEMORY;

	/* they created it; let the user do what he wants with it */

	info->acc_granted = GENERIC_RIGHTS_GROUP_ALL_ACCESS;

	/* get a (unique) handle.  open a policy on it. */
	if (!create_policy_hnd(p, &r_u->pol, free_samr_info, (void *)info))
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;

	force_flush_samr_cache(disp_info);

	return NT_STATUS_OK;
}

/*********************************************************************
 _samr_create_dom_alias
*********************************************************************/

NTSTATUS _samr_create_dom_alias(pipes_struct *p, SAMR_Q_CREATE_DOM_ALIAS *q_u, SAMR_R_CREATE_DOM_ALIAS *r_u)
{
	DOM_SID dom_sid;
	DOM_SID info_sid;
	fstring name;
	struct samr_info *info;
	uint32 acc_granted;
	gid_t gid;
	NTSTATUS result;
	SE_PRIV se_rights;
	BOOL can_add_accounts;
	DISP_INFO *disp_info = NULL;

	/* Find the policy handle. Open a policy on it. */
	if (!get_lsa_policy_samr_sid(p, &q_u->dom_pol, &dom_sid, &acc_granted, &disp_info)) 
		return NT_STATUS_INVALID_HANDLE;
		
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, SA_RIGHT_DOMAIN_CREATE_ALIAS, "_samr_create_alias"))) {
		return r_u->status;
	}
		
	if (!sid_equal(&dom_sid, get_global_sam_sid()))
		return NT_STATUS_ACCESS_DENIED;

	unistr2_to_ascii(name, &q_u->uni_acct_desc, sizeof(name)-1);

	se_priv_copy( &se_rights, &se_add_users );
	can_add_accounts = user_has_privileges( p->pipe_user.nt_user_token, &se_rights );

	result = can_create(p->mem_ctx, name);
	if (!NT_STATUS_IS_OK(result)) {
		return result;
	}

	/******** BEGIN SeAddUsers BLOCK *********/
	
	if ( can_add_accounts )
		become_root();

	/* Have passdb create the alias */
	result = pdb_create_alias(name, &r_u->rid);

	if ( can_add_accounts )
		unbecome_root();
		
	/******** END SeAddUsers BLOCK *********/

	if (!NT_STATUS_IS_OK(result)) {
		DEBUG(10, ("pdb_create_alias failed: %s\n",
			   nt_errstr(result)));
		return result;
	}

	sid_copy(&info_sid, get_global_sam_sid());
	sid_append_rid(&info_sid, r_u->rid);

	if (!sid_to_gid(&info_sid, &gid)) {
		DEBUG(10, ("Could not find alias just created\n"));
		return NT_STATUS_ACCESS_DENIED;
	}

	/* check if the group has been successfully created */
	if ( getgrgid(gid) == NULL ) {
		DEBUG(10, ("getgrgid(%d) of just created alias failed\n",
			   gid));
		return NT_STATUS_ACCESS_DENIED;
	}

	if ((info = get_samr_info_by_sid(&info_sid)) == NULL)
		return NT_STATUS_NO_MEMORY;

	/* they created it; let the user do what he wants with it */

	info->acc_granted = GENERIC_RIGHTS_ALIAS_ALL_ACCESS;

	/* get a (unique) handle.  open a policy on it. */
	if (!create_policy_hnd(p, &r_u->alias_pol, free_samr_info, (void *)info))
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;

	force_flush_samr_cache(disp_info);

	return NT_STATUS_OK;
}

/*********************************************************************
 _samr_query_groupinfo

sends the name/comment pair of a domain group
level 1 send also the number of users of that group
*********************************************************************/

NTSTATUS _samr_query_groupinfo(pipes_struct *p, SAMR_Q_QUERY_GROUPINFO *q_u, SAMR_R_QUERY_GROUPINFO *r_u)
{
	DOM_SID group_sid;
	GROUP_MAP map;
	GROUP_INFO_CTR *ctr;
	uint32 acc_granted;
	BOOL ret;

	if (!get_lsa_policy_samr_sid(p, &q_u->pol, &group_sid, &acc_granted, NULL)) 
		return NT_STATUS_INVALID_HANDLE;
	
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, SA_RIGHT_GROUP_LOOKUP_INFO, "_samr_query_groupinfo"))) {
		return r_u->status;
	}
		
	become_root();
	ret = get_domain_group_from_sid(group_sid, &map);
	unbecome_root();
	if (!ret)
		return NT_STATUS_INVALID_HANDLE;

	ctr=TALLOC_ZERO_P(p->mem_ctx, GROUP_INFO_CTR);
	if (ctr==NULL)
		return NT_STATUS_NO_MEMORY;

	switch (q_u->switch_level) {
		case 1: {
			uint32 *members;
			size_t num_members;

			ctr->switch_value1 = 1;

			become_root();
			r_u->status = pdb_enum_group_members(
				p->mem_ctx, &group_sid, &members, &num_members);
			unbecome_root();
	
			if (!NT_STATUS_IS_OK(r_u->status)) {
				return r_u->status;
			}

			init_samr_group_info1(&ctr->group.info1, map.nt_name,
				      map.comment, num_members);
			break;
		}
		case 2:
			ctr->switch_value1 = 2;
			init_samr_group_info2(&ctr->group.info2, map.nt_name);
			break;
		case 3:
			ctr->switch_value1 = 3;
			init_samr_group_info3(&ctr->group.info3);
			break;
		case 4:
			ctr->switch_value1 = 4;
			init_samr_group_info4(&ctr->group.info4, map.comment);
			break;
		case 5: {
			/*
			uint32 *members;
			size_t num_members;
			*/

			ctr->switch_value1 = 5;

			/*
			become_root();
			r_u->status = pdb_enum_group_members(
				p->mem_ctx, &group_sid, &members, &num_members);
			unbecome_root();
	
			if (!NT_STATUS_IS_OK(r_u->status)) {
				return r_u->status;
			}
			*/
			init_samr_group_info5(&ctr->group.info5, map.nt_name,
				      map.comment, 0 /* num_members */); /* in w2k3 this is always 0 */
			break;
		}
		default:
			return NT_STATUS_INVALID_INFO_CLASS;
	}

	init_samr_r_query_groupinfo(r_u, ctr, NT_STATUS_OK);

	return NT_STATUS_OK;
}

/*********************************************************************
 _samr_set_groupinfo
 
 update a domain group's comment.
*********************************************************************/

NTSTATUS _samr_set_groupinfo(pipes_struct *p, SAMR_Q_SET_GROUPINFO *q_u, SAMR_R_SET_GROUPINFO *r_u)
{
	DOM_SID group_sid;
	GROUP_MAP map;
	GROUP_INFO_CTR *ctr;
	uint32 acc_granted;
	NTSTATUS ret;
	BOOL result;
	BOOL can_mod_accounts;
	DISP_INFO *disp_info = NULL;

	if (!get_lsa_policy_samr_sid(p, &q_u->pol, &group_sid, &acc_granted, &disp_info))
		return NT_STATUS_INVALID_HANDLE;
	
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, SA_RIGHT_GROUP_SET_INFO, "_samr_set_groupinfo"))) {
		return r_u->status;
	}

	become_root();
	result = get_domain_group_from_sid(group_sid, &map);
	unbecome_root();
	if (!result)
		return NT_STATUS_NO_SUCH_GROUP;
	
	ctr=q_u->ctr;

	switch (ctr->switch_value1) {
		case 1:
			unistr2_to_ascii(map.comment, &(ctr->group.info1.uni_acct_desc), sizeof(map.comment)-1);
			break;
		case 4:
			unistr2_to_ascii(map.comment, &(ctr->group.info4.uni_acct_desc), sizeof(map.comment)-1);
			break;
		default:
			return NT_STATUS_INVALID_INFO_CLASS;
	}

	can_mod_accounts = user_has_privileges( p->pipe_user.nt_user_token, &se_add_users );

	/******** BEGIN SeAddUsers BLOCK *********/

	if ( can_mod_accounts )
		become_root();
	  
	ret = pdb_update_group_mapping_entry(&map);

	if ( can_mod_accounts )
		unbecome_root();

	/******** End SeAddUsers BLOCK *********/

	if (NT_STATUS_IS_OK(ret)) {
		force_flush_samr_cache(disp_info);
	}

	return ret;
}

/*********************************************************************
 _samr_set_aliasinfo
 
 update an alias's comment.
*********************************************************************/

NTSTATUS _samr_set_aliasinfo(pipes_struct *p, SAMR_Q_SET_ALIASINFO *q_u, SAMR_R_SET_ALIASINFO *r_u)
{
	DOM_SID group_sid;
	struct acct_info info;
	ALIAS_INFO_CTR *ctr;
	uint32 acc_granted;
	BOOL ret;
	BOOL can_mod_accounts;
	DISP_INFO *disp_info = NULL;

	if (!get_lsa_policy_samr_sid(p, &q_u->alias_pol, &group_sid, &acc_granted, &disp_info))
		return NT_STATUS_INVALID_HANDLE;
	
	if (!NT_STATUS_IS_OK(r_u->status = access_check_samr_function(acc_granted, SA_RIGHT_ALIAS_SET_INFO, "_samr_set_aliasinfo"))) {
		return r_u->status;
	}
		
	ctr=&q_u->ctr;

	/* get the current group information */

	become_root();
	ret = pdb_get_aliasinfo( &group_sid, &info );
	unbecome_root();

	if ( !ret ) {
		return NT_STATUS_NO_SUCH_ALIAS;
	}

	switch (ctr->level) {
		case 2:
			/* We currently do not support renaming groups in the
			   the BUILTIN domain.  Refer to util_builtin.c to understand 
			   why.  The eventually needs to be fixed to be like Windows
			   where you can rename builtin groups, just not delete them */

			if ( sid_check_is_in_builtin( &group_sid ) ) {
				return NT_STATUS_SPECIAL_ACCOUNT;
			}

			if ( ctr->alias.info2.name.string ) {
				unistr2_to_ascii( info.acct_name, ctr->alias.info2.name.string, 
					sizeof(info.acct_name)-1 );
			}
			else
				fstrcpy( info.acct_name, "" );
			break;
		case 3:
			if ( ctr->alias.info3.description.string ) {
				unistr2_to_ascii( info.acct_desc, 
					ctr->alias.info3.description.string, 
					sizeof(info.acct_desc)-1 );
			}
			else
				fstrcpy( info.acct_desc, "" );
			break;
		default:
			return NT_STATUS_INVALID_INFO_CLASS;
	}

        can_mod_accounts = user_has_privileges( p->pipe_user.nt_user_token, &se_add_users );

        /******** BEGIN SeAddUsers BLOCK *********/

        if ( can_mod_accounts )
                become_root();

        ret = pdb_set_aliasinfo( &group_sid, &info );

        if ( can_mod_accounts )
                unbecome_root();

        /******** End SeAddUsers BLOCK *********/

	if (ret) {
		force_flush_samr_cache(disp_info);
	}

	return ret ? NT_STATUS_OK : NT_STATUS_ACCESS_DENIED;
}

/*********************************************************************
 _samr_get_dom_pwinfo
*********************************************************************/

NTSTATUS _samr_get_dom_pwinfo(pipes_struct *p, SAMR_Q_GET_DOM_PWINFO *q_u, SAMR_R_GET_DOM_PWINFO *r_u)
{
	/* Perform access check.  Since this rpc does not require a
	   policy handle it will not be caught by the access checks on
	   SAMR_CONNECT or SAMR_CONNECT_ANON. */

	if (!pipe_access_check(p)) {
		DEBUG(3, ("access denied to samr_get_dom_pwinfo\n"));
		r_u->status = NT_STATUS_ACCESS_DENIED;
		return r_u->status;
	}

	/* Actually, returning zeros here works quite well :-). */

	return NT_STATUS_OK;
}

/*********************************************************************
 _samr_open_group
*********************************************************************/

NTSTATUS _samr_open_group(pipes_struct *p, SAMR_Q_OPEN_GROUP *q_u, SAMR_R_OPEN_GROUP *r_u)
{
	DOM_SID sid;
	DOM_SID info_sid;
	GROUP_MAP map;
	struct samr_info *info;
	SEC_DESC         *psd = NULL;
	uint32            acc_granted;
	uint32            des_access = q_u->access_mask;
	size_t            sd_size;
	NTSTATUS          status;
	fstring sid_string;
	BOOL ret;
	SE_PRIV se_rights;

	if (!get_lsa_policy_samr_sid(p, &q_u->domain_pol, &sid, &acc_granted, NULL)) 
		return NT_STATUS_INVALID_HANDLE;
	
	status = access_check_samr_function(acc_granted, 
		SA_RIGHT_DOMAIN_OPEN_ACCOUNT, "_samr_open_group");
		
	if ( !NT_STATUS_IS_OK(status) )
		return status;
		
	/*check if access can be granted as requested by client. */
	make_samr_object_sd(p->mem_ctx, &psd, &sd_size, &grp_generic_mapping, NULL, 0);
	se_map_generic(&des_access,&grp_generic_mapping);

	se_priv_copy( &se_rights, &se_add_users );

	status = access_check_samr_object(psd, p->pipe_user.nt_user_token, 
		&se_rights, GENERIC_RIGHTS_GROUP_WRITE, des_access, 
		&acc_granted, "_samr_open_group");
		
	if ( !NT_STATUS_IS_OK(status) ) 
		return status;

	/* this should not be hard-coded like this */
	
	if (!sid_equal(&sid, get_global_sam_sid()))
		return NT_STATUS_ACCESS_DENIED;

	sid_copy(&info_sid, get_global_sam_sid());
	sid_append_rid(&info_sid, q_u->rid_group);
	sid_to_string(sid_string, &info_sid);

	if ((info = get_samr_info_by_sid(&info_sid)) == NULL)
		return NT_STATUS_NO_MEMORY;
		
	info->acc_granted = acc_granted;

	DEBUG(10, ("_samr_open_group:Opening SID: %s\n", sid_string));

	/* check if that group really exists */
	become_root();
	ret = get_domain_group_from_sid(info->sid, &map);
	unbecome_root();
	if (!ret)
		return NT_STATUS_NO_SUCH_GROUP;

	/* get a (unique) handle.  open a policy on it. */
	if (!create_policy_hnd(p, &r_u->pol, free_samr_info, (void *)info))
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;

	return NT_STATUS_OK;
}

/*********************************************************************
 _samr_remove_sid_foreign_domain
*********************************************************************/

NTSTATUS _samr_remove_sid_foreign_domain(pipes_struct *p, 
                                          SAMR_Q_REMOVE_SID_FOREIGN_DOMAIN *q_u, 
                                          SAMR_R_REMOVE_SID_FOREIGN_DOMAIN *r_u)
{
	DOM_SID			delete_sid, domain_sid;
	uint32 			acc_granted;
	NTSTATUS		result;
	DISP_INFO *disp_info = NULL;

	sid_copy( &delete_sid, &q_u->sid.sid );
	
	DEBUG(5,("_samr_remove_sid_foreign_domain: removing SID [%s]\n",
		sid_string_static(&delete_sid)));
		
	/* Find the policy handle. Open a policy on it. */
	
	if (!get_lsa_policy_samr_sid(p, &q_u->dom_pol, &domain_sid,
				     &acc_granted, &disp_info)) 
		return NT_STATUS_INVALID_HANDLE;
	
	result = access_check_samr_function(acc_granted, STD_RIGHT_DELETE_ACCESS, 
		"_samr_remove_sid_foreign_domain");
		
	if (!NT_STATUS_IS_OK(result)) 
		return result;
			
	DEBUG(8, ("_samr_remove_sid_foreign_domain:sid is %s\n", 
		sid_string_static(&domain_sid)));

	/* we can only delete a user from a group since we don't have 
	   nested groups anyways.  So in the latter case, just say OK */

	/* TODO: The above comment nowadays is bogus. Since we have nested
	 * groups now, and aliases members are never reported out of the unix
	 * group membership, the "just say OK" makes this call a no-op. For
	 * us. This needs fixing however. */

	/* I've only ever seen this in the wild when deleting a user from
	 * usrmgr.exe. domain_sid is the builtin domain, and the sid to delete
	 * is the user about to be deleted. I very much suspect this is the
	 * only application of this call. To verify this, let people report
	 * other cases. */

	if (!sid_check_is_builtin(&domain_sid)) {
		DEBUG(1,("_samr_remove_sid_foreign_domain: domain_sid = %s, "
			 "global_sam_sid() = %s\n",
			 sid_string_static(&domain_sid),
			 sid_string_static(get_global_sam_sid())));
		DEBUGADD(1,("please report to samba-technical@samba.org!\n"));
		return NT_STATUS_OK;
	}

	force_flush_samr_cache(disp_info);

	result = NT_STATUS_OK;

	return result;
}

/*******************************************************************
 _samr_query_domain_info2
 ********************************************************************/

NTSTATUS _samr_query_domain_info2(pipes_struct *p,
				  SAMR_Q_QUERY_DOMAIN_INFO2 *q_u,
				  SAMR_R_QUERY_DOMAIN_INFO2 *r_u)
{
	SAMR_Q_QUERY_DOMAIN_INFO q;
	SAMR_R_QUERY_DOMAIN_INFO r;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	DEBUG(5,("_samr_query_domain_info2: %d\n", __LINE__));

	q.domain_pol = q_u->domain_pol;
	q.switch_value = q_u->switch_value;

	r_u->status = _samr_query_domain_info(p, &q, &r);

	r_u->ptr_0 		= r.ptr_0;
	r_u->switch_value	= r.switch_value;
	r_u->ctr		= r.ctr;

	return r_u->status;
}

/*******************************************************************
 _samr_set_dom_info
 ********************************************************************/

NTSTATUS _samr_set_dom_info(pipes_struct *p, SAMR_Q_SET_DOMAIN_INFO *q_u, SAMR_R_SET_DOMAIN_INFO *r_u)
{
	time_t u_expire, u_min_age;
	time_t u_logout;
	time_t u_lock_duration, u_reset_time;

	r_u->status = NT_STATUS_OK;

	DEBUG(5,("_samr_set_dom_info: %d\n", __LINE__));

	/* find the policy handle.  open a policy on it. */
	if (!find_policy_by_hnd(p, &q_u->domain_pol, NULL))
		return NT_STATUS_INVALID_HANDLE;

	DEBUG(5,("_samr_set_dom_info: switch_value: %d\n", q_u->switch_value));

	switch (q_u->switch_value) {
        	case 0x01:
			u_expire=nt_time_to_unix_abs(&q_u->ctr->info.inf1.expire);
			u_min_age=nt_time_to_unix_abs(&q_u->ctr->info.inf1.min_passwordage);
			
			pdb_set_account_policy(AP_MIN_PASSWORD_LEN, (uint32)q_u->ctr->info.inf1.min_length_password);
			pdb_set_account_policy(AP_PASSWORD_HISTORY, (uint32)q_u->ctr->info.inf1.password_history);
			pdb_set_account_policy(AP_USER_MUST_LOGON_TO_CHG_PASS, (uint32)q_u->ctr->info.inf1.password_properties);
			pdb_set_account_policy(AP_MAX_PASSWORD_AGE, (int)u_expire);
			pdb_set_account_policy(AP_MIN_PASSWORD_AGE, (int)u_min_age);
            		break;
        	case 0x02:
			break;
		case 0x03:
			u_logout=nt_time_to_unix_abs(&q_u->ctr->info.inf3.logout);
			pdb_set_account_policy(AP_TIME_TO_LOGOUT, (int)u_logout);
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x0c:
			u_lock_duration=nt_time_to_unix_abs(&q_u->ctr->info.inf12.duration);
			if (u_lock_duration != -1)
				u_lock_duration /= 60;

			u_reset_time=nt_time_to_unix_abs(&q_u->ctr->info.inf12.reset_count)/60;
			
			pdb_set_account_policy(AP_LOCK_ACCOUNT_DURATION, (int)u_lock_duration);
			pdb_set_account_policy(AP_RESET_COUNT_TIME, (int)u_reset_time);
			pdb_set_account_policy(AP_BAD_ATTEMPT_LOCKOUT, (uint32)q_u->ctr->info.inf12.bad_attempt_lockout);
			break;
		default:
			return NT_STATUS_INVALID_INFO_CLASS;
	}

	init_samr_r_set_domain_info(r_u, NT_STATUS_OK);

	DEBUG(5,("_samr_set_dom_info: %d\n", __LINE__));

	return r_u->status;
}
