/* 
 *  Unix SMB/Netbios implementation.
 *  SEC_ACE handling functions
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

/*******************************************************************
 Check if ACE has OBJECT type.
********************************************************************/

BOOL sec_ace_object(uint8 type)
{
	if (type == SEC_ACE_TYPE_ACCESS_ALLOWED_OBJECT ||
            type == SEC_ACE_TYPE_ACCESS_DENIED_OBJECT ||
            type == SEC_ACE_TYPE_SYSTEM_AUDIT_OBJECT ||
            type == SEC_ACE_TYPE_SYSTEM_ALARM_OBJECT) {
		return True;
	}
	return False;
}

/*******************************************************************
 copy a SEC_ACE structure.
********************************************************************/
void sec_ace_copy(SEC_ACE *ace_dest, SEC_ACE *ace_src)
{
	ace_dest->type  = ace_src->type;
	ace_dest->flags = ace_src->flags;
	ace_dest->size  = ace_src->size;
	ace_dest->info.mask = ace_src->info.mask;
	ace_dest->obj_flags = ace_src->obj_flags;
	memcpy(&ace_dest->obj_guid, &ace_src->obj_guid, sizeof(struct uuid));
	memcpy(&ace_dest->inh_guid, &ace_src->inh_guid, sizeof(struct uuid));
	sid_copy(&ace_dest->trustee, &ace_src->trustee);
}

/*******************************************************************
 Sets up a SEC_ACE structure.
********************************************************************/

void init_sec_ace(SEC_ACE *t, const DOM_SID *sid, uint8 type, SEC_ACCESS mask, uint8 flag)
{
	t->type = type;
	t->flags = flag;
	t->size = sid_size(sid) + 8;
	t->info = mask;

	ZERO_STRUCTP(&t->trustee);
	sid_copy(&t->trustee, sid);
}

/*******************************************************************
 adds new SID with its permissions to ACE list
********************************************************************/

NTSTATUS sec_ace_add_sid(TALLOC_CTX *ctx, SEC_ACE **pp_new, SEC_ACE *old, unsigned *num, DOM_SID *sid, uint32 mask)
{
	unsigned int i = 0;
	
	if (!ctx || !pp_new || !old || !sid || !num)  return NT_STATUS_INVALID_PARAMETER;

	*num += 1;
	
	if((pp_new[0] = TALLOC_ZERO_ARRAY(ctx, SEC_ACE, *num )) == 0)
		return NT_STATUS_NO_MEMORY;

	for (i = 0; i < *num - 1; i ++)
		sec_ace_copy(&(*pp_new)[i], &old[i]);

	(*pp_new)[i].type  = 0;
	(*pp_new)[i].flags = 0;
	(*pp_new)[i].size  = SEC_ACE_HEADER_SIZE + sid_size(sid);
	(*pp_new)[i].info.mask = mask;
	sid_copy(&(*pp_new)[i].trustee, sid);
	return NT_STATUS_OK;
}

/*******************************************************************
  modify SID's permissions at ACL 
********************************************************************/

NTSTATUS sec_ace_mod_sid(SEC_ACE *ace, size_t num, DOM_SID *sid, uint32 mask)
{
	unsigned int i = 0;

	if (!ace || !sid)  return NT_STATUS_INVALID_PARAMETER;

	for (i = 0; i < num; i ++) {
		if (sid_compare(&ace[i].trustee, sid) == 0) {
			ace[i].info.mask = mask;
			return NT_STATUS_OK;
		}
	}
	return NT_STATUS_NOT_FOUND;
}

/*******************************************************************
 delete SID from ACL
********************************************************************/

NTSTATUS sec_ace_del_sid(TALLOC_CTX *ctx, SEC_ACE **pp_new, SEC_ACE *old, uint32 *num, DOM_SID *sid)
{
	unsigned int i     = 0;
	unsigned int n_del = 0;

	if (!ctx || !pp_new || !old || !sid || !num)  return NT_STATUS_INVALID_PARAMETER;

	if((pp_new[0] = TALLOC_ZERO_ARRAY(ctx, SEC_ACE, *num )) == 0)
		return NT_STATUS_NO_MEMORY;

	for (i = 0; i < *num; i ++) {
		if (sid_compare(&old[i].trustee, sid) != 0)
			sec_ace_copy(&(*pp_new)[i], &old[i]);
		else
			n_del ++;
	}
	if (n_del == 0)
		return NT_STATUS_NOT_FOUND;
	else {
		*num -= n_del;
		return NT_STATUS_OK;
	}
}

/*******************************************************************
 Compares two SEC_ACE structures
********************************************************************/

