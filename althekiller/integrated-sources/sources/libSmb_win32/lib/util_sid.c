/* 
   Unix SMB/CIFS implementation.
   Samba utility functions
   Copyright (C) Andrew Tridgell 		1992-1998
   Copyright (C) Luke Kenneth Caseson Leighton 	1998-1999
   Copyright (C) Jeremy Allison  		1999
   Copyright (C) Stefan (metze) Metzmacher 	2002
   Copyright (C) Simo Sorce 			2002
   Copyright (C) Jim McDonough <jmcd@us.ibm.com> 2005
      
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

/*
 * Some useful sids
 */


const DOM_SID global_sid_World_Domain =               /* Everyone domain */
{ 1, 0, {0,0,0,0,0,1}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_World =                      /* Everyone */
{ 1, 1, {0,0,0,0,0,1}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_Creator_Owner_Domain =       /* Creator Owner domain */
{ 1, 0, {0,0,0,0,0,3}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_NT_Authority =    		/* NT Authority */
{ 1, 0, {0,0,0,0,0,5}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_System =			/* System */
{ 1, 1, {0,0,0,0,0,5}, {18,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_NULL =            		/* NULL sid */
{ 1, 1, {0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_Authenticated_Users =	/* All authenticated rids */
{ 1, 1, {0,0,0,0,0,5}, {11,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_Network =			/* Network rids */
{ 1, 1, {0,0,0,0,0,5}, {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};

const DOM_SID global_sid_Creator_Owner =		/* Creator Owner */
{ 1, 1, {0,0,0,0,0,3}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_Creator_Group =		/* Creator Group */
{ 1, 1, {0,0,0,0,0,3}, {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_Anonymous =			/* Anonymous login */
{ 1, 1, {0,0,0,0,0,5}, {7,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};

const DOM_SID global_sid_Builtin = 			/* Local well-known domain */
{ 1, 1, {0,0,0,0,0,5}, {32,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_Builtin_Administrators =	/* Builtin administrators */
{ 1, 2, {0,0,0,0,0,5}, {32,544,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_Builtin_Users =		/* Builtin users */
{ 1, 2, {0,0,0,0,0,5}, {32,545,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_Builtin_Guests =		/* Builtin guest users */
{ 1, 2, {0,0,0,0,0,5}, {32,546,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_Builtin_Power_Users =	/* Builtin power users */
{ 1, 2, {0,0,0,0,0,5}, {32,547,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_Builtin_Account_Operators =	/* Builtin account operators */
{ 1, 2, {0,0,0,0,0,5}, {32,548,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_Builtin_Server_Operators =	/* Builtin server operators */
{ 1, 2, {0,0,0,0,0,5}, {32,549,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_Builtin_Print_Operators =	/* Builtin print operators */
{ 1, 2, {0,0,0,0,0,5}, {32,550,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_Builtin_Backup_Operators =	/* Builtin backup operators */
{ 1, 2, {0,0,0,0,0,5}, {32,551,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_Builtin_Replicator =		/* Builtin replicator */
{ 1, 2, {0,0,0,0,0,5}, {32,552,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_Builtin_PreWin2kAccess =	/* Builtin pre win2k access */
{ 1, 2, {0,0,0,0,0,5}, {32,554,0,0,0,0,0,0,0,0,0,0,0,0,0}};

const DOM_SID global_sid_Unix_Users =			/* Unmapped Unix users */
{ 1, 1, {0,0,0,0,0,22}, {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const DOM_SID global_sid_Unix_Groups =			/* Unmapped Unix groups */
{ 1, 1, {0,0,0,0,0,22}, {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};

/* Unused, left here for documentary purposes */
#if 0
#define SECURITY_NULL_SID_AUTHORITY    0
#define SECURITY_WORLD_SID_AUTHORITY   1
#define SECURITY_LOCAL_SID_AUTHORITY   2
#define SECURITY_CREATOR_SID_AUTHORITY 3
#define SECURITY_NT_AUTHORITY          5
#endif

/*
 * An NT compatible anonymous token.
 */

static DOM_SID anon_sid_array[3] =
{ { 1, 1, {0,0,0,0,0,1}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
  { 1, 1, {0,0,0,0,0,5}, {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
  { 1, 1, {0,0,0,0,0,5}, {7,0,0,0,0,0,0,0,0,0,0,0,0,0,0}} };
NT_USER_TOKEN anonymous_token = { 3, anon_sid_array, SE_NONE };

static DOM_SID system_sid_array[1] =
{ { 1, 1, {0,0,0,0,0,5}, {18,0,0,0,0,0,0,0,0,0,0,0,0,0,0}} };
NT_USER_TOKEN system_token = { 1, system_sid_array, SE_ALL_PRIVS };

/****************************************************************************
 Lookup string names for SID types.
****************************************************************************/

static const struct {
	enum SID_NAME_USE sid_type;
	const char *string;
} sid_name_type[] = {
	{SID_NAME_USER, "User"},
	{SID_NAME_DOM_GRP, "Domain Group"},
	{SID_NAME_DOMAIN, "Domain"},
	{SID_NAME_ALIAS, "Local Group"},
	{SID_NAME_WKN_GRP, "Well-known Group"},
	{SID_NAME_DELETED, "Deleted Account"},
	{SID_NAME_INVALID, "Invalid Account"},
	{SID_NAME_UNKNOWN, "UNKNOWN"},
	{SID_NAME_COMPUTER, "Computer"},

 	{(enum SID_NAME_USE)0, NULL}
};

const char *sid_type_lookup(uint32 sid_type) 
{
	int i = 0;

	/* Look through list */
	while(sid_name_type[i].sid_type != 0) {
		if (sid_name_type[i].sid_type == sid_type)
			return sid_name_type[i].string;
		i++;
	}

	/* Default return */
	return "SID *TYPE* is INVALID";
}

/**************************************************************************
 Create the SYSTEM token.
***************************************************************************/

NT_USER_TOKEN *get_system_token(void) 
{
	return &system_token;
}

/******************************************************************
 get the default domain/netbios name to be used when dealing 
 with our passdb list of accounts
******************************************************************/

const char *get_global_sam_name(void) 
{
	if ((lp_server_role() == ROLE_DOMAIN_PDC) || (lp_server_role() == ROLE_DOMAIN_BDC)) {
		return lp_workgroup();
	}
	return global_myname();
}

/*****************************************************************
 Convert a SID to an ascii string.
*****************************************************************/

char *sid_to_string(fstring sidstr_out, const DOM_SID *sid)
{
	char subauth[16];
	int i;
	uint32 ia;
  
	if (!sid) {
		fstrcpy(sidstr_out, "(NULL SID)");
		return sidstr_out;
	}

	/*
	 * BIG NOTE: this function only does SIDS where the identauth is not >= 2^32 
	 * in a range of 2^48.
	 */
	ia = (sid->id_auth[5]) +
		(sid->id_auth[4] << 8 ) +
		(sid->id_auth[3] << 16) +
		(sid->id_auth[2] << 24);

	slprintf(sidstr_out, sizeof(fstring) - 1, "S-%u-%lu", (unsigned int)sid->sid_rev_num, (unsigned long)ia);

	for (i = 0; i < sid->num_auths; i++) {
		slprintf(subauth, sizeof(subauth)-1, "-%lu", (unsigned long)sid->sub_auths[i]);
		fstrcat(sidstr_out, subauth);
	}

	return sidstr_out;
}

/*****************************************************************
 Useful function for debug lines.
*****************************************************************/  

const char *sid_string_static(const DOM_SID *sid)
{
	static fstring sid_str;
	sid_to_string(sid_str, sid);
	return sid_str;
}

/*****************************************************************
 Convert a string to a SID. Returns True on success, False on fail.
*****************************************************************/  
   
BOOL string_to_sid(DOM_SID *sidout, const char *sidstr)
{
	const char *p;
	char *q;
	/* BIG NOTE: this function only does SIDS where the identauth is not >= 2^32 */
	uint32 conv;
  
	if ((sidstr[0] != 'S' && sidstr[0] != 's') || sidstr[1] != '-') {
		DEBUG(3,("string_to_sid: Sid %s does not start with 'S-'.\n", sidstr));
		return False;
	}

	ZERO_STRUCTP(sidout);

	/* Get the revision number. */
	p = sidstr + 2;
	conv = (uint32) strtoul(p, &q, 10);
	if (!q || (*q != '-')) {
		DEBUG(3,("string_to_sid: Sid %s is not in a valid format.\n", sidstr));
		return False;
	}
	sidout->sid_rev_num = (uint8) conv;
	q++;

	/* get identauth */
	conv = (uint32) strtoul(q, &q, 10);
	if (!q || (*q != '-')) {
		DEBUG(0,("string_to_sid: Sid %s is not in a valid format.\n", sidstr));
		return False;
	}
	/* identauth in decimal should be <  2^32 */
	/* NOTE - the conv value is in big-endian format. */
	sidout->id_auth[0] = 0;
	sidout->id_auth[1] = 0;
	sidout->id_auth[2] = (conv & 0xff000000) >> 24;
	sidout->id_auth[3] = (conv & 0x00ff0000) >> 16;
	sidout->id_auth[4] = (conv & 0x0000ff00) >> 8;
	sidout->id_auth[5] = (conv & 0x000000ff);

	q++;
	sidout->num_auths = 0;

	for(conv = (uint32) strtoul(q, &q, 10);
	    q && (*q =='-' || *q =='\0') && (sidout->num_auths < MAXSUBAUTHS);
	    conv = (uint32) strtoul(q, &q, 10)) {
		sid_append_rid(sidout, conv);
		if (*q == '\0')
			break;
		q++;
	}
		
	return True;
}

DOM_SID *string_sid_talloc(TALLOC_CTX *mem_ctx, const char *sidstr)
{
	DOM_SID *result = TALLOC_P(mem_ctx, DOM_SID);

	if (result == NULL)
		return NULL;

	if (!string_to_sid(result, sidstr))
		return NULL;

	return result;
}

/*****************************************************************
 Add a rid to the end of a sid
*****************************************************************/  

BOOL sid_append_rid(DOM_SID *sid, uint32 rid)
{
	if (sid->num_auths < MAXSUBAUTHS) {
		sid->sub_auths[sid->num_auths++] = rid;
		return True;
	}
	return False;
}

BOOL sid_compose(DOM_SID *dst, const DOM_SID *domain_sid, uint32 rid)
{
	sid_copy(dst, domain_sid);
	return sid_append_rid(dst, rid);
}

/*****************************************************************
 Removes the last rid from the end of a sid
*****************************************************************/  

BOOL sid_split_rid(DOM_SID *sid, uint32 *rid)
{
	if (sid->num_auths > 0) {
		sid->num_auths--;
		*rid = sid->sub_auths[sid->num_auths];
		return True;
	}
	return False;
}

/*****************************************************************
 Return the last rid from the end of a sid
*****************************************************************/  

BOOL sid_peek_rid(const DOM_SID *sid, uint32 *rid)
{
	if (!sid || !rid)
		return False;		
	
	if (sid->num_auths > 0) {
		*rid = sid->sub_auths[sid->num_auths - 1];
		return True;
	}
	return False;
}

/*****************************************************************
 Return the last rid from the end of a sid
 and check the sid against the exp_dom_sid  
*****************************************************************/  

BOOL sid_peek_check_rid(const DOM_SID *exp_dom_sid, const DOM_SID *sid, uint32 *rid)
{
	if (!exp_dom_sid || !sid || !rid)
		return False;
			
	if (sid->num_auths != (exp_dom_sid->num_auths+1)) {
		return False;
	}

	if (sid_compare_domain(exp_dom_sid, sid)!=0){
		*rid=(-1);
		return False;
	}
	
	return sid_peek_rid(sid, rid);
}

/*****************************************************************
 Copies a sid
*****************************************************************/  

void sid_copy(DOM_SID *dst, const DOM_SID *src)
{
	int i;

	ZERO_STRUCTP(dst);

	dst->sid_rev_num = src->sid_rev_num;
	dst->num_auths = src->num_auths;

	memcpy(&dst->id_auth[0], &src->id_auth[0], sizeof(src->id_auth));

	for (i = 0; i < src->num_auths; i++)
		dst->sub_auths[i] = src->sub_auths[i];
}

/*****************************************************************
 Write a sid out into on-the-wire format.
*****************************************************************/  

BOOL sid_linearize(char *outbuf, size_t len, const DOM_SID *sid)
{
	size_t i;

	if (len < sid_size(sid))
		return False;

	SCVAL(outbuf,0,sid->sid_rev_num);
	SCVAL(outbuf,1,sid->num_auths);
	memcpy(&outbuf[2], sid->id_auth, 6);
	for(i = 0; i < sid->num_auths; i++)
		SIVAL(outbuf, 8 + (i*4), sid->sub_auths[i]);

	return True;
}

/*****************************************************************
 Parse a on-the-wire SID to a DOM_SID.
*****************************************************************/  

BOOL sid_parse(const char *inbuf, size_t len, DOM_SID *sid)
{
	int i;
	if (len < 8)
		return False;

	ZERO_STRUCTP(sid);

	sid->sid_rev_num = CVAL(inbuf, 0);
	sid->num_auths = CVAL(inbuf, 1);
	memcpy(sid->id_auth, inbuf+2, 6);
	if (len < 8 + sid->num_auths*4)
		return False;
	for (i=0;i<sid->num_auths;i++)
		sid->sub_auths[i] = IVAL(inbuf, 8+i*4);
	return True;
}

/*****************************************************************
 Compare the auth portion of two sids.
*****************************************************************/  

static int sid_compare_auth(const DOM_SID *sid1, const DOM_SID *sid2)
{
	int i;

	if (sid1 == sid2)
		return 0;
	if (!sid1)
		return -1;
	if (!sid2)
		return 1;

	if (sid1->sid_rev_num != sid2->sid_rev_num)
		return sid1->sid_rev_num - sid2->sid_rev_num;

	for (i = 0; i < 6; i++)
		if (sid1->id_auth[i] != sid2->id_auth[i])
			return sid1->id_auth[i] - sid2->id_auth[i];

	return 0;
}

/*****************************************************************
 Compare two sids.
*****************************************************************/  

int sid_compare(const DOM_SID *sid1, const DOM_SID *sid2)
{
	int i;

	if (sid1 == sid2)
		return 0;
	if (!sid1)
		return -1;
	if (!sid2)
		return 1;

	/* Compare most likely different rids, first: i.e start at end */
	if (sid1->num_auths != sid2->num_auths)
		return sid1->num_auths - sid2->num_auths;

	for (i = sid1->num_auths-1; i >= 0; --i)
		if (sid1->sub_auths[i] != sid2->sub_auths[i])
			return sid1->sub_auths[i] - sid2->sub_auths[i];

	return sid_compare_auth(sid1, sid2);
}

/*****************************************************************
 See if 2 SIDs are in the same domain
 this just compares the leading sub-auths
*****************************************************************/  

int sid_compare_domain(const DOM_SID *sid1, const DOM_SID *sid2)
{
	int n, i;

	n = MIN(sid1->num_auths, sid2->num_auths);

	for (i = n-1; i >= 0; --i)
		if (sid1->sub_auths[i] != sid2->sub_auths[i])
			return sid1->sub_auths[i] - sid2->sub_auths[i];

	return sid_compare_auth(sid1, sid2);
}

/*****************************************************************
 Compare two sids.
*****************************************************************/  

BOOL sid_equal(const DOM_SID *sid1, const DOM_SID *sid2)
{
	return sid_compare(sid1, sid2) == 0;
}

/*****************************************************************
 Calculates size of a sid.
*****************************************************************/  

size_t sid_size(const DOM_SID *sid)
{
	if (sid == NULL)
		return 0;

	return sid->num_auths * sizeof(uint32) + 8;
}

/*****************************************************************
 Returns true if SID is internal (and non-mappable).
*****************************************************************/

BOOL non_mappable_sid(DOM_SID *sid)
{
	DOM_SID dom;
	uint32 rid;

	sid_copy(&dom, sid);
	sid_split_rid(&dom, &rid);

	if (sid_equal(&dom, &global_sid_Builtin))
		return True;

	if (sid_equal(&dom, &global_sid_NT_Authority))
		return True;

	return False;
}

/*****************************************************************
 Return the binary string representation of a DOM_SID.
 Caller must free.
*****************************************************************/

char *sid_binstring(const DOM_SID *sid)
{
	char *buf, *s;
	int len = sid_size(sid);
	buf = SMB_MALLOC(len);
	if (!buf)
		return NULL;
	sid_linearize(buf, len, sid);
	s = binary_string_rfc2254(buf, len);
	free(buf);
	return s;
}

/*****************************************************************
 Return the binary string representation of a DOM_SID.
 Caller must free.
*****************************************************************/

char *sid_binstring_hex(const DOM_SID *sid)
{
	char *buf, *s;
	int len = sid_size(sid);
	buf = SMB_MALLOC(len);
	if (!buf)
		return NULL;
	sid_linearize(buf, len, sid);
	s = binary_string(buf, len);
	free(buf);
	return s;
}

/*******************************************************************
 Tallocs a duplicate SID. 
********************************************************************/ 

DOM_SID *sid_dup_talloc(TALLOC_CTX *ctx, const DOM_SID *src)
{
	DOM_SID *dst;
	
	if(!src)
		return NULL;
	
	if((dst = TALLOC_ZERO_P(ctx, DOM_SID)) != NULL) {
		sid_copy( dst, src);
	}
	
	return dst;
}

/********************************************************************
 Add SID to an array SIDs
********************************************************************/

void add_sid_to_array(TALLOC_CTX *mem_ctx, const DOM_SID *sid, 
		      DOM_SID **sids, size_t *num)
{
	if (mem_ctx != NULL) {
		*sids = TALLOC_REALLOC_ARRAY(mem_ctx, *sids, DOM_SID,
					     (*num)+1);
	} else {
		*sids = SMB_REALLOC_ARRAY(*sids, DOM_SID, (*num)+1);
	}

	if (*sids == NULL) {
		return;
	}

	sid_copy(&((*sids)[*num]), sid);
	*num += 1;

	return;
}


/********************************************************************
 Add SID to an array SIDs ensuring that it is not already there
********************************************************************/

void add_sid_to_array_unique(TALLOC_CTX *mem_ctx, const DOM_SID *sid,
			     DOM_SID **sids, size_t *num_sids)
{
	size_t i;

	for (i=0; i<(*num_sids); i++) {
		if (sid_compare(sid, &(*sids)[i]) == 0)
			return;
	}

	add_sid_to_array(mem_ctx, sid, sids, num_sids);
}

/********************************************************************
 Remove SID from an array
********************************************************************/

void del_sid_from_array(const DOM_SID *sid, DOM_SID **sids, size_t *num)
{
	DOM_SID *sid_list = *sids;
	size_t i;

	for ( i=0; i<*num; i++ ) {

		/* if we find the SID, then decrement the count
		   and break out of the loop */

		if ( sid_equal(sid, &sid_list[i]) ) {
			*num -= 1;
			break;
		}
	}

	/* This loop will copy the remainder of the array 
	   if i < num of sids ni the array */

	for ( ; i<*num; i++ ) 
		sid_copy( &sid_list[i], &sid_list[i+1] );
	
	return;
}

void add_rid_to_array_unique(TALLOC_CTX *mem_ctx,
				    uint32 rid, uint32 **pp_rids, size_t *p_num)
{
	size_t i;

	for (i=0; i<*p_num; i++) {
		if ((*pp_rids)[i] == rid)
			return;
	}
	
	*pp_rids = TALLOC_REALLOC_ARRAY(mem_ctx, *pp_rids, uint32, *p_num+1);

	if (*pp_rids == NULL)
		return;

	(*pp_rids)[*p_num] = rid;
	*p_num += 1;
}

BOOL is_null_sid(const DOM_SID *sid)
{
	static const DOM_SID null_sid = {0};
	return sid_equal(sid, &null_sid);
}
