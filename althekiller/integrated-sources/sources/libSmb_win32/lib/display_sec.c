/* 
   Unix SMB/CIFS implementation.
   Samba utility functions
   Copyright (C) Andrew Tridgell 1992-1999
   Copyright (C) Luke Kenneth Casson Leighton 1996 - 1999
   
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

/****************************************************************************
convert a security permissions into a string
****************************************************************************/
char *get_sec_mask_str(uint32 type)
{
	static fstring typestr="";

	typestr[0] = 0;

	if (type & GENERIC_ALL_ACCESS)
		fstrcat(typestr, "Generic all access ");
	if (type & GENERIC_EXECUTE_ACCESS)
		fstrcat(typestr, "Generic execute access ");
	if (type & GENERIC_WRITE_ACCESS)
		fstrcat(typestr, "Generic write access ");
	if (type & GENERIC_READ_ACCESS)
		fstrcat(typestr, "Generic read access ");
	if (type & MAXIMUM_ALLOWED_ACCESS)
		fstrcat(typestr, "MAXIMUM_ALLOWED_ACCESS ");
	if (type & SYSTEM_SECURITY_ACCESS)
		fstrcat(typestr, "SYSTEM_SECURITY_ACCESS ");
	if (type & SYNCHRONIZE_ACCESS)
		fstrcat(typestr, "SYNCHRONIZE_ACCESS ");
	if (type & WRITE_OWNER_ACCESS)
		fstrcat(typestr, "WRITE_OWNER_ACCESS ");
	if (type & WRITE_DAC_ACCESS)
		fstrcat(typestr, "WRITE_DAC_ACCESS ");
	if (type & READ_CONTROL_ACCESS)
		fstrcat(typestr, "READ_CONTROL_ACCESS ");
	if (type & DELETE_ACCESS)
		fstrcat(typestr, "DELETE_ACCESS ");

	printf("\t\tSpecific bits: 0x%lx\n", (unsigned long)type&SPECIFIC_RIGHTS_MASK);

	return typestr;
}

/****************************************************************************
 display sec_access structure
 ****************************************************************************/
void display_sec_access(SEC_ACCESS *info)
{
	printf("\t\tPermissions: 0x%x: %s\n", info->mask, get_sec_mask_str(info->mask));
}

/****************************************************************************
 display sec_ace structure
 ****************************************************************************/
void display_sec_ace(SEC_ACE *ace)
{
	fstring sid_str;

	printf("\tACE\n\t\ttype: ");
	switch (ace->type) {
		case SEC_ACE_TYPE_ACCESS_ALLOWED:
			printf("ACCESS ALLOWED");
			break;
		case SEC_ACE_TYPE_ACCESS_DENIED:
			printf("ACCESS DENIED");
			break;
		case SEC_ACE_TYPE_SYSTEM_AUDIT:
			printf("SYSTEM AUDIT");
			break;
		case SEC_ACE_TYPE_SYSTEM_ALARM:
			printf("SYSTEM ALARM");
			break;
		default:
			printf("????");
			break;
	}
	printf(" (%d) flags: %d\n", ace->type, ace->flags);
	display_sec_access(&ace->info);
	sid_to_string(sid_str, &ace->trustee);
	printf("\t\tSID: %s\n\n", sid_str);
}

/****************************************************************************
 display sec_acl structure
 ****************************************************************************/
void display_sec_acl(SEC_ACL *sec_acl)
{
	int i;

	printf("\tACL\tNum ACEs:\t%d\trevision:\t%x\n",
			 sec_acl->num_aces, sec_acl->revision); 
	printf("\t---\n");

	if (sec_acl->size != 0 && sec_acl->num_aces != 0)
		for (i = 0; i < sec_acl->num_aces; i++)
			display_sec_ace(&sec_acl->ace[i]);
				
}

void display_acl_type(uint16 type)
{
	static fstring typestr="";

	typestr[0] = 0;

	if (type & SEC_DESC_OWNER_DEFAULTED)	/* 0x0001 */
		fstrcat(typestr, "SEC_DESC_OWNER_DEFAULTED ");
	if (type & SEC_DESC_GROUP_DEFAULTED)	/* 0x0002 */
		fstrcat(typestr, "SEC_DESC_GROUP_DEFAULTED ");
	if (type & SEC_DESC_DACL_PRESENT) 	/* 0x0004 */
		fstrcat(typestr, "SEC_DESC_DACL_PRESENT ");
	if (type & SEC_DESC_DACL_DEFAULTED)	/* 0x0008 */
		fstrcat(typestr, "SEC_DESC_DACL_DEFAULTED ");
	if (type & SEC_DESC_SACL_PRESENT)	/* 0x0010 */
		fstrcat(typestr, "SEC_DESC_SACL_PRESENT ");
	if (type & SEC_DESC_SACL_DEFAULTED)	/* 0x0020 */
		fstrcat(typestr, "SEC_DESC_SACL_DEFAULTED ");
	if (type & SEC_DESC_DACL_TRUSTED)	/* 0x0040 */
		fstrcat(typestr, "SEC_DESC_DACL_TRUSTED ");
	if (type & SEC_DESC_SERVER_SECURITY)	/* 0x0080 */
		fstrcat(typestr, "SEC_DESC_SERVER_SECURITY ");
	if (type & 0x0100) fstrcat(typestr, "0x0100 ");
	if (type & 0x0200) fstrcat(typestr, "0x0200 ");
	if (type & 0x0400) fstrcat(typestr, "0x0400 ");
	if (type & 0x0800) fstrcat(typestr, "0x0800 ");
	if (type & 0x1000) fstrcat(typestr, "0x1000 ");
	if (type & 0x2000) fstrcat(typestr, "0x2000 ");
	if (type & 0x4000) fstrcat(typestr, "0x4000 ");
	if (type & SEC_DESC_SELF_RELATIVE)	/* 0x8000 */
		fstrcat(typestr, "SEC_DESC_SELF_RELATIVE ");
	
	printf("type: 0x%04x: %s\n", type, typestr);
}

/****************************************************************************
 display sec_desc structure
 ****************************************************************************/
void display_sec_desc(SEC_DESC *sec)
{
	fstring sid_str;

	if (!sec) {
		printf("NULL\n");
		return;
	}

	printf("revision: %d\n", sec->revision);
	display_acl_type(sec->type);

	if (sec->sacl) {
		printf("SACL\n");
		display_sec_acl(sec->sacl);
	}

	if (sec->dacl) {
		printf("DACL\n");
		display_sec_acl(sec->dacl);
	}

	if (sec->owner_sid) {
		sid_to_string(sid_str, sec->owner_sid);
		printf("\tOwner SID:\t%s\n", sid_str);
	}

	if (sec->grp_sid) {
		sid_to_string(sid_str, sec->grp_sid);
		printf("\tParent SID:\t%s\n", sid_str);
	}
}
