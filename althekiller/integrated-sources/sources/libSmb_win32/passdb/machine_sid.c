/* 
   Unix SMB/CIFS implementation.
   Password and authentication handling
   Copyright (C) Jeremy Allison 		1996-2002
   Copyright (C) Andrew Tridgell		2002
   Copyright (C) Gerald (Jerry) Carter		2000
   Copyright (C) Stefan (metze) Metzmacher	2002
      
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

/* NOTE! the global_sam_sid is the SID of our local SAM. This is only
   equal to the domain SID when we are a DC, otherwise its our
   workstation SID */
static DOM_SID *global_sam_sid=NULL;

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_PASSDB

/****************************************************************************
 Read a SID from a file. This is for compatibility with the old MACHINE.SID
 style of SID storage
****************************************************************************/

static BOOL read_sid_from_file(const char *fname, DOM_SID *sid)
{
	char **lines;
	int numlines;
	BOOL ret;

	lines = file_lines_load(fname, &numlines,0);
	
	if (!lines || numlines < 1) {
		if (lines) file_lines_free(lines);
		return False;
	}
	
	ret = string_to_sid(sid, lines[0]);
	file_lines_free(lines);
	return ret;
}

/*
  generate a random sid - used to build our own sid if we don't have one
*/
static void generate_random_sid(DOM_SID *sid)
{
	int i;
	uchar raw_sid_data[12];

	memset((char *)sid, '\0', sizeof(*sid));
	sid->sid_rev_num = 1;
	sid->id_auth[5] = 5;
	sid->num_auths = 0;
	sid->sub_auths[sid->num_auths++] = 21;

	generate_random_buffer(raw_sid_data, 12);
	for (i = 0; i < 3; i++)
		sid->sub_auths[sid->num_auths++] = IVAL(raw_sid_data, i*4);
}

/****************************************************************************
 Generate the global machine sid.
****************************************************************************/

static DOM_SID *pdb_generate_sam_sid(void)
{
	DOM_SID domain_sid;
	char *fname = NULL;
	DOM_SID *sam_sid;
	
	if(!(sam_sid=SMB_MALLOC_P(DOM_SID)))
		return NULL;

	if ( IS_DC ) {
		if (secrets_fetch_domain_sid(lp_workgroup(), &domain_sid)) {
			sid_copy(sam_sid, &domain_sid);
			return sam_sid;
		}
	}

	if (secrets_fetch_domain_sid(global_myname(), sam_sid)) {

		/* We got our sid. If not a pdc/bdc, we're done. */
		if ( !IS_DC )
			return sam_sid;

		if (!secrets_fetch_domain_sid(lp_workgroup(), &domain_sid)) {

			/* No domain sid and we're a pdc/bdc. Store it */

			if (!secrets_store_domain_sid(lp_workgroup(), sam_sid)) {
				DEBUG(0,("pdb_generate_sam_sid: Can't store domain SID as a pdc/bdc.\n"));
				SAFE_FREE(sam_sid);
				return NULL;
			}
			return sam_sid;
		}

		if (!sid_equal(&domain_sid, sam_sid)) {

			/* Domain name sid doesn't match global sam sid. Re-store domain sid as 'local' sid. */

			DEBUG(0,("pdb_generate_sam_sid: Mismatched SIDs as a pdc/bdc.\n"));
			if (!secrets_store_domain_sid(global_myname(), &domain_sid)) {
				DEBUG(0,("pdb_generate_sam_sid: Can't re-store domain SID for local sid as PDC/BDC.\n"));
				SAFE_FREE(sam_sid);
				return NULL;
			}
			return sam_sid;
		}

		return sam_sid;
		
	}

	/* check for an old MACHINE.SID file for backwards compatibility */
	asprintf(&fname, "%s/MACHINE.SID", lp_private_dir());

	if (read_sid_from_file(fname, sam_sid)) {
		/* remember it for future reference and unlink the old MACHINE.SID */
		if (!secrets_store_domain_sid(global_myname(), sam_sid)) {
			DEBUG(0,("pdb_generate_sam_sid: Failed to store SID from file.\n"));
			SAFE_FREE(fname);
			SAFE_FREE(sam_sid);
			return NULL;
		}
		unlink(fname);
		if ( !IS_DC ) {
			if (!secrets_store_domain_sid(lp_workgroup(), sam_sid)) {
				DEBUG(0,("pdb_generate_sam_sid: Failed to store domain SID from file.\n"));
				SAFE_FREE(fname);
				SAFE_FREE(sam_sid);
				return NULL;
			}
		}

		/* Stored the old sid from MACHINE.SID successfully.*/
		SAFE_FREE(fname);
		return sam_sid;
	}

	SAFE_FREE(fname);

	/* we don't have the SID in secrets.tdb, we will need to
           generate one and save it */
	generate_random_sid(sam_sid);

	if (!secrets_store_domain_sid(global_myname(), sam_sid)) {
		DEBUG(0,("pdb_generate_sam_sid: Failed to store generated machine SID.\n"));
		SAFE_FREE(sam_sid);
		return NULL;
	}
	if ( IS_DC ) {
		if (!secrets_store_domain_sid(lp_workgroup(), sam_sid)) {
			DEBUG(0,("pdb_generate_sam_sid: Failed to store generated domain SID.\n"));
			SAFE_FREE(sam_sid);
			return NULL;
		}
	}

	return sam_sid;
}   

/* return our global_sam_sid */
DOM_SID *get_global_sam_sid(void)
{
	if (global_sam_sid != NULL)
		return global_sam_sid;
	
	/* memory for global_sam_sid is allocated in 
	   pdb_generate_sam_sid() as needed */

	if (!(global_sam_sid = pdb_generate_sam_sid())) {
		smb_panic("Could not generate a machine SID\n");
	}

	return global_sam_sid;
}

/** 
 * Force get_global_sam_sid to requery the backends 
 */
void reset_global_sam_sid(void) 
{
	SAFE_FREE(global_sam_sid);
}

/*****************************************************************
 Check if the SID is our domain SID (S-1-5-21-x-y-z).
*****************************************************************/  

BOOL sid_check_is_domain(const DOM_SID *sid)
{
	return sid_equal(sid, get_global_sam_sid());
}

/*****************************************************************
 Check if the SID is our domain SID (S-1-5-21-x-y-z).
*****************************************************************/  

BOOL sid_check_is_in_our_domain(const DOM_SID *sid)
{
	DOM_SID dom_sid;
	uint32 rid;

	sid_copy(&dom_sid, sid);
	sid_split_rid(&dom_sid, &rid);
	
	return sid_equal(&dom_sid, get_global_sam_sid());
}
