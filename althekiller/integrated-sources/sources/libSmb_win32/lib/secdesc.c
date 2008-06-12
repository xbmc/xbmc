/* 
 *  Unix SMB/Netbios implementation.
 *  SEC_DESC handling functions
 *  Copyright (C) Andrew Tridgell              1992-1998,
 *  Copyright (C) Jeremy R. Allison            1995-2003.
 *  Copyright (C) Luke Kenneth Casson Leighton 1996-1998,
 *  Copyright (C) Paul Ashton                  1997-1998.
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

/* Map generic permissions to file object specific permissions */

struct generic_mapping file_generic_mapping = {
	FILE_GENERIC_READ,
	FILE_GENERIC_WRITE,
	FILE_GENERIC_EXECUTE,
	FILE_GENERIC_ALL
};

/*******************************************************************
 Works out the linearization size of a SEC_DESC.
********************************************************************/

size_t sec_desc_size(SEC_DESC *psd)
{
	size_t offset;

	if (!psd) return 0;

	offset = SEC_DESC_HEADER_SIZE;

	/* don't align */

	if (psd->owner_sid != NULL)
		offset += sid_size(psd->owner_sid);

	if (psd->grp_sid != NULL)
		offset += sid_size(psd->grp_sid);

	if (psd->sacl != NULL)
		offset += psd->sacl->size;

	if (psd->dacl != NULL)
		offset += psd->dacl->size;

	return offset;
}

/*******************************************************************
 Compares two SEC_DESC structures
********************************************************************/

BOOL sec_desc_equal(SEC_DESC *s1, SEC_DESC *s2)
{
	/* Trivial case */

	if (!s1 && !s2) {
		goto done;
	}

	if (!s1 || !s2) {
		return False;
	}

	/* Check top level stuff */

	if (s1->revision != s2->revision) {
		DEBUG(10, ("sec_desc_equal(): revision differs (%d != %d)\n",
			   s1->revision, s2->revision));
		return False;
	}

	if (s1->type!= s2->type) {
		DEBUG(10, ("sec_desc_equal(): type differs (%d != %d)\n",
			   s1->type, s2->type));
		return False;
	}

	/* Check owner and group */

	if (!sid_equal(s1->owner_sid, s2->owner_sid)) {
		fstring str1, str2;

		sid_to_string(str1, s1->owner_sid);
		sid_to_string(str2, s2->owner_sid);

		DEBUG(10, ("sec_desc_equal(): owner differs (%s != %s)\n",
			   str1, str2));
		return False;
	}

	if (!sid_equal(s1->grp_sid, s2->grp_sid)) {
		fstring str1, str2;

		sid_to_string(str1, s1->grp_sid);
		sid_to_string(str2, s2->grp_sid);

		DEBUG(10, ("sec_desc_equal(): group differs (%s != %s)\n",
			   str1, str2));
		return False;
	}

	/* Check ACLs present in one but not the other */

	if ((s1->dacl && !s2->dacl) || (!s1->dacl && s2->dacl) ||
	    (s1->sacl && !s2->sacl) || (!s1->sacl && s2->sacl)) {
		DEBUG(10, ("sec_desc_equal(): dacl or sacl not present\n"));
		return False;
	}

	/* Sigh - we have to do it the hard way by iterating over all
	   the ACEs in the ACLs */

	if (!sec_acl_equal(s1->dacl, s2->dacl) ||
	    !sec_acl_equal(s1->sacl, s2->sacl)) {
		DEBUG(10, ("sec_desc_equal(): dacl/sacl list not equal\n"));
		return False;
	}

 done:
	DEBUG(10, ("sec_desc_equal(): secdescs are identical\n"));
	return True;
}

/*******************************************************************
 Merge part of security descriptor old_sec in to the empty sections of 
 security descriptor new_sec.
********************************************************************/