BOOL sec_ace_equal(SEC_ACE *s1, SEC_ACE *s2)
{
	/* Trivial case */

	if (!s1 && !s2) {
		return True;
	}

	if (!s1 || !s2) {
		return False;
	}

	/* Check top level stuff */

	if (s1->type != s2->type || s1->flags != s2->flags ||
	    s1->info.mask != s2->info.mask) {
		return False;
	}

	/* Check SID */

	if (!sid_equal(&s1->trustee, &s2->trustee)) {
		return False;
	}

	return True;
}

int nt_ace_inherit_comp( SEC_ACE *a1, SEC_ACE *a2)
{
	int a1_inh = a1->flags & SEC_ACE_FLAG_INHERITED_ACE;
	int a2_inh = a2->flags & SEC_ACE_FLAG_INHERITED_ACE;

	if (a1_inh == a2_inh)
		return 0;

	if (!a1_inh && a2_inh)
		return -1;
	return 1;
}

/*******************************************************************
  Comparison function to apply the order explained below in a group.
*******************************************************************/

int nt_ace_canon_comp( SEC_ACE *a1, SEC_ACE *a2)
{
	if ((a1->type == SEC_ACE_TYPE_ACCESS_DENIED) &&
				(a2->type != SEC_ACE_TYPE_ACCESS_DENIED))
		return -1;

	if ((a2->type == SEC_ACE_TYPE_ACCESS_DENIED) &&
				(a1->type != SEC_ACE_TYPE_ACCESS_DENIED))
		return 1;

	/* Both access denied or access allowed. */

	/* 1. ACEs that apply to the object itself */

	if (!(a1->flags & SEC_ACE_FLAG_INHERIT_ONLY) &&
			(a2->flags & SEC_ACE_FLAG_INHERIT_ONLY))
		return -1;
	else if (!(a2->flags & SEC_ACE_FLAG_INHERIT_ONLY) &&
			(a1->flags & SEC_ACE_FLAG_INHERIT_ONLY))
		return 1;

	/* 2. ACEs that apply to a subobject of the object, such as
	 * a property set or property. */

	if (a1->flags & (SEC_ACE_FLAG_CONTAINER_INHERIT|SEC_ACE_FLAG_OBJECT_INHERIT) &&
			!(a2->flags & (SEC_ACE_FLAG_CONTAINER_INHERIT|SEC_ACE_FLAG_OBJECT_INHERIT)))
		return -1;
	else if (a2->flags & (SEC_ACE_FLAG_CONTAINER_INHERIT|SEC_ACE_FLAG_OBJECT_INHERIT) &&
			!(a1->flags & (SEC_ACE_FLAG_CONTAINER_INHERIT|SEC_ACE_FLAG_OBJECT_INHERIT)))
		return 1;

	return 0;
}

/*******************************************************************
 Functions to convert a SEC_DESC ACE DACL list into canonical order.
 JRA.

--- from http://msdn.microsoft.com/library/default.asp?url=/library/en-us/security/security/order_of_aces_in_a_dacl.asp

The following describes the preferred order:

 To ensure that noninherited ACEs have precedence over inherited ACEs,
 place all noninherited ACEs in a group before any inherited ACEs.
 This ordering ensures, for example, that a noninherited access-denied ACE
 is enforced regardless of any inherited ACE that allows access.

 Within the groups of noninherited ACEs and inherited ACEs, order ACEs according to ACE type, as the following shows:
	1. Access-denied ACEs that apply to the object itself
	2. Access-denied ACEs that apply to a subobject of the object, such as a property set or property
	3. Access-allowed ACEs that apply to the object itself
	4. Access-allowed ACEs that apply to a subobject of the object"

********************************************************************/

void dacl_sort_into_canonical_order(SEC_ACE *srclist, unsigned int num_aces)
{
	unsigned int i;

	if (!srclist || num_aces == 0)
		return;

	/* Sort so that non-inherited ACE's come first. */
	qsort( srclist, num_aces, sizeof(srclist[0]), QSORT_CAST nt_ace_inherit_comp);

	/* Find the boundary between non-inherited ACEs. */
	for (i = 0; i < num_aces; i++ ) {
		SEC_ACE *curr_ace = &srclist[i];

		if (curr_ace->flags & SEC_ACE_FLAG_INHERITED_ACE)
			break;
	}

	/* i now points at entry number of the first inherited ACE. */

	/* Sort the non-inherited ACEs. */
	if (i)
		qsort( srclist, i, sizeof(srclist[0]), QSORT_CAST nt_ace_canon_comp);

	/* Now sort the inherited ACEs. */
	if (num_aces - i)
		qsort( &srclist[i], num_aces - i, sizeof(srclist[0]), QSORT_CAST nt_ace_canon_comp);
}

/*******************************************************************
 Check if this ACE has a SID in common with the token.
********************************************************************/

BOOL token_sid_in_ace(const NT_USER_TOKEN *token, const SEC_ACE *ace)
{
	size_t i;

	for (i = 0; i < token->num_sids; i++) {
		if (sid_equal(&ace->trustee, &token->user_sids[i]))
			return True;
	}

	return False;
}
