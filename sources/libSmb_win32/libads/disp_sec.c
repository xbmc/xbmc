/* 
   Unix SMB/CIFS implementation.
   Samba utility functions. ADS stuff
   Copyright (C) Alexey Kotovich 2002
   
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

static struct perm_mask_str {
	uint32  mask;
	const char   *str;
} perms[] = {
	{SEC_RIGHTS_FULL_CTRL,		"[Full Control]"},

	{SEC_RIGHTS_LIST_CONTENTS,	"[List Contents]"},
	{SEC_RIGHTS_LIST_OBJECT,	"[List Object]"},

	{SEC_RIGHTS_READ_ALL_PROP,	"[Read All Properties]"},	
	{SEC_RIGHTS_READ_PERMS,		"[Read Permissions]"},	

	{SEC_RIGHTS_WRITE_ALL_VALID,	"[All validate writes]"},
	{SEC_RIGHTS_WRITE_ALL_PROP,  	"[Write All Properties]"},

	{SEC_RIGHTS_MODIFY_PERMS,	"[Modify Permissions]"},
	{SEC_RIGHTS_MODIFY_OWNER,	"[Modify Owner]"},

	{SEC_RIGHTS_CREATE_CHILD,	"[Create All Child Objects]"},

	{SEC_RIGHTS_DELETE,		"[Delete]"},
	{SEC_RIGHTS_DELETE_SUBTREE,	"[Delete Subtree]"},
	{SEC_RIGHTS_DELETE_CHILD,	"[Delete All Child Objects]"},

	{SEC_RIGHTS_CHANGE_PASSWD,	"[Change Password]"},	
	{SEC_RIGHTS_RESET_PASSWD,	"[Reset Password]"},
	{0,				0}
};

/* convert a security permissions into a string */
static void ads_disp_perms(uint32 type)
{
	int i = 0;
	int j = 0;

	printf("Permissions: ");
	
	if (type == SEC_RIGHTS_FULL_CTRL) {
		printf("%s\n", perms[j].str);
		return;
	}

	for (i = 0; i < 32; i++) {
		if (type & (1 << i)) {
			for (j = 1; perms[j].str; j ++) {
				if (perms[j].mask == (((unsigned) 1) << i)) {
					printf("\n\t%s", perms[j].str);
				}	
			}
			type &= ~(1 << i);
		}
	}

	/* remaining bits get added on as-is */
	if (type != 0) {
		printf("[%08x]", type);
	}
	puts("");
}

/* display ACE */
static void ads_disp_ace(SEC_ACE *sec_ace)
{
	const char *access_type = "UNKNOWN";

	if (!sec_ace_object(sec_ace->type)) {
		printf("------- ACE (type: 0x%02x, flags: 0x%02x, size: 0x%02x, mask: 0x%x)\n", 
		  sec_ace->type,
		  sec_ace->flags,
		  sec_ace->size,
		  sec_ace->info.mask);			
	} else {
		printf("------- ACE (type: 0x%02x, flags: 0x%02x, size: 0x%02x, mask: 0x%x, object flags: 0x%x)\n", 
		  sec_ace->type,
		  sec_ace->flags,
		  sec_ace->size,
		  sec_ace->info.mask,
		  sec_ace->obj_flags);
	}
	
	if (sec_ace->type == SEC_ACE_TYPE_ACCESS_ALLOWED) {
		access_type = "ALLOWED";
	} else if (sec_ace->type == SEC_ACE_TYPE_ACCESS_DENIED) {
		access_type = "DENIED";
	} else if (sec_ace->type == SEC_ACE_TYPE_SYSTEM_AUDIT) {
		access_type = "SYSTEM AUDIT";
	} else if (sec_ace->type == SEC_ACE_TYPE_ACCESS_ALLOWED_OBJECT) {
		access_type = "ALLOWED OBJECT";
	} else if (sec_ace->type == SEC_ACE_TYPE_ACCESS_DENIED_OBJECT) {
		access_type = "DENIED OBJECT";
	} else if (sec_ace->type == SEC_ACE_TYPE_SYSTEM_AUDIT_OBJECT) {
		access_type = "AUDIT OBJECT";
	}

	printf("access SID:  %s\naccess type: %s\n", 
               sid_string_static(&sec_ace->trustee), access_type);

	ads_disp_perms(sec_ace->info.mask);
}

/* display ACL */
static void ads_disp_acl(SEC_ACL *sec_acl, const char *type)
{
        if (!sec_acl)
		printf("------- (%s) ACL not present\n", type);
	else {
		printf("------- (%s) ACL (revision: %d, size: %d, number of ACEs: %d)\n", 
    	    	       type,
        	       sec_acl->revision,
	               sec_acl->size,
    		       sec_acl->num_aces);			
	}
}

/* display SD */
void ads_disp_sd(SEC_DESC *sd)
{
	int i;
	
	printf("-------------- Security Descriptor (revision: %d, type: 0x%02x)\n", 
               sd->revision,
               sd->type);
	printf("owner SID: %s\n", sid_string_static(sd->owner_sid));
	printf("group SID: %s\n", sid_string_static(sd->grp_sid));

	ads_disp_acl(sd->sacl, "system");
	for (i = 0; i < sd->sacl->num_aces; i ++)
		ads_disp_ace(&sd->sacl->ace[i]);
	
	ads_disp_acl(sd->dacl, "user");
	for (i = 0; i < sd->dacl->num_aces; i ++)
		ads_disp_ace(&sd->dacl->ace[i]);

	printf("-------------- End Of Security Descriptor\n");
}