SEC_DESC_BUF *sec_desc_merge(TALLOC_CTX *ctx, SEC_DESC_BUF *new_sdb, SEC_DESC_BUF *old_sdb)
{
	DOM_SID *owner_sid, *group_sid;
	SEC_DESC_BUF *return_sdb;
	SEC_ACL *dacl, *sacl;
	SEC_DESC *psd = NULL;
	uint16 secdesc_type;
	size_t secdesc_size;

	/* Copy over owner and group sids.  There seems to be no flag for
	   this so just check the pointer values. */

	owner_sid = new_sdb->sec->owner_sid ? new_sdb->sec->owner_sid :
		old_sdb->sec->owner_sid;

	group_sid = new_sdb->sec->grp_sid ? new_sdb->sec->grp_sid :
		old_sdb->sec->grp_sid;
	
	secdesc_type = new_sdb->sec->type;

	/* Ignore changes to the system ACL.  This has the effect of making
	   changes through the security tab audit button not sticking. 
	   Perhaps in future Samba could implement these settings somehow. */

	sacl = NULL;
	secdesc_type &= ~SEC_DESC_SACL_PRESENT;

	/* Copy across discretionary ACL */

	if (secdesc_type & SEC_DESC_DACL_PRESENT) {
		dacl = new_sdb->sec->dacl;
	} else {
		dacl = old_sdb->sec->dacl;
	}

	/* Create new security descriptor from bits */

	psd = make_sec_desc(ctx, new_sdb->sec->revision, secdesc_type,
			    owner_sid, group_sid, sacl, dacl, &secdesc_size);

	return_sdb = make_sec_desc_buf(ctx, secdesc_size, psd);

	return(return_sdb);
}

/*******************************************************************
 Creates a SEC_DESC structure
********************************************************************/

SEC_DESC *make_sec_desc(TALLOC_CTX *ctx, uint16 revision, uint16 type,
			const DOM_SID *owner_sid, const DOM_SID *grp_sid,
			SEC_ACL *sacl, SEC_ACL *dacl, size_t *sd_size)
{
	SEC_DESC *dst;
	uint32 offset     = 0;

	*sd_size = 0;

	if(( dst = TALLOC_ZERO_P(ctx, SEC_DESC)) == NULL)
		return NULL;

	dst->revision = revision;
	dst->type = type;

	if (sacl)
		dst->type |= SEC_DESC_SACL_PRESENT;
	if (dacl)
		dst->type |= SEC_DESC_DACL_PRESENT;

	dst->off_owner_sid = 0;
	dst->off_grp_sid   = 0;
	dst->off_sacl      = 0;
	dst->off_dacl      = 0;

	if(owner_sid && ((dst->owner_sid = sid_dup_talloc(ctx,owner_sid)) == NULL))
		goto error_exit;

	if(grp_sid && ((dst->grp_sid = sid_dup_talloc(ctx,grp_sid)) == NULL))
		goto error_exit;

	if(sacl && ((dst->sacl = dup_sec_acl(ctx, sacl)) == NULL))
		goto error_exit;

	if(dacl && ((dst->dacl = dup_sec_acl(ctx, dacl)) == NULL))
		goto error_exit;

	offset = SEC_DESC_HEADER_SIZE;

	/*
	 * Work out the linearization sizes.
	 */

	if (dst->sacl != NULL) {
		dst->off_sacl = offset;
		offset += dst->sacl->size;
	}
	if (dst->dacl != NULL) {
		dst->off_dacl = offset;
		offset += dst->dacl->size;
	}

	if (dst->owner_sid != NULL) {
		dst->off_owner_sid = offset;
		offset += sid_size(dst->owner_sid);
	}

	if (dst->grp_sid != NULL) {
		dst->off_grp_sid = offset;
		offset += sid_size(dst->grp_sid);
	}

	*sd_size = (size_t)offset;
	return dst;

error_exit:

	*sd_size = 0;
	return NULL;
}

/*******************************************************************
 Duplicate a SEC_DESC structure.  
********************************************************************/

SEC_DESC *dup_sec_desc(TALLOC_CTX *ctx, const SEC_DESC *src)
{
	size_t dummy;

	if(src == NULL)
		return NULL;

	return make_sec_desc( ctx, src->revision, src->type,
				src->owner_sid, src->grp_sid, src->sacl,
				src->dacl, &dummy);
}

/*******************************************************************
 Creates a SEC_DESC structure with typical defaults.
********************************************************************/

SEC_DESC *make_standard_sec_desc(TALLOC_CTX *ctx, const DOM_SID *owner_sid, const DOM_SID *grp_sid,
				 SEC_ACL *dacl, size_t *sd_size)
{
	return make_sec_desc(ctx, SEC_DESC_REVISION, SEC_DESC_SELF_RELATIVE,
			     owner_sid, grp_sid, NULL, dacl, sd_size);
}

