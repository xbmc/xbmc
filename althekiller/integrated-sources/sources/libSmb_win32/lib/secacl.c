/* 
 *  Unix SMB/Netbios implementation.
 *  SEC_ACL handling routines
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
 Create a SEC_ACL structure.  
********************************************************************/

SEC_ACL *make_sec_acl(TALLOC_CTX *ctx, uint16 revision, int num_aces, SEC_ACE *ace_list)
{
	SEC_ACL *dst;
	int i;

	if((dst = TALLOC_ZERO_P(ctx,SEC_ACL)) == NULL)
		return NULL;

	dst->revision = revision;
	dst->num_aces = num_aces;
	dst->size = SEC_ACL_HEADER_SIZE;

	/* Now we need to return a non-NULL address for the ace list even
	   if the number of aces required is zero.  This is because there
	   is a distinct difference between a NULL ace and an ace with zero
	   entries in it.  This is achieved by checking that num_aces is a
	   positive number. */

	if ((num_aces) && 
            ((dst->ace = TALLOC_ARRAY(ctx, SEC_ACE, num_aces)) 
             == NULL)) {
		return NULL;
	}
        
	for (i = 0; i < num_aces; i++) {
		dst->ace[i] = ace_list[i]; /* Structure copy. */
		dst->size += ace_list[i].size;
	}

	return dst;
}

/*******************************************************************
 Duplicate a SEC_ACL structure.  
********************************************************************/

SEC_ACL *dup_sec_acl(TALLOC_CTX *ctx, SEC_ACL *src)
{
	if(src == NULL)
		return NULL;

	return make_sec_acl(ctx, src->revision, src->num_aces, src->ace);
}

/*******************************************************************
 Compares two SEC_ACL structures
********************************************************************/

BOOL sec_acl_equal(SEC_ACL *s1, SEC_ACL *s2)
{
	unsigned int i, j;

	/* Trivial cases */

	if (!s1 && !s2) return True;
	if (!s1 || !s2) return False;

	/* Check top level stuff */

	if (s1->revision != s2->revision) {
		DEBUG(10, ("sec_acl_equal(): revision differs (%d != %d)\n",
			   s1->revision, s2->revision));
		return False;
	}

	if (s1->num_aces != s2->num_aces) {
		DEBUG(10, ("sec_acl_equal(): num_aces differs (%d != %d)\n",
			   s1->revision, s2->revision));
		return False;
	}

	/* The ACEs could be in any order so check each ACE in s1 against 
	   each ACE in s2. */

	for (i = 0; i < s1->num_aces; i++) {
		BOOL found = False;

		for (j = 0; j < s2->num_aces; j++) {
			if (sec_ace_equal(&s1->ace[i], &s2->ace[j])) {
				found = True;
				break;
			}
		}

		if (!found) return False;
	}

	return True;
}
