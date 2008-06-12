/* 
 *  Unix SMB/Netbios implementation.
 *  Version 1.9.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell              1992-1998,
 *  Copyright (C) Jeremy R. Allison            1995-2005.
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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_PARSE

/*******************************************************************
 Reads or writes a SEC_ACCESS structure.
********************************************************************/

BOOL sec_io_access(const char *desc, SEC_ACCESS *t, prs_struct *ps, int depth)
{
	if (t == NULL)
		return False;

	prs_debug(ps, depth, desc, "sec_io_access");
	depth++;
	
	if(!prs_uint32("mask", ps, depth, &t->mask))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a SEC_ACE structure.
********************************************************************/

BOOL sec_io_ace(const char *desc, SEC_ACE *psa, prs_struct *ps, int depth)
{
	uint32 old_offset;
	uint32 offset_ace_size;

	if (psa == NULL)
		return False;

	prs_debug(ps, depth, desc, "sec_io_ace");
	depth++;
	
	old_offset = prs_offset(ps);

	if(!prs_uint8("type ", ps, depth, &psa->type))
		return False;

	if(!prs_uint8("flags", ps, depth, &psa->flags))
		return False;

	if(!prs_uint16_pre("size ", ps, depth, &psa->size, &offset_ace_size))
		return False;

	if(!sec_io_access("info ", &psa->info, ps, depth))
		return False;

	/* check whether object access is present */
	if (!sec_ace_object(psa->type)) {
		if (!smb_io_dom_sid("trustee  ", &psa->trustee , ps, depth))
			return False;
	} else {
		if (!prs_uint32("obj_flags", ps, depth, &psa->obj_flags))
			return False;

		if (psa->obj_flags & SEC_ACE_OBJECT_PRESENT)
			if (!smb_io_uuid("obj_guid", &psa->obj_guid, ps,depth))
				return False;

		if (psa->obj_flags & SEC_ACE_OBJECT_INHERITED_PRESENT)
			if (!smb_io_uuid("inh_guid", &psa->inh_guid, ps,depth))
				return False;

		if(!smb_io_dom_sid("trustee  ", &psa->trustee , ps, depth))
			return False;
	}

	/* Theorectically an ACE can have a size greater than the
	   sum of its components. When marshalling, pad with extra null bytes up to the
	   correct size. */

	if (MARSHALLING(ps) && (psa->size > prs_offset(ps) - old_offset)) {
		uint32 extra_len = psa->size - (prs_offset(ps) - old_offset);
		uint32 i;
		uint8 c = 0;

		for (i = 0; i < extra_len; i++) {
			if (!prs_uint8("ace extra space", ps, depth, &c))
				return False;
		}
	}

	if(!prs_uint16_post("size ", ps, depth, &psa->size, offset_ace_size, old_offset))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a SEC_ACL structure.  

 First of the xx_io_xx functions that allocates its data structures
 for you as it reads them.
********************************************************************/

BOOL sec_io_acl(const char *desc, SEC_ACL **ppsa, prs_struct *ps, int depth)
{
	unsigned int i;
	uint32 old_offset;
	uint32 offset_acl_size;
	SEC_ACL *psa;

	/*
	 * Note that the size is always a multiple of 4 bytes due to the
	 * nature of the data structure.  Therefore the prs_align() calls
	 * have been removed as they through us off when doing two-layer
	 * marshalling such as in the printing code (RPC_BUFFER).  --jerry
	 */

	if (ppsa == NULL)
		return False;

	psa = *ppsa;

	if(UNMARSHALLING(ps) && psa == NULL) {
		/*
		 * This is a read and we must allocate the stuct to read into.
		 */
		if((psa = PRS_ALLOC_MEM(ps, SEC_ACL, 1)) == NULL)
			return False;
		*ppsa = psa;
	}

	prs_debug(ps, depth, desc, "sec_io_acl");
	depth++;
	
	old_offset = prs_offset(ps);

	if(!prs_uint16("revision", ps, depth, &psa->revision))
		return False;

	if(!prs_uint16_pre("size     ", ps, depth, &psa->size, &offset_acl_size))
		return False;

	if(!prs_uint32("num_aces ", ps, depth, &psa->num_aces))
		return False;

	if (UNMARSHALLING(ps)) {
		/*
		 * Even if the num_aces is zero, allocate memory as there's a difference
		 * between a non-present DACL (allow all access) and a DACL with no ACE's
		 * (allow no access).
		 */
		if((psa->ace = PRS_ALLOC_MEM(ps, SEC_ACE, psa->num_aces+1)) == NULL)
			return False;
	}

	for (i = 0; i < psa->num_aces; i++) {
		fstring tmp;
		slprintf(tmp, sizeof(tmp)-1, "ace_list[%02d]: ", i);
		if(!sec_io_ace(tmp, &psa->ace[i], ps, depth))
			return False;
	}

	/* Theorectically an ACL can have a size greater than the
	   sum of its components. When marshalling, pad with extra null bytes up to the
	   correct size. */

	if (MARSHALLING(ps) && (psa->size > prs_offset(ps) - old_offset)) {
		uint32 extra_len = psa->size - (prs_offset(ps) - old_offset);
		uint8 c = 0;

		for (i = 0; i < extra_len; i++) {
			if (!prs_uint8("acl extra space", ps, depth, &c))
				return False;
		}
	}

	if(!prs_uint16_post("size     ", ps, depth, &psa->size, offset_acl_size, old_offset))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a SEC_DESC structure.
 If reading and the *ppsd = NULL, allocates the structure.
********************************************************************/

BOOL sec_io_desc(const char *desc, SEC_DESC **ppsd, prs_struct *ps, int depth)
{
	uint32 old_offset;
	uint32 max_offset = 0; /* after we're done, move offset to end */
	uint32 tmp_offset = 0;

	SEC_DESC *psd;

	if (ppsd == NULL)
		return False;

	psd = *ppsd;

	if (psd == NULL) {
		if(UNMARSHALLING(ps)) {
			if((psd = PRS_ALLOC_MEM(ps,SEC_DESC,1)) == NULL)
				return False;
			*ppsd = psd;
		} else {
			/* Marshalling - just ignore. */
			return True;
		}
	}

	prs_debug(ps, depth, desc, "sec_io_desc");
	depth++;

	/* start of security descriptor stored for back-calc offset purposes */
	old_offset = prs_offset(ps);

	if(!prs_uint16("revision ", ps, depth, &psd->revision))
		return False;

	if(!prs_uint16("type     ", ps, depth, &psd->type))
		return False;

	if (MARSHALLING(ps)) {
		uint32 offset = SEC_DESC_HEADER_SIZE;

		/*
		 * Work out the offsets here, as we write it out.
		 */

		if (psd->sacl != NULL) {
			psd->off_sacl = offset;
			offset += psd->sacl->size;
		} else {
			psd->off_sacl = 0;
		}

		if (psd->dacl != NULL) {
			psd->off_dacl = offset;
			offset += psd->dacl->size;
		} else {
			psd->off_dacl = 0;
		}

		if (psd->owner_sid != NULL) {
			psd->off_owner_sid = offset;
			offset += sid_size(psd->owner_sid);
		} else {
			psd->off_owner_sid = 0;
		}

		if (psd->grp_sid != NULL) {
			psd->off_grp_sid = offset;
			offset += sid_size(psd->grp_sid);
		} else {
			psd->off_grp_sid = 0;
		}
	}

	if(!prs_uint32("off_owner_sid", ps, depth, &psd->off_owner_sid))
		return False;

	if(!prs_uint32("off_grp_sid  ", ps, depth, &psd->off_grp_sid))
		return False;

	if(!prs_uint32("off_sacl     ", ps, depth, &psd->off_sacl))
		return False;

	if(!prs_uint32("off_dacl     ", ps, depth, &psd->off_dacl))
		return False;

	max_offset = MAX(max_offset, prs_offset(ps));

	if (psd->off_owner_sid != 0) {

		tmp_offset = prs_offset(ps);
		if(!prs_set_offset(ps, old_offset + psd->off_owner_sid))
			return False;

		if (UNMARSHALLING(ps)) {
			/* reading */
			if((psd->owner_sid = PRS_ALLOC_MEM(ps,DOM_SID,1)) == NULL)
				return False;
		}

		if(!smb_io_dom_sid("owner_sid ", psd->owner_sid , ps, depth))
			return False;

		max_offset = MAX(max_offset, prs_offset(ps));

		if (!prs_set_offset(ps,tmp_offset))
			return False;
	}

	if (psd->off_grp_sid != 0) {

		tmp_offset = prs_offset(ps);
		if(!prs_set_offset(ps, old_offset + psd->off_grp_sid))
			return False;

		if (UNMARSHALLING(ps)) {
			/* reading */
			if((psd->grp_sid = PRS_ALLOC_MEM(ps,DOM_SID,1)) == NULL)
				return False;
		}

		if(!smb_io_dom_sid("grp_sid", psd->grp_sid, ps, depth))
			return False;
			
		max_offset = MAX(max_offset, prs_offset(ps));

		if (!prs_set_offset(ps,tmp_offset))
			return False;
	}

	if ((psd->type & SEC_DESC_SACL_PRESENT) && psd->off_sacl) {
		tmp_offset = prs_offset(ps);
		if(!prs_set_offset(ps, old_offset + psd->off_sacl))
			return False;
		if(!sec_io_acl("sacl", &psd->sacl, ps, depth))
			return False;
		max_offset = MAX(max_offset, prs_offset(ps));
		if (!prs_set_offset(ps,tmp_offset))
			return False;
	}

	if ((psd->type & SEC_DESC_DACL_PRESENT) && psd->off_dacl != 0) {
		tmp_offset = prs_offset(ps);
		if(!prs_set_offset(ps, old_offset + psd->off_dacl))
			return False;
		if(!sec_io_acl("dacl", &psd->dacl, ps, depth))
			return False;
		max_offset = MAX(max_offset, prs_offset(ps));
		if (!prs_set_offset(ps,tmp_offset))
			return False;
	}

	if(!prs_set_offset(ps, max_offset))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a SEC_DESC_BUF structure.
********************************************************************/

BOOL sec_io_desc_buf(const char *desc, SEC_DESC_BUF **ppsdb, prs_struct *ps, int depth)
{
	uint32 off_len;
	uint32 off_max_len;
	uint32 old_offset;
	uint32 size;
	SEC_DESC_BUF *psdb;

	if (ppsdb == NULL)
		return False;

	psdb = *ppsdb;

	if (UNMARSHALLING(ps) && psdb == NULL) {
		if((psdb = PRS_ALLOC_MEM(ps,SEC_DESC_BUF,1)) == NULL)
			return False;
		*ppsdb = psdb;
	}

	prs_debug(ps, depth, desc, "sec_io_desc_buf");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint32_pre("max_len", ps, depth, &psdb->max_len, &off_max_len))
		return False;

	if(!prs_uint32    ("ptr  ", ps, depth, &psdb->ptr))
		return False;

	if(!prs_uint32_pre("len    ", ps, depth, &psdb->len, &off_len))
		return False;

	old_offset = prs_offset(ps);

	/* reading, length is non-zero; writing, descriptor is non-NULL */
	if ((UNMARSHALLING(ps) && psdb->len != 0) || (MARSHALLING(ps) && psdb->sec != NULL)) {
		if(!sec_io_desc("sec   ", &psdb->sec, ps, depth))
			return False;
	}

	if(!prs_align(ps))
		return False;
	
	size = prs_offset(ps) - old_offset;
	if(!prs_uint32_post("max_len", ps, depth, &psdb->max_len, off_max_len, size == 0 ? psdb->max_len : size))
		return False;

	if(!prs_uint32_post("len    ", ps, depth, &psdb->len, off_len, size))
		return False;

	return True;
}