/*******************************************************************
 Creates a SEC_DESC_BUF structure.
********************************************************************/

SEC_DESC_BUF *make_sec_desc_buf(TALLOC_CTX *ctx, size_t len, SEC_DESC *sec_desc)
{
	SEC_DESC_BUF *dst;

	if((dst = TALLOC_ZERO_P(ctx, SEC_DESC_BUF)) == NULL)
		return NULL;

	/* max buffer size (allocated size) */
	dst->max_len = (uint32)len;
	dst->len = (uint32)len;
	
	if(sec_desc && ((dst->sec = dup_sec_desc(ctx, sec_desc)) == NULL)) {
		return NULL;
	}

	dst->ptr = 0x1;

	return dst;
}

/*******************************************************************
 Duplicates a SEC_DESC_BUF structure.
********************************************************************/

SEC_DESC_BUF *dup_sec_desc_buf(TALLOC_CTX *ctx, SEC_DESC_BUF *src)
{
	if(src == NULL)
		return NULL;

	return make_sec_desc_buf( ctx, src->len, src->sec);
}

/*******************************************************************
 Add a new SID with its permissions to SEC_DESC.
********************************************************************/

NTSTATUS sec_desc_add_sid(TALLOC_CTX *ctx, SEC_DESC **psd, DOM_SID *sid, uint32 mask, size_t *sd_size)
{
	SEC_DESC *sd   = 0;
	SEC_ACL  *dacl = 0;
	SEC_ACE  *ace  = 0;
	NTSTATUS  status;

	if (!ctx || !psd || !sid || !sd_size)
		return NT_STATUS_INVALID_PARAMETER;

	*sd_size = 0;

	status = sec_ace_add_sid(ctx, &ace, psd[0]->dacl->ace, &psd[0]->dacl->num_aces, sid, mask);
	
	if (!NT_STATUS_IS_OK(status))
		return status;

	if (!(dacl = make_sec_acl(ctx, psd[0]->dacl->revision, psd[0]->dacl->num_aces, ace)))
		return NT_STATUS_UNSUCCESSFUL;
	
	if (!(sd = make_sec_desc(ctx, psd[0]->revision, psd[0]->type, psd[0]->owner_sid, 
		psd[0]->grp_sid, psd[0]->sacl, dacl, sd_size)))
		return NT_STATUS_UNSUCCESSFUL;

	*psd = sd;
	 sd  = 0;
	return NT_STATUS_OK;
}

/*******************************************************************
 Modify a SID's permissions in a SEC_DESC.
********************************************************************/

NTSTATUS sec_desc_mod_sid(SEC_DESC *sd, DOM_SID *sid, uint32 mask)
{
	NTSTATUS status;

	if (!sd || !sid)
		return NT_STATUS_INVALID_PARAMETER;

	status = sec_ace_mod_sid(sd->dacl->ace, sd->dacl->num_aces, sid, mask);

	if (!NT_STATUS_IS_OK(status))
		return status;
	
	return NT_STATUS_OK;
}

/*******************************************************************
 Delete a SID from a SEC_DESC.
********************************************************************/

NTSTATUS sec_desc_del_sid(TALLOC_CTX *ctx, SEC_DESC **psd, DOM_SID *sid, size_t *sd_size)
{
	SEC_DESC *sd   = 0;
	SEC_ACL  *dacl = 0;
	SEC_ACE  *ace  = 0;
	NTSTATUS  status;

	if (!ctx || !psd[0] || !sid || !sd_size)
		return NT_STATUS_INVALID_PARAMETER;

	*sd_size = 0;
	
	status = sec_ace_del_sid(ctx, &ace, psd[0]->dacl->ace, &psd[0]->dacl->num_aces, sid);

	if (!NT_STATUS_IS_OK(status))
		return status;

	if (!(dacl = make_sec_acl(ctx, psd[0]->dacl->revision, psd[0]->dacl->num_aces, ace)))
		return NT_STATUS_UNSUCCESSFUL;
	
	if (!(sd = make_sec_desc(ctx, psd[0]->revision, psd[0]->type, psd[0]->owner_sid, 
		psd[0]->grp_sid, psd[0]->sacl, dacl, sd_size)))
		return NT_STATUS_UNSUCCESSFUL;

	*psd = sd;
	 sd  = 0;
	return NT_STATUS_OK;
}

/* Create a child security descriptor using another security descriptor as
   the parent container.  This child object can either be a container or
   non-container object. */

SEC_DESC_BUF *se_create_child_secdesc(TALLOC_CTX *ctx, SEC_DESC *parent_ctr, 
				      BOOL child_container)
{
	SEC_DESC_BUF *sdb;
	SEC_DESC *sd;
	SEC_ACL *new_dacl, *the_acl;
	SEC_ACE *new_ace_list = NULL;
	unsigned int new_ace_list_ndx = 0, i;
	size_t size;

	/* Currently we only process the dacl when creating the child.  The
	   sacl should also be processed but this is left out as sacls are
	   not implemented in Samba at the moment.*/

	the_acl = parent_ctr->dacl;

	if (!(new_ace_list = TALLOC_ARRAY(ctx, SEC_ACE, the_acl->num_aces))) 
		return NULL;

	for (i = 0; i < the_acl->num_aces; i++) {
		SEC_ACE *ace = &the_acl->ace[i];
		SEC_ACE *new_ace = &new_ace_list[new_ace_list_ndx];
		uint8 new_flags = 0;
		BOOL inherit = False;
		fstring sid_str;

		/* The OBJECT_INHERIT_ACE flag causes the ACE to be
		   inherited by non-container children objects.  Container
		   children objects will inherit it as an INHERIT_ONLY
		   ACE. */

		if (ace->flags & SEC_ACE_FLAG_OBJECT_INHERIT) {

			if (!child_container) {
				new_flags |= SEC_ACE_FLAG_OBJECT_INHERIT;
			} else {
				new_flags |= SEC_ACE_FLAG_INHERIT_ONLY;
			}

			inherit = True;
		}

		/* The CONAINER_INHERIT_ACE flag means all child container
		   objects will inherit and use the ACE. */

		if (ace->flags & SEC_ACE_FLAG_CONTAINER_INHERIT) {
			if (!child_container) {
				inherit = False;
			} else {
				new_flags |= SEC_ACE_FLAG_CONTAINER_INHERIT;
			}
		}

		/* The INHERIT_ONLY_ACE is not used by the se_access_check()
		   function for the parent container, but is inherited by
		   all child objects as a normal ACE. */

		if (ace->flags & SEC_ACE_FLAG_INHERIT_ONLY) {
			/* Move along, nothing to see here */
		}

		/* The SEC_ACE_FLAG_NO_PROPAGATE_INHERIT flag means the ACE
		   is inherited by child objects but not grandchildren
		   objects.  We clear the object inherit and container
		   inherit flags in the inherited ACE. */

		if (ace->flags & SEC_ACE_FLAG_NO_PROPAGATE_INHERIT) {
			new_flags &= ~(SEC_ACE_FLAG_OBJECT_INHERIT |
				       SEC_ACE_FLAG_CONTAINER_INHERIT);
		}

		/* Add ACE to ACE list */

		if (!inherit)
			continue;

		init_sec_access(&new_ace->info, ace->info.mask);
		init_sec_ace(new_ace, &ace->trustee, ace->type,
			     new_ace->info, new_flags);

		sid_to_string(sid_str, &ace->trustee);

		DEBUG(5, ("se_create_child_secdesc(): %s:%d/0x%02x/0x%08x "
			  " inherited as %s:%d/0x%02x/0x%08x\n", sid_str,
			  ace->type, ace->flags, ace->info.mask,
			  sid_str, new_ace->type, new_ace->flags,
			  new_ace->info.mask));

		new_ace_list_ndx++;
	}

	/* Create child security descriptor to return */
	
	new_dacl = make_sec_acl(ctx, ACL_REVISION, new_ace_list_ndx, new_ace_list);

	/* Use the existing user and group sids.  I don't think this is
	   correct.  Perhaps the user and group should be passed in as
	   parameters by the caller? */

	sd = make_sec_desc(ctx, SEC_DESC_REVISION, SEC_DESC_SELF_RELATIVE,
			   parent_ctr->owner_sid,
			   parent_ctr->grp_sid,
			   parent_ctr->sacl,
			   new_dacl, &size);

	sdb = make_sec_desc_buf(ctx, size, sd);

	return sdb;
}

/*******************************************************************
 Sets up a SEC_ACCESS structure.
********************************************************************/

void init_sec_access(SEC_ACCESS *t, uint32 mask)
{
	t->mask = mask;
}


