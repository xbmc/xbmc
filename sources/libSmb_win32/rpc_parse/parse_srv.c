/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell              1992-1997,
 *  Copyright (C) Luke Kenneth Casson Leighton 1996-1997,
 *  Copyright (C) Paul Ashton                       1997,
 *  Copyright (C) Jeremy Allison		    1999,
 *  Copyright (C) Nigel Williams		    2001,
 *  Copyright (C) Jim McDonough (jmcd@us.ibm.com)   2002.
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
 Inits a SH_INFO_0_STR structure
********************************************************************/

void init_srv_share_info0_str(SH_INFO_0_STR *sh0, const char *net_name)
{
	DEBUG(5,("init_srv_share_info0_str\n"));

	init_unistr2(&sh0->uni_netname, net_name, UNI_STR_TERMINATE);
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_share_info0_str(const char *desc, SH_INFO_0_STR *sh0, prs_struct *ps, int depth)
{
	if (sh0 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_share_info0_str");
	depth++;

	if(!prs_align(ps))
		return False;
	if(sh0->ptrs->ptr_netname)
		if(!smb_io_unistr2("", &sh0->uni_netname, True, ps, depth))
			return False;

	return True;
}

/*******************************************************************
 makes a SH_INFO_0 structure
********************************************************************/

void init_srv_share_info0(SH_INFO_0 *sh0, const char *net_name)
{
	DEBUG(5,("init_srv_share_info0: %s\n", net_name));

	sh0->ptr_netname = (net_name != NULL) ? 1 : 0;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_share_info0(const char *desc, SH_INFO_0 *sh0, prs_struct *ps, int depth)
{
	if (sh0 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_share_info0");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_netname", ps, depth, &sh0->ptr_netname))
		return False;

	return True;
}

/*******************************************************************
 Inits a SH_INFO_1_STR structure
********************************************************************/

void init_srv_share_info1_str(SH_INFO_1_STR *sh1, const char *net_name, const char *remark)
{
	DEBUG(5,("init_srv_share_info1_str\n"));

	init_unistr2(&sh1->uni_netname, net_name, UNI_STR_TERMINATE);
	init_unistr2(&sh1->uni_remark, remark, UNI_STR_TERMINATE);
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_share_info1_str(const char *desc, SH_INFO_1_STR *sh1, prs_struct *ps, int depth)
{
	if (sh1 == NULL)
		return False;
	
	prs_debug(ps, depth, desc, "srv_io_share_info1_str");
	depth++;
	
	if(!prs_align(ps))
		return False;

	if(sh1->ptrs->ptr_netname)
		if(!smb_io_unistr2("", &sh1->uni_netname, True, ps, depth))
			return False;
	
	if(!prs_align(ps))
		return False;
	
	if(sh1->ptrs->ptr_remark)
		if(!smb_io_unistr2("", &sh1->uni_remark, True, ps, depth))
			return False;
	
	return True;
}

/*******************************************************************
 makes a SH_INFO_1 structure
********************************************************************/

void init_srv_share_info1(SH_INFO_1 *sh1, const char *net_name, uint32 type, const char *remark)
{
	DEBUG(5,("init_srv_share_info1: %s %8x %s\n", net_name, type, remark));
	
	sh1->ptr_netname = (net_name != NULL) ? 1 : 0;
	sh1->type        = type;
	sh1->ptr_remark  = (remark != NULL) ? 1 : 0;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_share_info1(const char *desc, SH_INFO_1 *sh1, prs_struct *ps, int depth)
{
	if (sh1 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_share_info1");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_netname", ps, depth, &sh1->ptr_netname))
		return False;
	if(!prs_uint32("type       ", ps, depth, &sh1->type))
		return False;
	if(!prs_uint32("ptr_remark ", ps, depth, &sh1->ptr_remark))
		return False;

	return True;
}

/*******************************************************************
 Inits a SH_INFO_2_STR structure
********************************************************************/

void init_srv_share_info2_str(SH_INFO_2_STR *sh2,
				const char *net_name, const char *remark,
				const char *path, const char *passwd)
{
	DEBUG(5,("init_srv_share_info2_str\n"));

	init_unistr2(&sh2->uni_netname, net_name, UNI_STR_TERMINATE);
	init_unistr2(&sh2->uni_remark, remark, UNI_STR_TERMINATE);
	init_unistr2(&sh2->uni_path, path, UNI_STR_TERMINATE);
	init_unistr2(&sh2->uni_passwd, passwd, UNI_STR_TERMINATE);
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_share_info2_str(const char *desc, SH_INFO_2 *sh, SH_INFO_2_STR *sh2, prs_struct *ps, int depth)
{
	if (sh2 == NULL)
		return False;

	if (UNMARSHALLING(ps))
		ZERO_STRUCTP(sh2);

	prs_debug(ps, depth, desc, "srv_io_share_info2_str");
	depth++;

	if(!prs_align(ps))
		return False;

	if (sh->ptr_netname)
		if(!smb_io_unistr2("", &sh2->uni_netname, True, ps, depth))
			return False;

	if (sh->ptr_remark)
		if(!smb_io_unistr2("", &sh2->uni_remark, True, ps, depth))
			return False;

	if (sh->ptr_netname)
		if(!smb_io_unistr2("", &sh2->uni_path, True, ps, depth))
			return False;

	if (sh->ptr_passwd)
		if(!smb_io_unistr2("", &sh2->uni_passwd, True, ps, depth))
			return False;

	return True;
}

/*******************************************************************
 Inits a SH_INFO_2 structure
********************************************************************/

void init_srv_share_info2(SH_INFO_2 *sh2,
				const char *net_name, uint32 type, const char *remark,
				uint32 perms, uint32 max_uses, uint32 num_uses,
				const char *path, const char *passwd)
{
	DEBUG(5,("init_srv_share_info2: %s %8x %s\n", net_name, type, remark));

	sh2->ptr_netname = (net_name != NULL) ? 1 : 0;
	sh2->type        = type;
	sh2->ptr_remark  = (remark != NULL) ? 1 : 0;
	sh2->perms       = perms;
	sh2->max_uses    = max_uses;
	sh2->num_uses    = num_uses;
	sh2->ptr_path    = (path != NULL) ? 1 : 0;
	sh2->ptr_passwd  = (passwd != NULL) ? 1 : 0;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_share_info2(const char *desc, SH_INFO_2 *sh2, prs_struct *ps, int depth)
{
	if (sh2 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_share_info2");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_netname", ps, depth, &sh2->ptr_netname))
		return False;
	if(!prs_uint32("type       ", ps, depth, &sh2->type))
		return False;
	if(!prs_uint32("ptr_remark ", ps, depth, &sh2->ptr_remark))
		return False;
	if(!prs_uint32("perms      ", ps, depth, &sh2->perms))
		return False;
	if(!prs_uint32("max_uses   ", ps, depth, &sh2->max_uses))
		return False;
	if(!prs_uint32("num_uses   ", ps, depth, &sh2->num_uses))
		return False;
	if(!prs_uint32("ptr_path   ", ps, depth, &sh2->ptr_path))
		return False;
	if(!prs_uint32("ptr_passwd ", ps, depth, &sh2->ptr_passwd))
		return False;

	return True;
}

/*******************************************************************
 Inits a SH_INFO_501_STR structure
********************************************************************/

void init_srv_share_info501_str(SH_INFO_501_STR *sh501,
				const char *net_name, const char *remark)
{
	DEBUG(5,("init_srv_share_info501_str\n"));

	init_unistr2(&sh501->uni_netname, net_name, UNI_STR_TERMINATE);
	init_unistr2(&sh501->uni_remark, remark, UNI_STR_TERMINATE);
}

/*******************************************************************
 Inits a SH_INFO_2 structure
*******************************************************************/

void init_srv_share_info501(SH_INFO_501 *sh501, const char *net_name, uint32 type, const char *remark, uint32 csc_policy)
{
	DEBUG(5,("init_srv_share_info501: %s %8x %s %08x\n", net_name, type,
		remark, csc_policy));

	ZERO_STRUCTP(sh501);

	sh501->ptr_netname = (net_name != NULL) ? 1 : 0;
	sh501->type = type;
	sh501->ptr_remark = (remark != NULL) ? 1 : 0;
	sh501->csc_policy = csc_policy;
}

/*******************************************************************
 Reads of writes a structure.
*******************************************************************/

static BOOL srv_io_share_info501(const char *desc, SH_INFO_501 *sh501, prs_struct *ps, int depth)
{
	if (sh501 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_share_info501");
	depth++;

	if (!prs_align(ps))
		return False;

	if (!prs_uint32("ptr_netname", ps, depth, &sh501->ptr_netname))
		return False;
	if (!prs_uint32("type     ", ps, depth, &sh501->type))
		return False;
	if (!prs_uint32("ptr_remark ", ps, depth, &sh501->ptr_remark))
		return False;
	if (!prs_uint32("csc_policy ", ps, depth, &sh501->csc_policy))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_share_info501_str(const char *desc, SH_INFO_501_STR *sh501, prs_struct *ps, int depth)
{
	if (sh501 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_share_info501_str");
	depth++;

	if(!prs_align(ps))
		return False;
	if(!smb_io_unistr2("", &sh501->uni_netname, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;
	if(!smb_io_unistr2("", &sh501->uni_remark, True, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Inits a SH_INFO_502 structure
********************************************************************/

void init_srv_share_info502(SH_INFO_502 *sh502,
				const char *net_name, uint32 type, const char *remark,
				uint32 perms, uint32 max_uses, uint32 num_uses,
				const char *path, const char *passwd, SEC_DESC *psd, size_t sd_size)
{
	DEBUG(5,("init_srv_share_info502: %s %8x %s\n", net_name, type, remark));

	ZERO_STRUCTP(sh502);

	sh502->ptr_netname = (net_name != NULL) ? 1 : 0;
	sh502->type        = type;
	sh502->ptr_remark  = (remark != NULL) ? 1 : 0;
	sh502->perms       = perms;
	sh502->max_uses    = max_uses;
	sh502->num_uses    = num_uses;
	sh502->ptr_path    = (path != NULL) ? 1 : 0;
	sh502->ptr_passwd  = (passwd != NULL) ? 1 : 0;
	sh502->reserved    = 0;  /* actual size within rpc */
	sh502->sd_size     = (uint32)sd_size;
	sh502->ptr_sd      = (psd != NULL) ? 1 : 0;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_share_info502(const char *desc, SH_INFO_502 *sh502, prs_struct *ps, int depth)
{
	if (sh502 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_share_info502");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_netname", ps, depth, &sh502->ptr_netname))
		return False;
	if(!prs_uint32("type       ", ps, depth, &sh502->type))
		return False;
	if(!prs_uint32("ptr_remark ", ps, depth, &sh502->ptr_remark))
		return False;
	if(!prs_uint32("perms      ", ps, depth, &sh502->perms))
		return False;
	if(!prs_uint32("max_uses   ", ps, depth, &sh502->max_uses))
		return False;
	if(!prs_uint32("num_uses   ", ps, depth, &sh502->num_uses))
		return False;
	if(!prs_uint32("ptr_path   ", ps, depth, &sh502->ptr_path))
		return False;
	if(!prs_uint32("ptr_passwd ", ps, depth, &sh502->ptr_passwd))
		return False;
	if(!prs_uint32_pre("reserved   ", ps, depth, &sh502->reserved, &sh502->reserved_offset))
		return False;
	if(!prs_uint32("ptr_sd     ", ps, depth, &sh502->ptr_sd))
		return False;

	return True;
}

/*******************************************************************
 Inits a SH_INFO_502_STR structure
********************************************************************/

void init_srv_share_info502_str(SH_INFO_502_STR *sh502str,
				const char *net_name, const char *remark,
				const char *path, const char *passwd, SEC_DESC *psd, size_t sd_size)
{
	DEBUG(5,("init_srv_share_info502_str\n"));

	init_unistr2(&sh502str->uni_netname, net_name, UNI_STR_TERMINATE);
	init_unistr2(&sh502str->uni_remark, remark, UNI_STR_TERMINATE);
	init_unistr2(&sh502str->uni_path, path, UNI_STR_TERMINATE);
	init_unistr2(&sh502str->uni_passwd, passwd, UNI_STR_TERMINATE);
	sh502str->sd = psd;
	sh502str->reserved = 0;
	sh502str->sd_size = sd_size;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_share_info502_str(const char *desc, SH_INFO_502_STR *sh502, prs_struct *ps, int depth)
{
	if (sh502 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_share_info502_str");
	depth++;

	if(!prs_align(ps))
		return False;

	if(sh502->ptrs->ptr_netname) {
		if(!smb_io_unistr2("", &sh502->uni_netname, True, ps, depth))
			return False;
	}

	if(!prs_align(ps))
		return False;

	if(sh502->ptrs->ptr_remark) {
		if(!smb_io_unistr2("", &sh502->uni_remark, True, ps, depth))
			return False;
	}

	if(!prs_align(ps))
		return False;

	if(sh502->ptrs->ptr_path) {
		if(!smb_io_unistr2("", &sh502->uni_path, True, ps, depth))
			return False;
	}

	if(!prs_align(ps))
		return False;

	if(sh502->ptrs->ptr_passwd) {
		if(!smb_io_unistr2("", &sh502->uni_passwd, True, ps, depth))
			return False;
	}

	if(!prs_align(ps))
		return False;

	if(sh502->ptrs->ptr_sd) {
		uint32 old_offset;
		uint32 reserved_offset;

		if(!prs_uint32_pre("reserved ", ps, depth, &sh502->reserved, &reserved_offset))
			return False;
	  
		old_offset = prs_offset(ps);
	  
		if (!sec_io_desc(desc, &sh502->sd, ps, depth))
			return False;

		if(UNMARSHALLING(ps)) {

			sh502->ptrs->sd_size = sh502->sd_size = sec_desc_size(sh502->sd);

			prs_set_offset(ps, old_offset + sh502->reserved);
		}

		prs_align(ps);

		if(MARSHALLING(ps)) {

			sh502->ptrs->reserved = sh502->reserved = prs_offset(ps) - old_offset;
		}
	    
		if(!prs_uint32_post("reserved ", ps, depth, 
				    &sh502->reserved, reserved_offset, sh502->reserved))
			return False;
		if(!prs_uint32_post("reserved ", ps, depth, 
				    &sh502->ptrs->reserved, sh502->ptrs->reserved_offset, sh502->ptrs->reserved))
			return False;
	}

	return True;
}

/*******************************************************************
 Inits a SH_INFO_1004_STR structure
********************************************************************/

void init_srv_share_info1004_str(SH_INFO_1004_STR *sh1004, const char *remark)
{
	DEBUG(5,("init_srv_share_info1004_str\n"));

	init_unistr2(&sh1004->uni_remark, remark, UNI_STR_TERMINATE);
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_share_info1004_str(const char *desc, SH_INFO_1004_STR *sh1004, prs_struct *ps, int depth)
{
	if (sh1004 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_share_info1004_str");
	depth++;

	if(!prs_align(ps))
		return False;
	if(sh1004->ptrs->ptr_remark)
		if(!smb_io_unistr2("", &sh1004->uni_remark, True, ps, depth))
			return False;

	return True;
}

/*******************************************************************
 makes a SH_INFO_1004 structure
********************************************************************/

void init_srv_share_info1004(SH_INFO_1004 *sh1004, const char *remark)
{
	DEBUG(5,("init_srv_share_info1004: %s\n", remark));

	sh1004->ptr_remark = (remark != NULL) ? 1 : 0;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_share_info1004(const char *desc, SH_INFO_1004 *sh1004, prs_struct *ps, int depth)
{
	if (sh1004 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_share_info1004");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_remark", ps, depth, &sh1004->ptr_remark))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_share_info1005(const char* desc, SRV_SHARE_INFO_1005* sh1005, prs_struct* ps, int depth)
{
	if(sh1005 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_share_info1005");
		depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("share_info_flags", ps, depth, 
		       &sh1005->share_info_flags))
		return False;

	return True;
}   

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_share_info1006(const char* desc, SRV_SHARE_INFO_1006* sh1006, prs_struct* ps, int depth)
{
	if(sh1006 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_share_info1006");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("max uses     ", ps, depth, &sh1006->max_uses))
		return False;

	return True;
}   

/*******************************************************************
 Inits a SH_INFO_1007_STR structure
********************************************************************/

void init_srv_share_info1007_str(SH_INFO_1007_STR *sh1007, const char *alternate_directory_name)
{
	DEBUG(5,("init_srv_share_info1007_str\n"));

	init_unistr2(&sh1007->uni_AlternateDirectoryName, alternate_directory_name, UNI_STR_TERMINATE);
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_share_info1007_str(const char *desc, SH_INFO_1007_STR *sh1007, prs_struct *ps, int depth)
{
	if (sh1007 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_share_info1007_str");
	depth++;

	if(!prs_align(ps))
		return False;
	if(sh1007->ptrs->ptr_AlternateDirectoryName)
		if(!smb_io_unistr2("", &sh1007->uni_AlternateDirectoryName, True, ps, depth))
			return False;

	return True;
}

/*******************************************************************
 makes a SH_INFO_1007 structure
********************************************************************/

void init_srv_share_info1007(SH_INFO_1007 *sh1007, uint32 flags, const char *alternate_directory_name)
{
	DEBUG(5,("init_srv_share_info1007: %s\n", alternate_directory_name));

	sh1007->flags                      = flags;
	sh1007->ptr_AlternateDirectoryName = (alternate_directory_name != NULL) ? 1 : 0;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_share_info1007(const char *desc, SH_INFO_1007 *sh1007, prs_struct *ps, int depth)
{
	if (sh1007 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_share_info1007");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("flags      ", ps, depth, &sh1007->flags))
		return False;
	if(!prs_uint32("ptr_Alter..", ps, depth, &sh1007->ptr_AlternateDirectoryName))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_share_info1501(const char* desc, SRV_SHARE_INFO_1501* sh1501,
				  prs_struct* ps, int depth)
{
	if(sh1501 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_share_info1501");
	depth++;

	if(!prs_align(ps))
		return False;

	if (!sec_io_desc_buf(desc, &sh1501->sdb, ps, depth))
		return False;

	return True;
}   

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_srv_share_ctr(const char *desc, SRV_SHARE_INFO_CTR *ctr, prs_struct *ps, int depth)
{
	if (ctr == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_srv_share_ctr");
	depth++;

	if (UNMARSHALLING(ps)) {
		memset(ctr, '\0', sizeof(SRV_SHARE_INFO_CTR));
	}

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("info_level", ps, depth, &ctr->info_level))
		return False;

	if(!prs_uint32("switch_value", ps, depth, &ctr->switch_value))
		return False;
	if(!prs_uint32("ptr_share_info", ps, depth, &ctr->ptr_share_info))
		return False;

	if (ctr->ptr_share_info == 0)
		return True;

	if(!prs_uint32("num_entries", ps, depth, &ctr->num_entries))
		return False;
	if(!prs_uint32("ptr_entries", ps, depth, &ctr->ptr_entries))
		return False;

	if (ctr->ptr_entries == 0) {
		if (ctr->num_entries == 0)
			return True;
		else
			return False;
	}

	if(!prs_uint32("num_entries2", ps, depth, &ctr->num_entries2))
		return False;

	if (ctr->num_entries2 != ctr->num_entries)
		return False;

	switch (ctr->switch_value) {

	case 0:
	{
		SRV_SHARE_INFO_0 *info0 = ctr->share.info0;
		int num_entries = ctr->num_entries;
		int i;

		if (UNMARSHALLING(ps)) {
			if (!(info0 = PRS_ALLOC_MEM(ps, SRV_SHARE_INFO_0, num_entries)))
				return False;
			ctr->share.info0 = info0;
		}

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_share_info0("", &info0[i].info_0, ps, depth))
				return False;
		}

		for (i = 0; i < num_entries; i++) {
			info0[i].info_0_str.ptrs = &info0[i].info_0;
			if(!srv_io_share_info0_str("", &info0[i].info_0_str, ps, depth))
				return False;
		}

		break;
	}

	case 1:
	{
		SRV_SHARE_INFO_1 *info1 = ctr->share.info1;
		int num_entries = ctr->num_entries;
		int i;

		if (UNMARSHALLING(ps)) {
			if (!(info1 = PRS_ALLOC_MEM(ps, SRV_SHARE_INFO_1, num_entries)))
				return False;
			ctr->share.info1 = info1;
		}

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_share_info1("", &info1[i].info_1, ps, depth))
				return False;
		}

		for (i = 0; i < num_entries; i++) {
			info1[i].info_1_str.ptrs = &info1[i].info_1;
			if(!srv_io_share_info1_str("", &info1[i].info_1_str, ps, depth))
				return False;
		}

		break;
	}

	case 2:
	{
		SRV_SHARE_INFO_2 *info2 = ctr->share.info2;
		int num_entries = ctr->num_entries;
		int i;

		if (UNMARSHALLING(ps)) {
			if (!(info2 = PRS_ALLOC_MEM(ps,SRV_SHARE_INFO_2,num_entries)))
				return False;
			ctr->share.info2 = info2;
		}

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_share_info2("", &info2[i].info_2, ps, depth))
				return False;
		}

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_share_info2_str("", &info2[i].info_2, &info2[i].info_2_str, ps, depth))
				return False;
		}

		break;
	}

	case 501:
	{
		SRV_SHARE_INFO_501 *info501 = ctr->share.info501;
		int num_entries = ctr->num_entries;
		int i;

		if (UNMARSHALLING(ps)) {
			if (!(info501 = PRS_ALLOC_MEM(ps, SRV_SHARE_INFO_501, num_entries)))
				return False;
			ctr->share.info501 = info501;
		}

		for (i = 0; i < num_entries; i++) {
			if (!srv_io_share_info501("", &info501[i].info_501, ps, depth))
				return False;
		}

		for (i = 0; i < num_entries; i++) {
			if (!srv_io_share_info501_str("", &info501[i].info_501_str, ps, depth))
				return False;
		}

		break;
	}

	case 502:
	{
		SRV_SHARE_INFO_502 *info502 = ctr->share.info502;
		int num_entries = ctr->num_entries;
		int i;

		if (UNMARSHALLING(ps)) {
			if (!(info502 = PRS_ALLOC_MEM(ps,SRV_SHARE_INFO_502,num_entries)))
				return False;
			ctr->share.info502 = info502;
		}

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_share_info502("", &info502[i].info_502, ps, depth))
				return False;
	}
		
		for (i = 0; i < num_entries; i++) {
			info502[i].info_502_str.ptrs = &info502[i].info_502;
			if(!srv_io_share_info502_str("", &info502[i].info_502_str, ps, depth))
				return False;
		}

		break;
	}

	case 1004:
	{
		SRV_SHARE_INFO_1004 *info1004 = ctr->share.info1004;
		int num_entries = ctr->num_entries;
		int i;

		if (UNMARSHALLING(ps)) {
			if (!(info1004 = PRS_ALLOC_MEM(ps,SRV_SHARE_INFO_1004,num_entries)))
				return False;
			ctr->share.info1004 = info1004;
		}

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_share_info1004("", &info1004[i].info_1004, ps, depth))
				return False;
		}

		for (i = 0; i < num_entries; i++) {
			info1004[i].info_1004_str.ptrs = &info1004[i].info_1004;
			if(!srv_io_share_info1004_str("", &info1004[i].info_1004_str, ps, depth))
				return False;
		}

		break;
	}

	case 1005:
	{
		SRV_SHARE_INFO_1005 *info1005 = ctr->share.info1005;
		int num_entries = ctr->num_entries;
		int i;

		if (UNMARSHALLING(ps)) {
			if (!(info1005 = PRS_ALLOC_MEM(ps,SRV_SHARE_INFO_1005,num_entries)))
				return False;
			ctr->share.info1005 = info1005;
		}

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_share_info1005("", &info1005[i], ps, depth))
				return False;
		}

		break;
	}

	case 1006:
	{
		SRV_SHARE_INFO_1006 *info1006 = ctr->share.info1006;
		int num_entries = ctr->num_entries;
		int i;

		if (UNMARSHALLING(ps)) {
			if (!(info1006 = PRS_ALLOC_MEM(ps,SRV_SHARE_INFO_1006,num_entries)))
				return False;
			ctr->share.info1006 = info1006;
		}

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_share_info1006("", &info1006[i], ps, depth))
				return False;
		}

		break;
	}

	case 1007:
	{
		SRV_SHARE_INFO_1007 *info1007 = ctr->share.info1007;
		int num_entries = ctr->num_entries;
		int i;

		if (UNMARSHALLING(ps)) {
			if (!(info1007 = PRS_ALLOC_MEM(ps,SRV_SHARE_INFO_1007,num_entries)))
				return False;
			ctr->share.info1007 = info1007;
		}

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_share_info1007("", &info1007[i].info_1007, ps, depth))
				return False;
		}

		for (i = 0; i < num_entries; i++) {
			info1007[i].info_1007_str.ptrs = &info1007[i].info_1007;
			if(!srv_io_share_info1007_str("", &info1007[i].info_1007_str, ps, depth))
				return False;
		}

		break;
	}

	case 1501:
	{
		SRV_SHARE_INFO_1501 *info1501 = ctr->share.info1501;
		int num_entries = ctr->num_entries;
		int i;

		if (UNMARSHALLING(ps)) {
			if (!(info1501 = PRS_ALLOC_MEM(ps,SRV_SHARE_INFO_1501,num_entries)))
				return False;
			ctr->share.info1501 = info1501;
		}

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_share_info1501("", &info1501[i], ps, depth))
				return False;
		}

		break;
	}

	default:
		DEBUG(5,("%s no share info at switch_value %d\n",
			 tab_depth(depth), ctr->switch_value));
		break;
	}

	return True;
}

/*******************************************************************
 Inits a SRV_Q_NET_SHARE_ENUM structure.
********************************************************************/

void init_srv_q_net_share_enum(SRV_Q_NET_SHARE_ENUM *q_n, 
				const char *srv_name, uint32 info_level,
				uint32 preferred_len, ENUM_HND *hnd)
{

	DEBUG(5,("init_q_net_share_enum\n"));

	init_buf_unistr2(&q_n->uni_srv_name, &q_n->ptr_srv_name, srv_name);

	q_n->ctr.info_level = q_n->ctr.switch_value = info_level;
	q_n->ctr.ptr_share_info = 1;
	q_n->ctr.num_entries  = 0;
	q_n->ctr.ptr_entries  = 0;
	q_n->ctr.num_entries2 = 0;
	q_n->preferred_len = preferred_len;

	memcpy(&q_n->enum_hnd, hnd, sizeof(*hnd));
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_q_net_share_enum(const char *desc, SRV_Q_NET_SHARE_ENUM *q_n, prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_q_net_share_enum");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name", ps, depth, &q_n->ptr_srv_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_srv_name, True, ps, depth))
		return False;

	if(!srv_io_srv_share_ctr("share_ctr", &q_n->ctr, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("preferred_len", ps, depth, &q_n->preferred_len))
		return False;

	if(!smb_io_enum_hnd("enum_hnd", &q_n->enum_hnd, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_r_net_share_enum(const char *desc, SRV_R_NET_SHARE_ENUM *r_n, prs_struct *ps, int depth)
{
	if (r_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_r_net_share_enum");
	depth++;

	if(!srv_io_srv_share_ctr("share_ctr", &r_n->ctr, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("total_entries", ps, depth, &r_n->total_entries))
		return False;

	if(!smb_io_enum_hnd("enum_hnd", &r_n->enum_hnd, ps, depth))
		return False;

	if(!prs_werror("status", ps, depth, &r_n->status))
		return False;

	return True;
}

/*******************************************************************
 initialises a structure.
********************************************************************/

BOOL init_srv_q_net_share_get_info(SRV_Q_NET_SHARE_GET_INFO *q_n, const char *srv_name, const char *share_name, uint32 info_level)
{

	uint32 ptr_share_name;

	DEBUG(5,("init_srv_q_net_share_get_info\n"));

	init_buf_unistr2(&q_n->uni_srv_name,   &q_n->ptr_srv_name, srv_name);
	init_buf_unistr2(&q_n->uni_share_name, &ptr_share_name,    share_name);

	q_n->info_level = info_level;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_q_net_share_get_info(const char *desc, SRV_Q_NET_SHARE_GET_INFO *q_n, prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_q_net_share_get_info");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name", ps, depth, &q_n->ptr_srv_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_srv_name, True, ps, depth))
		return False;

	if(!smb_io_unistr2("", &q_n->uni_share_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("info_level", ps, depth, &q_n->info_level))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_srv_share_info(const char *desc, prs_struct *ps, int depth, SRV_SHARE_INFO *r_n)
{
	if (r_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_srv_share_info");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("switch_value ", ps, depth, &r_n->switch_value ))
		return False;

	if(!prs_uint32("ptr_share_ctr", ps, depth, &r_n->ptr_share_ctr))
		return False;

	if (r_n->ptr_share_ctr != 0) {
		switch (r_n->switch_value) {
		case 0:
			if(!srv_io_share_info0("", &r_n->share.info0.info_0, ps, depth))
				return False;

			/* allow access to pointers in the str part. */
			r_n->share.info0.info_0_str.ptrs = &r_n->share.info0.info_0;

			if(!srv_io_share_info0_str("", &r_n->share.info0.info_0_str, ps, depth))
				return False;

			break;
		case 1:
			if(!srv_io_share_info1("", &r_n->share.info1.info_1, ps, depth))
				return False;

			/* allow access to pointers in the str part. */
			r_n->share.info1.info_1_str.ptrs = &r_n->share.info1.info_1;

			if(!srv_io_share_info1_str("", &r_n->share.info1.info_1_str, ps, depth))
				return False;

			break;
		case 2:
			if(!srv_io_share_info2("", &r_n->share.info2.info_2, ps, depth))
				return False;

			if(!srv_io_share_info2_str("", &r_n->share.info2.info_2, &r_n->share.info2.info_2_str, ps, depth))
				return False;

			break;
		case 501:
			if (!srv_io_share_info501("", &r_n->share.info501.info_501, ps, depth))
				return False;
			if (!srv_io_share_info501_str("", &r_n->share.info501.info_501_str, ps, depth))
				return False;
			break;

		case 502:
			if(!srv_io_share_info502("", &r_n->share.info502.info_502, ps, depth))
				return False;

			/* allow access to pointers in the str part. */
			r_n->share.info502.info_502_str.ptrs = &r_n->share.info502.info_502;

			if(!srv_io_share_info502_str("", &r_n->share.info502.info_502_str, ps, depth))
				return False;
			break;
		case 1004:
			if(!srv_io_share_info1004("", &r_n->share.info1004.info_1004, ps, depth))
				return False;

			/* allow access to pointers in the str part. */
			r_n->share.info1004.info_1004_str.ptrs = &r_n->share.info1004.info_1004;

			if(!srv_io_share_info1004_str("", &r_n->share.info1004.info_1004_str, ps, depth))
				return False;
			break;
		case 1005:
			if(!srv_io_share_info1005("", &r_n->share.info1005, ps, depth))
				return False;  		
			break;
		case 1006:
			if(!srv_io_share_info1006("", &r_n->share.info1006, ps, depth))
				return False;  		
			break;
		case 1007:
			if(!srv_io_share_info1007("", &r_n->share.info1007.info_1007, ps, depth))
				return False;

			/* allow access to pointers in the str part. */
			r_n->share.info1007.info_1007_str.ptrs = &r_n->share.info1007.info_1007;

			if(!srv_io_share_info1007_str("", &r_n->share.info1007.info_1007_str, ps, depth))
				return False;
			break;
		case 1501:
			if (!srv_io_share_info1501("", &r_n->share.info1501, ps, depth))
				return False;
		default:
		        DEBUG(5,("%s no share info at switch_value %d\n",
			         tab_depth(depth), r_n->switch_value));
			break;
		}
	}

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_r_net_share_get_info(const char *desc, SRV_R_NET_SHARE_GET_INFO *r_n, prs_struct *ps, int depth)
{
	if (r_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_r_net_share_get_info");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!srv_io_srv_share_info("info  ", ps, depth, &r_n->info))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_werror("status", ps, depth, &r_n->status))
		return False;

	return True;
}

/*******************************************************************
 intialises a structure.
********************************************************************/

BOOL init_srv_q_net_share_set_info(SRV_Q_NET_SHARE_SET_INFO *q_n, 
				   const char *srv_name, 
				   const char *share_name, 
				   uint32 info_level, 
				   const SRV_SHARE_INFO *info) 
{

	uint32 ptr_share_name;

	DEBUG(5,("init_srv_q_net_share_set_info\n"));

	init_buf_unistr2(&q_n->uni_srv_name,   &q_n->ptr_srv_name, srv_name);
	init_buf_unistr2(&q_n->uni_share_name, &ptr_share_name,    share_name);

	q_n->info_level = info_level;
  
	q_n->info = *info;

	q_n->ptr_parm_error = 1;
	q_n->parm_error     = 0;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_q_net_share_set_info(const char *desc, SRV_Q_NET_SHARE_SET_INFO *q_n, prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_q_net_share_set_info");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name", ps, depth, &q_n->ptr_srv_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_srv_name, True, ps, depth))
		return False;

	if(!smb_io_unistr2("", &q_n->uni_share_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("info_level", ps, depth, &q_n->info_level))
		return False;

	if(!prs_align(ps))
		return False;

	if(!srv_io_srv_share_info("info  ", ps, depth, &q_n->info))
		return False;

	if(!prs_align(ps))
		return False;
	if(!prs_uint32("ptr_parm_error", ps, depth, &q_n->ptr_parm_error))
		return False;
	if(q_n->ptr_parm_error!=0) {
		if(!prs_uint32("parm_error", ps, depth, &q_n->parm_error))
			return False;
	}

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_r_net_share_set_info(const char *desc, SRV_R_NET_SHARE_SET_INFO *r_n, prs_struct *ps, int depth)
{
	if (r_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_r_net_share_set_info");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_parm_error  ", ps, depth, &r_n->ptr_parm_error))
		return False;

	if(r_n->ptr_parm_error) {

		if(!prs_uint32("parm_error  ", ps, depth, &r_n->parm_error))
			return False;
	}

	if(!prs_werror("status", ps, depth, &r_n->status))
		return False;

	return True;
}	


/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_q_net_share_add(const char *desc, SRV_Q_NET_SHARE_ADD *q_n, prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_q_net_share_add");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name", ps, depth, &q_n->ptr_srv_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_srv_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("info_level", ps, depth, &q_n->info_level))
		return False;

	if(!prs_align(ps))
		return False;

	if(!srv_io_srv_share_info("info  ", ps, depth, &q_n->info))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_err_index", ps, depth, &q_n->ptr_err_index))
		return False;
	if (q_n->ptr_err_index)
		if (!prs_uint32("err_index", ps, depth, &q_n->err_index))
			return False;

	return True;
}

void init_srv_q_net_share_add(SRV_Q_NET_SHARE_ADD *q, const char *srvname,
			      const char *netname, uint32 type, const char *remark, 
			      uint32 perms, uint32 max_uses, uint32 num_uses,
			      const char *path, const char *passwd, 
			      int level, SEC_DESC *sd)
{
	switch(level) {
	case 502: {
		size_t sd_size = sec_desc_size(sd);
		q->ptr_srv_name = 1;
		init_unistr2(&q->uni_srv_name, srvname, UNI_STR_TERMINATE);
		q->info.switch_value = q->info_level = level;
		q->info.ptr_share_ctr = 1;
		init_srv_share_info502(&q->info.share.info502.info_502, netname, type,
				     remark, perms, max_uses, num_uses, path, passwd, sd, sd_size);
		init_srv_share_info502_str(&q->info.share.info502.info_502_str, netname,
					 remark, path, passwd, sd, sd_size);
		q->ptr_err_index = 1;
		q->err_index = 0;
		}
		break;
	case 2:
	default:
		q->ptr_srv_name = 1;
		init_unistr2(&q->uni_srv_name, srvname, UNI_STR_TERMINATE);
		q->info.switch_value = q->info_level = level;
		q->info.ptr_share_ctr = 1;
		init_srv_share_info2(&q->info.share.info2.info_2, netname, type,
				     remark, perms, max_uses, num_uses, path, passwd);
		init_srv_share_info2_str(&q->info.share.info2.info_2_str, netname,
					 remark, path, passwd);
		q->ptr_err_index = 1;
		q->err_index = 0;
		break;
	}
}


/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_r_net_share_add(const char *desc, SRV_R_NET_SHARE_ADD *r_n, prs_struct *ps, int depth)
{
	if (r_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_r_net_share_add");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_parm_error", ps, depth, &r_n->ptr_parm_error))
		return False;

	if(r_n->ptr_parm_error) {
	  
		if(!prs_uint32("parm_error", ps, depth, &r_n->parm_error))
			return False;
	}

	if(!prs_werror("status", ps, depth, &r_n->status))
		return False;

	return True;
}	

/*******************************************************************
 initialises a structure.
********************************************************************/

void init_srv_q_net_share_del(SRV_Q_NET_SHARE_DEL *del, const char *srvname,
			      const char *sharename)
{
	del->ptr_srv_name = 1;
	init_unistr2(&del->uni_srv_name, srvname, UNI_STR_TERMINATE);
	init_unistr2(&del->uni_share_name, sharename, UNI_STR_TERMINATE);
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_q_net_share_del(const char *desc, SRV_Q_NET_SHARE_DEL *q_n, prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_q_net_share_del");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name", ps, depth, &q_n->ptr_srv_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_srv_name, True, ps, depth))
		return False;

	if(!smb_io_unistr2("", &q_n->uni_share_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;
	if(!prs_uint32("reserved", ps, depth, &q_n->reserved))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_r_net_share_del(const char *desc, SRV_R_NET_SHARE_DEL *q_n, prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_r_net_share_del");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_werror("status", ps, depth, &q_n->status))
		return False;

	return True;
}	

/*******************************************************************
 Inits a SESS_INFO_0_STR structure
********************************************************************/

void init_srv_sess_info0_str(SESS_INFO_0_STR *ss0, const char *name)
{
	DEBUG(5,("init_srv_sess_info0_str\n"));

	init_unistr2(&ss0->uni_name, name, UNI_STR_TERMINATE);
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_sess_info0_str(const char *desc,  SESS_INFO_0_STR *ss0, prs_struct *ps, int depth)
{
	if (ss0 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_sess_info0_str");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_unistr2("", &ss0->uni_name, True, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Inits a SESS_INFO_0 structure
********************************************************************/

void init_srv_sess_info0(SESS_INFO_0 *ss0, const char *name)
{
	DEBUG(5,("init_srv_sess_info0: %s\n", name));

	ss0->ptr_name = (name != NULL) ? 1 : 0;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_sess_info0(const char *desc, SESS_INFO_0 *ss0, prs_struct *ps, int depth)
{
	if (ss0 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_sess_info0");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_name", ps, depth, &ss0->ptr_name))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_srv_sess_info_0(const char *desc, SRV_SESS_INFO_0 *ss0, prs_struct *ps, int depth)
{
	if (ss0 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_srv_sess_info_0");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("num_entries_read", ps, depth, &ss0->num_entries_read))
		return False;
	if(!prs_uint32("ptr_sess_info", ps, depth, &ss0->ptr_sess_info))
		return False;

	if (ss0->ptr_sess_info != 0) {
		uint32 i;
		uint32 num_entries = ss0->num_entries_read;

		if (num_entries > MAX_SESS_ENTRIES) {
			num_entries = MAX_SESS_ENTRIES; /* report this! */
		}

		if(!prs_uint32("num_entries_read2", ps, depth, &ss0->num_entries_read2))
			return False;

		SMB_ASSERT_ARRAY(ss0->info_0, num_entries);

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_sess_info0("", &ss0->info_0[i], ps, depth))
				return False;
		}

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_sess_info0_str("", &ss0->info_0_str[i], ps, depth))
				return False;
		}

		if(!prs_align(ps))
			return False;
	}

	return True;
}

/*******************************************************************
 Inits a SESS_INFO_1_STR structure
********************************************************************/

void init_srv_sess_info1_str(SESS_INFO_1_STR *ss1, const char *name, const char *user)
{
	DEBUG(5,("init_srv_sess_info1_str\n"));

	init_unistr2(&ss1->uni_name, name, UNI_STR_TERMINATE);
	init_unistr2(&ss1->uni_user, user, UNI_STR_TERMINATE);
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_sess_info1_str(const char *desc, SESS_INFO_1_STR *ss1, prs_struct *ps, int depth)
{
	if (ss1 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_sess_info1_str");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_unistr2("", &ss1->uni_name, True, ps, depth))
		return False;
	if(!smb_io_unistr2("", &(ss1->uni_user), True, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Inits a SESS_INFO_1 structure
********************************************************************/

void init_srv_sess_info1(SESS_INFO_1 *ss1, 
				const char *name, const char *user,
				uint32 num_opens, uint32 open_time, uint32 idle_time,
				uint32 user_flags)
{
	DEBUG(5,("init_srv_sess_info1: %s\n", name));

	ss1->ptr_name = (name != NULL) ? 1 : 0;
	ss1->ptr_user = (user != NULL) ? 1 : 0;

	ss1->num_opens  = num_opens;
	ss1->open_time  = open_time;
	ss1->idle_time  = idle_time;
	ss1->user_flags = user_flags;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL srv_io_sess_info1(const char *desc, SESS_INFO_1 *ss1, prs_struct *ps, int depth)
{
	if (ss1 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_sess_info1");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_name  ", ps, depth, &ss1->ptr_name))
		return False;
	if(!prs_uint32("ptr_user  ", ps, depth, &ss1->ptr_user))
		return False;

	if(!prs_uint32("num_opens ", ps, depth, &ss1->num_opens))
		return False;
	if(!prs_uint32("open_time ", ps, depth, &ss1->open_time))
		return False;
	if(!prs_uint32("idle_time ", ps, depth, &ss1->idle_time))
		return False;
	if(!prs_uint32("user_flags", ps, depth, &ss1->user_flags))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_srv_sess_info_1(const char *desc, SRV_SESS_INFO_1 *ss1, prs_struct *ps, int depth)
{
	if (ss1 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_srv_sess_info_1");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("num_entries_read", ps, depth, &ss1->num_entries_read))
		return False;
	if(!prs_uint32("ptr_sess_info", ps, depth, &ss1->ptr_sess_info))
		return False;

	if (ss1->ptr_sess_info != 0) {
		uint32 i;
		uint32 num_entries = ss1->num_entries_read;

		if (num_entries > MAX_SESS_ENTRIES) {
			num_entries = MAX_SESS_ENTRIES; /* report this! */
		}

		if(!prs_uint32("num_entries_read2", ps, depth, &ss1->num_entries_read2))
			return False;

		SMB_ASSERT_ARRAY(ss1->info_1, num_entries);

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_sess_info1("", &ss1->info_1[i], ps, depth))
				return False;
		}

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_sess_info1_str("", &ss1->info_1_str[i], ps, depth))
				return False;
		}

		if(!prs_align(ps))
			return False;
	}

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_srv_sess_ctr(const char *desc, SRV_SESS_INFO_CTR **pp_ctr, prs_struct *ps, int depth)
{
	SRV_SESS_INFO_CTR *ctr = *pp_ctr;

	prs_debug(ps, depth, desc, "srv_io_srv_sess_ctr");
	depth++;

	if(UNMARSHALLING(ps)) {
		ctr = *pp_ctr = PRS_ALLOC_MEM(ps, SRV_SESS_INFO_CTR, 1);
		if (ctr == NULL)
			return False;
	}

	if (ctr == NULL)
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("switch_value", ps, depth, &ctr->switch_value))
		return False;
	if(!prs_uint32("ptr_sess_ctr", ps, depth, &ctr->ptr_sess_ctr))
		return False;

	if (ctr->ptr_sess_ctr != 0) {
		switch (ctr->switch_value) {
		case 0:
			if(!srv_io_srv_sess_info_0("", &ctr->sess.info0, ps, depth))
				return False;
			break;
		case 1:
			if(!srv_io_srv_sess_info_1("", &ctr->sess.info1, ps, depth))
				return False;
			break;
		default:
			DEBUG(5,("%s no session info at switch_value %d\n",
			         tab_depth(depth), ctr->switch_value));
			break;
		}
	}

	return True;
}

/*******************************************************************
 Inits a SRV_Q_NET_SESS_ENUM structure.
********************************************************************/

void init_srv_q_net_sess_enum(SRV_Q_NET_SESS_ENUM *q_n, 
			      const char *srv_name, const char *qual_name,
			      const char *user_name, uint32 sess_level, 
			      SRV_SESS_INFO_CTR *ctr, uint32 preferred_len,
			      ENUM_HND *hnd)
{
	q_n->ctr = ctr;

	DEBUG(5,("init_q_net_sess_enum\n"));

	init_buf_unistr2(&q_n->uni_srv_name, &q_n->ptr_srv_name, srv_name);
	init_buf_unistr2(&q_n->uni_qual_name, &q_n->ptr_qual_name, qual_name);
	init_buf_unistr2(&q_n->uni_user_name, &q_n->ptr_user_name, user_name);

	q_n->sess_level    = sess_level;
	q_n->preferred_len = preferred_len;

	memcpy(&q_n->enum_hnd, hnd, sizeof(*hnd));
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_q_net_sess_enum(const char *desc, SRV_Q_NET_SESS_ENUM *q_n, prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_q_net_sess_enum");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name", ps, depth, &q_n->ptr_srv_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_srv_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_qual_name", ps, depth, &q_n->ptr_qual_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_qual_name, q_n->ptr_qual_name, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;
	if(!prs_uint32("ptr_user_name", ps, depth, &q_n->ptr_user_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_user_name, q_n->ptr_user_name, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("sess_level", ps, depth, &q_n->sess_level))
		return False;
	
	if (q_n->sess_level != (uint32)-1) {
		if(!srv_io_srv_sess_ctr("sess_ctr", &q_n->ctr, ps, depth))
			return False;
	}

	if(!prs_uint32("preferred_len", ps, depth, &q_n->preferred_len))
		return False;

	if(!smb_io_enum_hnd("enum_hnd", &q_n->enum_hnd, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_r_net_sess_enum(const char *desc, SRV_R_NET_SESS_ENUM *r_n, prs_struct *ps, int depth)
{
	if (r_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_r_net_sess_enum");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("sess_level", ps, depth, &r_n->sess_level))
		return False;

	if (r_n->sess_level != (uint32)-1) {
		if(!srv_io_srv_sess_ctr("sess_ctr", &r_n->ctr, ps, depth))
			return False;
	}

	if(!prs_uint32("total_entries", ps, depth, &r_n->total_entries))
		return False;
	if(!smb_io_enum_hnd("enum_hnd", &r_n->enum_hnd, ps, depth))
		return False;
	if(!prs_werror("status", ps, depth, &r_n->status))
		return False;

	return True;
}

/*******************************************************************
 Inits a SRV_Q_NET_SESS_DEL structure.
********************************************************************/

void init_srv_q_net_sess_del(SRV_Q_NET_SESS_DEL *q_n, const char *srv_name,
			      const char *cli_name, const char *user_name)
{
	DEBUG(5,("init_q_net_sess_enum\n"));

	init_buf_unistr2(&q_n->uni_srv_name, &q_n->ptr_srv_name, srv_name);
	init_buf_unistr2(&q_n->uni_cli_name, &q_n->ptr_cli_name, cli_name);
	init_buf_unistr2(&q_n->uni_user_name, &q_n->ptr_user_name, user_name);
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_q_net_sess_del(const char *desc, SRV_Q_NET_SESS_DEL *q_n, prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_q_net_sess_del");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name", ps, depth, &q_n->ptr_srv_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_srv_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_cli_name", ps, depth, &q_n->ptr_cli_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_cli_name, q_n->ptr_cli_name, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;
	if(!prs_uint32("ptr_user_name", ps, depth, &q_n->ptr_user_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_user_name, q_n->ptr_user_name, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_r_net_sess_del(const char *desc, SRV_R_NET_SESS_DEL *r_n, prs_struct *ps, int depth)
{
	if (r_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_r_net_sess_del");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_werror("status", ps, depth, &r_n->status))
		return False;

	return True;
}

/*******************************************************************
 Inits a CONN_INFO_0 structure
********************************************************************/

void init_srv_conn_info0(CONN_INFO_0 *ss0, uint32 id)
{
	DEBUG(5,("init_srv_conn_info0\n"));

	ss0->id = id;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_conn_info0(const char *desc, CONN_INFO_0 *ss0, prs_struct *ps, int depth)
{
	if (ss0 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_conn_info0");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("id", ps, depth, &ss0->id))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_srv_conn_info_0(const char *desc, SRV_CONN_INFO_0 *ss0, prs_struct *ps, int depth)
{
	if (ss0 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_srv_conn_info_0");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("num_entries_read", ps, depth, &ss0->num_entries_read))
		return False;
	if(!prs_uint32("ptr_conn_info", ps, depth, &ss0->ptr_conn_info))
		return False;

	if (ss0->ptr_conn_info != 0) {
		int i;
		int num_entries = ss0->num_entries_read;

		if (num_entries > MAX_CONN_ENTRIES) {
			num_entries = MAX_CONN_ENTRIES; /* report this! */
		}

		if(!prs_uint32("num_entries_read2", ps, depth, &ss0->num_entries_read2))
			return False;

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_conn_info0("", &ss0->info_0[i], ps, depth))
				return False;
		}

		if(!prs_align(ps))
			return False;
	}

	return True;
}

/*******************************************************************
 Inits a CONN_INFO_1_STR structure
********************************************************************/

void init_srv_conn_info1_str(CONN_INFO_1_STR *ss1, const char *usr_name, const char *net_name)
{
	DEBUG(5,("init_srv_conn_info1_str\n"));

	init_unistr2(&ss1->uni_usr_name, usr_name, UNI_STR_TERMINATE);
	init_unistr2(&ss1->uni_net_name, net_name, UNI_STR_TERMINATE);
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_conn_info1_str(const char *desc, CONN_INFO_1_STR *ss1, prs_struct *ps, int depth)
{
	if (ss1 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_conn_info1_str");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_unistr2("", &ss1->uni_usr_name, True, ps, depth))
		return False;
	if(!smb_io_unistr2("", &ss1->uni_net_name, True, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Inits a CONN_INFO_1 structure
********************************************************************/

void init_srv_conn_info1(CONN_INFO_1 *ss1, 
				uint32 id, uint32 type,
				uint32 num_opens, uint32 num_users, uint32 open_time,
				const char *usr_name, const char *net_name)
{
	DEBUG(5,("init_srv_conn_info1: %s %s\n", usr_name, net_name));

	ss1->id        = id       ;
	ss1->type      = type     ;
	ss1->num_opens = num_opens ;
	ss1->num_users = num_users;
	ss1->open_time = open_time;

	ss1->ptr_usr_name = (usr_name != NULL) ? 1 : 0;
	ss1->ptr_net_name = (net_name != NULL) ? 1 : 0;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_conn_info1(const char *desc, CONN_INFO_1 *ss1, prs_struct *ps, int depth)
{
	if (ss1 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_conn_info1");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("id          ", ps, depth, &ss1->id))
		return False;
	if(!prs_uint32("type        ", ps, depth, &ss1->type))
		return False;
	if(!prs_uint32("num_opens   ", ps, depth, &ss1->num_opens))
		return False;
	if(!prs_uint32("num_users   ", ps, depth, &ss1->num_users))
		return False;
	if(!prs_uint32("open_time   ", ps, depth, &ss1->open_time))
		return False;

	if(!prs_uint32("ptr_usr_name", ps, depth, &ss1->ptr_usr_name))
		return False;
	if(!prs_uint32("ptr_net_name", ps, depth, &ss1->ptr_net_name))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_srv_conn_info_1(const char *desc, SRV_CONN_INFO_1 *ss1, prs_struct *ps, int depth)
{
	if (ss1 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_srv_conn_info_1");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("num_entries_read", ps, depth, &ss1->num_entries_read))
		return False;
	if(!prs_uint32("ptr_conn_info", ps, depth, &ss1->ptr_conn_info))
		return False;

	if (ss1->ptr_conn_info != 0) {
		int i;
		int num_entries = ss1->num_entries_read;

		if (num_entries > MAX_CONN_ENTRIES) {
			num_entries = MAX_CONN_ENTRIES; /* report this! */
		}

		if(!prs_uint32("num_entries_read2", ps, depth, &ss1->num_entries_read2))
			return False;

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_conn_info1("", &ss1->info_1[i], ps, depth))
				return False;
		}

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_conn_info1_str("", &ss1->info_1_str[i], ps, depth))
				return False;
		}

		if(!prs_align(ps))
			return False;
	}

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_srv_conn_ctr(const char *desc, SRV_CONN_INFO_CTR **pp_ctr, prs_struct *ps, int depth)
{
	SRV_CONN_INFO_CTR *ctr = *pp_ctr;

	prs_debug(ps, depth, desc, "srv_io_srv_conn_ctr");
	depth++;

	if (UNMARSHALLING(ps)) {
		ctr = *pp_ctr = PRS_ALLOC_MEM(ps, SRV_CONN_INFO_CTR, 1);
		if (ctr == NULL)
			return False;
	}
		
	if (ctr == NULL)
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("switch_value", ps, depth, &ctr->switch_value))
		return False;
	if(!prs_uint32("ptr_conn_ctr", ps, depth, &ctr->ptr_conn_ctr))
		return False;

	if (ctr->ptr_conn_ctr != 0) {
		switch (ctr->switch_value) {
		case 0:
			if(!srv_io_srv_conn_info_0("", &ctr->conn.info0, ps, depth))
				return False;
			break;
		case 1:
			if(!srv_io_srv_conn_info_1("", &ctr->conn.info1, ps, depth))
				return False;
			break;
		default:
			DEBUG(5,("%s no connection info at switch_value %d\n",
			         tab_depth(depth), ctr->switch_value));
			break;
		}
	}

	return True;
}

/*******************************************************************
  Reads or writes a structure.
********************************************************************/

void init_srv_q_net_conn_enum(SRV_Q_NET_CONN_ENUM *q_n, 
				const char *srv_name, const char *qual_name,
				uint32 conn_level, SRV_CONN_INFO_CTR *ctr,
				uint32 preferred_len,
				ENUM_HND *hnd)
{
	DEBUG(5,("init_q_net_conn_enum\n"));

	q_n->ctr = ctr;

	init_buf_unistr2(&q_n->uni_srv_name, &q_n->ptr_srv_name, srv_name );
	init_buf_unistr2(&q_n->uni_qual_name, &q_n->ptr_qual_name, qual_name);

	q_n->conn_level    = conn_level;
	q_n->preferred_len = preferred_len;

	memcpy(&q_n->enum_hnd, hnd, sizeof(*hnd));
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_q_net_conn_enum(const char *desc, SRV_Q_NET_CONN_ENUM *q_n, prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_q_net_conn_enum");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name ", ps, depth, &q_n->ptr_srv_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_srv_name, q_n->ptr_srv_name, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_qual_name", ps, depth, &q_n->ptr_qual_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_qual_name, q_n->ptr_qual_name, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("conn_level", ps, depth, &q_n->conn_level))
		return False;
	
	if (q_n->conn_level != (uint32)-1) {
		if(!srv_io_srv_conn_ctr("conn_ctr", &q_n->ctr, ps, depth))
			return False;
	}

	if(!prs_uint32("preferred_len", ps, depth, &q_n->preferred_len))
		return False;

	if(!smb_io_enum_hnd("enum_hnd", &q_n->enum_hnd, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_r_net_conn_enum(const char *desc,  SRV_R_NET_CONN_ENUM *r_n, prs_struct *ps, int depth)
{
	if (r_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_r_net_conn_enum");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("conn_level", ps, depth, &r_n->conn_level))
		return False;

	if (r_n->conn_level != (uint32)-1) {
		if(!srv_io_srv_conn_ctr("conn_ctr", &r_n->ctr, ps, depth))
			return False;
	}

	if(!prs_uint32("total_entries", ps, depth, &r_n->total_entries))
		return False;
	if(!smb_io_enum_hnd("enum_hnd", &r_n->enum_hnd, ps, depth))
		return False;
	if(!prs_werror("status", ps, depth, &r_n->status))
		return False;

	return True;
}

/*******************************************************************
 Inits a FILE_INFO_3_STR structure
********************************************************************/

void init_srv_file_info3_str(FILE_INFO_3_STR *fi3, const char *user_name, const char *path_name)
{
	DEBUG(5,("init_srv_file_info3_str\n"));

	init_unistr2(&fi3->uni_path_name, path_name, UNI_STR_TERMINATE);
	init_unistr2(&fi3->uni_user_name, user_name, UNI_STR_TERMINATE);
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_file_info3_str(const char *desc, FILE_INFO_3_STR *sh1, prs_struct *ps, int depth)
{
	if (sh1 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_file_info3_str");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_unistr2("", &sh1->uni_path_name, True, ps, depth))
		return False;
	if(!smb_io_unistr2("", &sh1->uni_user_name, True, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Inits a FILE_INFO_3 structure
********************************************************************/

void init_srv_file_info3(FILE_INFO_3 *fl3,
			 uint32 id, uint32 perms, uint32 num_locks,
			 const char *path_name, const char *user_name)
{
	DEBUG(5,("init_srv_file_info3: %s %s\n", path_name, user_name));

	fl3->id        = id;	
	fl3->perms     = perms;
	fl3->num_locks = num_locks;

	fl3->ptr_path_name = (path_name != NULL) ? 1 : 0;
	fl3->ptr_user_name = (user_name != NULL) ? 1 : 0;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_file_info3(const char *desc, FILE_INFO_3 *fl3, prs_struct *ps, int depth)
{
	if (fl3 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_file_info3");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("id           ", ps, depth, &fl3->id))
		return False;
	if(!prs_uint32("perms        ", ps, depth, &fl3->perms))
		return False;
	if(!prs_uint32("num_locks    ", ps, depth, &fl3->num_locks))
		return False;
	if(!prs_uint32("ptr_path_name", ps, depth, &fl3->ptr_path_name))
		return False;
	if(!prs_uint32("ptr_user_name", ps, depth, &fl3->ptr_user_name))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

static BOOL srv_io_srv_file_ctr(const char *desc, SRV_FILE_INFO_CTR *ctr, prs_struct *ps, int depth)
{
	if (ctr == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_srv_file_ctr");
	depth++;

	if (UNMARSHALLING(ps)) {
		memset(ctr, '\0', sizeof(SRV_FILE_INFO_CTR));
	}

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("switch_value", ps, depth, &ctr->switch_value))
		return False;
	if (ctr->switch_value != 3) {
		DEBUG(5,("%s File info %d level not supported\n",
			 tab_depth(depth), ctr->switch_value));
	}
	if(!prs_uint32("ptr_file_info", ps, depth, &ctr->ptr_file_info))
		return False;
	if(!prs_uint32("num_entries", ps, depth, &ctr->num_entries))
		return False;
	if(!prs_uint32("ptr_entries", ps, depth, &ctr->ptr_entries))
		return False;
	if (ctr->ptr_entries == 0)
		return True;
	if(!prs_uint32("num_entries2", ps, depth, 
		       &ctr->num_entries2))
		return False;

	switch (ctr->switch_value) {
	case 3: {
		SRV_FILE_INFO_3 *info3 = ctr->file.info3;
		int num_entries = ctr->num_entries;
		int i;

		if (UNMARSHALLING(ps)) {
			if (!(info3 = PRS_ALLOC_MEM(ps, SRV_FILE_INFO_3, num_entries)))
				return False;
			ctr->file.info3 = info3;
		}

		for (i = 0; i < num_entries; i++) {
			if(!srv_io_file_info3("", &ctr->file.info3[i].info_3, ps, depth))
				return False;
		}
		for (i = 0; i < num_entries; i++) {
			if(!srv_io_file_info3_str("", &ctr->file.info3[i].info_3_str, ps, depth))
				return False;
		}
		break;
	}
	default:
		DEBUG(5,("%s no file info at switch_value %d\n",
			 tab_depth(depth), ctr->switch_value));
		break;
	}
			
	return True;
}

/*******************************************************************
 Inits a SRV_Q_NET_FILE_ENUM structure.
********************************************************************/

void init_srv_q_net_file_enum(SRV_Q_NET_FILE_ENUM *q_n, 
			      const char *srv_name, const char *qual_name, 
			      const char *user_name,
			      uint32 file_level, SRV_FILE_INFO_CTR *ctr,
			      uint32 preferred_len,
			      ENUM_HND *hnd)
{
	DEBUG(5,("init_q_net_file_enum\n"));

	init_buf_unistr2(&q_n->uni_srv_name, &q_n->ptr_srv_name, srv_name);
	init_buf_unistr2(&q_n->uni_qual_name, &q_n->ptr_qual_name, qual_name);
	init_buf_unistr2(&q_n->uni_user_name, &q_n->ptr_user_name, user_name);

	q_n->file_level    = q_n->ctr.switch_value = file_level;
	q_n->preferred_len = preferred_len;
	q_n->ctr.ptr_file_info = 1;
	q_n->ctr.num_entries = 0;
	q_n->ctr.num_entries2 = 0;

	memcpy(&q_n->enum_hnd, hnd, sizeof(*hnd));
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_q_net_file_enum(const char *desc, SRV_Q_NET_FILE_ENUM *q_n, prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_q_net_file_enum");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name", ps, depth, &q_n->ptr_srv_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_srv_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_qual_name", ps, depth, &q_n->ptr_qual_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_qual_name, q_n->ptr_qual_name, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_user_name", ps, depth, &q_n->ptr_user_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_user_name, q_n->ptr_user_name, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;
	if(!prs_uint32("file_level", ps, depth, &q_n->file_level))
		return False;

	if (q_n->file_level != (uint32)-1) {
		if(!srv_io_srv_file_ctr("file_ctr", &q_n->ctr, ps, depth))
			return False;
	}

	if(!prs_uint32("preferred_len", ps, depth, &q_n->preferred_len))
		return False;

	if(!smb_io_enum_hnd("enum_hnd", &q_n->enum_hnd, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_r_net_file_enum(const char *desc, SRV_R_NET_FILE_ENUM *r_n, prs_struct *ps, int depth)
{
	if (r_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_r_net_file_enum");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("file_level", ps, depth, &r_n->file_level))
		return False;

	if (r_n->file_level != 0) {
		if(!srv_io_srv_file_ctr("file_ctr", &r_n->ctr, ps, depth))
			return False;
	}

	if(!prs_uint32("total_entries", ps, depth, &r_n->total_entries))
		return False;
	if(!smb_io_enum_hnd("enum_hnd", &r_n->enum_hnd, ps, depth))
		return False;
	if(!prs_werror("status", ps, depth, &r_n->status))
		return False;

	return True;
}

/*******************************************************************
 Initialize a net file close request
********************************************************************/
void init_srv_q_net_file_close(SRV_Q_NET_FILE_CLOSE *q_n, const char *server,
			       uint32 file_id)
{
	q_n->ptr_srv_name = 1;
	init_unistr2(&q_n->uni_srv_name, server, UNI_STR_TERMINATE);
	q_n->file_id = file_id;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/
BOOL srv_io_q_net_file_close(const char *desc, SRV_Q_NET_FILE_CLOSE *q_n,
			     prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_q_net_file_close");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name", ps, depth, &q_n->ptr_srv_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_srv_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("file_id", ps, depth, &q_n->file_id))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_r_net_file_close(const char *desc, SRV_R_NET_FILE_CLOSE *q_n, 
			     prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_r_net_file_close");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_werror("status", ps, depth, &q_n->status))
		return False;

	return True;
}	

/*******************************************************************
 Inits a SRV_INFO_100 structure.
 ********************************************************************/

void init_srv_info_100(SRV_INFO_100 *sv100, uint32 platform_id, const char *name)
{
	DEBUG(5,("init_srv_info_100\n"));

	sv100->platform_id  = platform_id;
	init_buf_unistr2(&sv100->uni_name, &sv100->ptr_name, name);
}

/*******************************************************************
 Reads or writes a SRV_INFO_101 structure.
 ********************************************************************/

static BOOL srv_io_info_100(const char *desc, SRV_INFO_100 *sv100, prs_struct *ps, int depth)
{
	if (sv100 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_info_100");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("platform_id ", ps, depth, &sv100->platform_id))
		return False;
	if(!prs_uint32("ptr_name    ", ps, depth, &sv100->ptr_name))
		return False;

	if(!smb_io_unistr2("uni_name    ", &sv100->uni_name, True, ps, depth))
		return False;

	return True;
}


/*******************************************************************
 Inits a SRV_INFO_101 structure.
 ********************************************************************/

void init_srv_info_101(SRV_INFO_101 *sv101, uint32 platform_id, const char *name,
				uint32 ver_major, uint32 ver_minor,
				uint32 srv_type, const char *comment)
{
	DEBUG(5,("init_srv_info_101\n"));

	sv101->platform_id  = platform_id;
	init_buf_unistr2(&sv101->uni_name, &sv101->ptr_name, name);
	sv101->ver_major    = ver_major;
	sv101->ver_minor    = ver_minor;
	sv101->srv_type     = srv_type;
	init_buf_unistr2(&sv101->uni_comment, &sv101->ptr_comment, comment);
}

/*******************************************************************
 Reads or writes a SRV_INFO_101 structure.
 ********************************************************************/

static BOOL srv_io_info_101(const char *desc, SRV_INFO_101 *sv101, prs_struct *ps, int depth)
{
	if (sv101 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_info_101");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("platform_id ", ps, depth, &sv101->platform_id))
		return False;
	if(!prs_uint32("ptr_name    ", ps, depth, &sv101->ptr_name))
		return False;
	if(!prs_uint32("ver_major   ", ps, depth, &sv101->ver_major))
		return False;
	if(!prs_uint32("ver_minor   ", ps, depth, &sv101->ver_minor))
		return False;
	if(!prs_uint32("srv_type    ", ps, depth, &sv101->srv_type))
		return False;
	if(!prs_uint32("ptr_comment ", ps, depth, &sv101->ptr_comment))
		return False;

	if(!prs_align(ps))
		return False;

	if(!smb_io_unistr2("uni_name    ", &sv101->uni_name, True, ps, depth))
		return False;
	if(!smb_io_unistr2("uni_comment ", &sv101->uni_comment, True, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Inits a SRV_INFO_102 structure.
 ********************************************************************/

void init_srv_info_102(SRV_INFO_102 *sv102, uint32 platform_id, const char *name,
				const char *comment, uint32 ver_major, uint32 ver_minor,
				uint32 srv_type, uint32 users, uint32 disc, uint32 hidden,
				uint32 announce, uint32 ann_delta, uint32 licenses,
				const char *usr_path)
{
	DEBUG(5,("init_srv_info_102\n"));

	sv102->platform_id  = platform_id;
	init_buf_unistr2(&sv102->uni_name, &sv102->ptr_name, name);
	sv102->ver_major    = ver_major;
	sv102->ver_minor    = ver_minor;
	sv102->srv_type     = srv_type;
	init_buf_unistr2(&sv102->uni_comment, &sv102->ptr_comment, comment);

	/* same as 101 up to here */

	sv102->users        = users;
	sv102->disc         = disc;
	sv102->hidden       = hidden;
	sv102->announce     = announce;
	sv102->ann_delta    = ann_delta;
	sv102->licenses     = licenses;
	init_buf_unistr2(&sv102->uni_usr_path, &sv102->ptr_usr_path, usr_path);
}


/*******************************************************************
 Reads or writes a SRV_INFO_102 structure.
 ********************************************************************/

static BOOL srv_io_info_102(const char *desc, SRV_INFO_102 *sv102, prs_struct *ps, int depth)
{
	if (sv102 == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_info102");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("platform_id ", ps, depth, &sv102->platform_id))
		return False;
	if(!prs_uint32("ptr_name    ", ps, depth, &sv102->ptr_name))
		return False;
	if(!prs_uint32("ver_major   ", ps, depth, &sv102->ver_major))
		return False;
	if(!prs_uint32("ver_minor   ", ps, depth, &sv102->ver_minor))
		return False;
	if(!prs_uint32("srv_type    ", ps, depth, &sv102->srv_type))
		return False;
	if(!prs_uint32("ptr_comment ", ps, depth, &sv102->ptr_comment))
		return False;

	/* same as 101 up to here */

	if(!prs_uint32("users       ", ps, depth, &sv102->users))
		return False;
	if(!prs_uint32("disc        ", ps, depth, &sv102->disc))
		return False;
	if(!prs_uint32("hidden      ", ps, depth, &sv102->hidden))
		return False;
	if(!prs_uint32("announce    ", ps, depth, &sv102->announce))
		return False;
	if(!prs_uint32("ann_delta   ", ps, depth, &sv102->ann_delta))
		return False;
	if(!prs_uint32("licenses    ", ps, depth, &sv102->licenses))
		return False;
	if(!prs_uint32("ptr_usr_path", ps, depth, &sv102->ptr_usr_path))
		return False;

	if(!smb_io_unistr2("uni_name    ", &sv102->uni_name, True, ps, depth))
		return False;
	if(!prs_align(ps))
		return False;
	if(!smb_io_unistr2("uni_comment ", &sv102->uni_comment, True, ps, depth))
		return False;
	if(!prs_align(ps))
		return False;
	if(!smb_io_unistr2("uni_usr_path", &sv102->uni_usr_path, True, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a SRV_INFO_102 structure.
 ********************************************************************/

static BOOL srv_io_info_ctr(const char *desc, SRV_INFO_CTR *ctr, prs_struct *ps, int depth)
{
	if (ctr == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_info_ctr");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("switch_value", ps, depth, &ctr->switch_value))
		return False;
	if(!prs_uint32("ptr_srv_ctr ", ps, depth, &ctr->ptr_srv_ctr))
		return False;

	if (ctr->ptr_srv_ctr != 0 && ctr->switch_value != 0 && ctr != NULL) {
		switch (ctr->switch_value) {
		case 100:
			if(!srv_io_info_100("sv100", &ctr->srv.sv100, ps, depth))
				return False;
			break;
		case 101:
			if(!srv_io_info_101("sv101", &ctr->srv.sv101, ps, depth))
				return False;
			break;
		case 102:
			if(!srv_io_info_102("sv102", &ctr->srv.sv102, ps, depth))
				return False;
			break;
		default:
			DEBUG(5,("%s no server info at switch_value %d\n",
					 tab_depth(depth), ctr->switch_value));
			break;
		}
		if(!prs_align(ps))
			return False;
	}

	return True;
}

/*******************************************************************
 Inits a SRV_Q_NET_SRV_GET_INFO structure.
 ********************************************************************/

void init_srv_q_net_srv_get_info(SRV_Q_NET_SRV_GET_INFO *srv,
				const char *server_name, uint32 switch_value)
{
	DEBUG(5,("init_srv_q_net_srv_get_info\n"));

	init_buf_unistr2(&srv->uni_srv_name, &srv->ptr_srv_name, server_name);

	srv->switch_value = switch_value;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_q_net_srv_get_info(const char *desc, SRV_Q_NET_SRV_GET_INFO *q_n, prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_q_net_srv_get_info");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name  ", ps, depth, &q_n->ptr_srv_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_srv_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("switch_value  ", ps, depth, &q_n->switch_value))
		return False;

	return True;
}

/*******************************************************************
 Inits a SRV_R_NET_SRV_GET_INFO structure.
 ********************************************************************/

void init_srv_r_net_srv_get_info(SRV_R_NET_SRV_GET_INFO *srv,
				uint32 switch_value, SRV_INFO_CTR *ctr, WERROR status)
{
	DEBUG(5,("init_srv_r_net_srv_get_info\n"));

	srv->ctr = ctr;

	if (W_ERROR_IS_OK(status)) {
		srv->ctr->switch_value = switch_value;
		srv->ctr->ptr_srv_ctr  = 1;
	} else {
		srv->ctr->switch_value = 0;
		srv->ctr->ptr_srv_ctr  = 0;
	}

	srv->status = status;
}

/*******************************************************************
 Inits a SRV_R_NET_SRV_SET_INFO structure.
 ********************************************************************/

void init_srv_r_net_srv_set_info(SRV_R_NET_SRV_SET_INFO *srv,
				 uint32 switch_value, WERROR status)
{
	DEBUG(5,("init_srv_r_net_srv_set_info\n"));

	srv->switch_value = switch_value;
	srv->status = status;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_q_net_srv_set_info(const char *desc, SRV_Q_NET_SRV_SET_INFO *q_n, 
			       prs_struct *ps, int depth)
{
	prs_debug(ps, depth, desc, "srv_io_q_net_srv_set_info");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name  ", ps, depth, &q_n->ptr_srv_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_srv_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("switch_value  ", ps, depth, &q_n->switch_value))
		return False;

	if (UNMARSHALLING(ps)) {
		q_n->ctr = PRS_ALLOC_MEM(ps, SRV_INFO_CTR, 1);

		if (!q_n->ctr)
			return False;
	}

	if(!srv_io_info_ctr("ctr", q_n->ctr, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
 ********************************************************************/

BOOL srv_io_r_net_srv_get_info(const char *desc, SRV_R_NET_SRV_GET_INFO *r_n, prs_struct *ps, int depth)
{
	if (r_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_r_net_srv_get_info");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!srv_io_info_ctr("ctr", r_n->ctr, ps, depth))
		return False;

	if(!prs_werror("status", ps, depth, &r_n->status))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
 ********************************************************************/

BOOL srv_io_r_net_srv_set_info(const char *desc, SRV_R_NET_SRV_SET_INFO *r_n, 
			       prs_struct *ps, int depth)
{
	prs_debug(ps, depth, desc, "srv_io_r_net_srv_set_info");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("switch value ", ps, depth, &r_n->switch_value))
		return False;

	if(!prs_werror("status", ps, depth, &r_n->status))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
 ********************************************************************/

BOOL srv_io_q_net_remote_tod(const char *desc, SRV_Q_NET_REMOTE_TOD *q_n, prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_q_net_remote_tod");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name  ", ps, depth, &q_n->ptr_srv_name))
		return False;
	if(!smb_io_unistr2("", &q_n->uni_srv_name, True, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a TIME_OF_DAY_INFO structure.
 ********************************************************************/

static BOOL srv_io_time_of_day_info(const char *desc, TIME_OF_DAY_INFO *tod, prs_struct *ps, int depth)
{
	if (tod == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_time_of_day_info");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint32("elapsedt   ", ps, depth, &tod->elapsedt))
		return False;
	if(!prs_uint32("msecs      ", ps, depth, &tod->msecs))
		return False;
	if(!prs_uint32("hours      ", ps, depth, &tod->hours))
		return False;
	if(!prs_uint32("mins       ", ps, depth, &tod->mins))
		return False;
	if(!prs_uint32("secs       ", ps, depth, &tod->secs))
		return False;
	if(!prs_uint32("hunds      ", ps, depth, &tod->hunds))
		return False;
	if(!prs_uint32("timezone   ", ps, depth, &tod->zone))
		return False;
	if(!prs_uint32("tintervals ", ps, depth, &tod->tintervals))
		return False;
	if(!prs_uint32("day        ", ps, depth, &tod->day))
		return False;
	if(!prs_uint32("month      ", ps, depth, &tod->month))
		return False;
	if(!prs_uint32("year       ", ps, depth, &tod->year))
		return False;
	if(!prs_uint32("weekday    ", ps, depth, &tod->weekday))
		return False;

	return True;
}

/*******************************************************************
 Inits a TIME_OF_DAY_INFO structure.
 ********************************************************************/

void init_time_of_day_info(TIME_OF_DAY_INFO *tod, uint32 elapsedt, uint32 msecs,
                           uint32 hours, uint32 mins, uint32 secs, uint32 hunds,
			   uint32 zone, uint32 tintervals, uint32 day,
			   uint32 month, uint32 year, uint32 weekday)
{
	DEBUG(5,("init_time_of_day_info\n"));

	tod->elapsedt	= elapsedt;
	tod->msecs	= msecs;
	tod->hours	= hours;
	tod->mins	= mins;
	tod->secs	= secs;
	tod->hunds	= hunds;
	tod->zone	= zone;
	tod->tintervals	= tintervals;
	tod->day	= day;
	tod->month	= month;
	tod->year	= year;
	tod->weekday	= weekday;
}


/*******************************************************************
 Reads or writes a structure.
 ********************************************************************/

BOOL srv_io_r_net_remote_tod(const char *desc, SRV_R_NET_REMOTE_TOD *r_n, prs_struct *ps, int depth)
{
	if (r_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_r_net_remote_tod");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint32("ptr_srv_tod ", ps, depth, &r_n->ptr_srv_tod))
		return False;

	if(!srv_io_time_of_day_info("tod", r_n->tod, ps, depth))
		return False;

	if(!prs_werror("status", ps, depth, &r_n->status))
		return False;

	return True;
}

/*******************************************************************
 initialises a structure.
 ********************************************************************/

BOOL init_srv_q_net_disk_enum(SRV_Q_NET_DISK_ENUM *q_n,
			      const char *srv_name,
			      uint32 preferred_len,
			      ENUM_HND *enum_hnd
	) 
{
  

	DEBUG(5,("init_srv_q_net_srv_disk_enum\n"));

	init_buf_unistr2(&q_n->uni_srv_name, &q_n->ptr_srv_name, srv_name);

	q_n->disk_enum_ctr.level = 0;
	q_n->disk_enum_ctr.disk_info_ptr   = 0;
  
	q_n->preferred_len = preferred_len;
	memcpy(&q_n->enum_hnd, enum_hnd, sizeof(*enum_hnd));

	return True;
}

/*******************************************************************
 Reads or writes a structure.
 ********************************************************************/

BOOL srv_io_q_net_disk_enum(const char *desc, SRV_Q_NET_DISK_ENUM *q_n, prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_q_net_disk_enum");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name", ps, depth, &q_n->ptr_srv_name))
		return False;

	if(!smb_io_unistr2("", &q_n->uni_srv_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("level", ps, depth, &q_n->disk_enum_ctr.level))
		return False;

	if(!prs_uint32("entries_read", ps, depth, &q_n->disk_enum_ctr.entries_read))
		return False;

	if(!prs_uint32("buffer", ps, depth, &q_n->disk_enum_ctr.disk_info_ptr))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("preferred_len", ps, depth, &q_n->preferred_len))
		return False;
	if(!smb_io_enum_hnd("enum_hnd", &q_n->enum_hnd, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
 ********************************************************************/

BOOL srv_io_r_net_disk_enum(const char *desc, SRV_R_NET_DISK_ENUM *r_n, prs_struct *ps, int depth)
{

	unsigned int i;
	uint32 entries_read, entries_read2, entries_read3;

	if (r_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_r_net_disk_enum");
	depth++;

	entries_read = entries_read2 = entries_read3 = r_n->disk_enum_ctr.entries_read;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("entries_read", ps, depth, &entries_read))
		return False;
	if(!prs_uint32("ptr_disk_info", ps, depth, &r_n->disk_enum_ctr.disk_info_ptr))
		return False;

	/*this may be max, unknown, actual?*/

	if(!prs_uint32("max_elements", ps, depth, &entries_read2))
		return False;
	if(!prs_uint32("unknown", ps, depth, &r_n->disk_enum_ctr.unknown))
		return False;
	if(!prs_uint32("actual_elements", ps, depth, &entries_read3))
		return False;

	r_n->disk_enum_ctr.entries_read = entries_read3;

	if(UNMARSHALLING(ps)) {

		DISK_INFO *dinfo;

		if(!(dinfo = PRS_ALLOC_MEM(ps, DISK_INFO, entries_read3)))
			return False;
		r_n->disk_enum_ctr.disk_info = dinfo;
	}

	for(i=0; i < r_n->disk_enum_ctr.entries_read; i++) {

		if(!prs_uint32("unknown", ps, depth, &r_n->disk_enum_ctr.disk_info[i].unknown))
			return False;
   
		if(!smb_io_unistr3("disk_name", &r_n->disk_enum_ctr.disk_info[i].disk_name, ps, depth))
			return False;

		if(!prs_align(ps))
			return False;
	}

	if(!prs_uint32("total_entries", ps, depth, &r_n->total_entries))
		return False;

	if(!smb_io_enum_hnd("enum_hnd", &r_n->enum_hnd, ps, depth))
		return False;

	if(!prs_werror("status", ps, depth, &r_n->status))
		return False;

	return True;
}

/*******************************************************************
 initialises a structure.
 ********************************************************************/

BOOL init_srv_q_net_name_validate(SRV_Q_NET_NAME_VALIDATE *q_n, const char *srv_name, const char *share_name, int type) 
{
	uint32 ptr_share_name;

	DEBUG(5,("init_srv_q_net_name_validate\n"));
  
	init_buf_unistr2(&q_n->uni_srv_name, &q_n->ptr_srv_name, srv_name);
	init_buf_unistr2(&q_n->uni_name,     &ptr_share_name,    share_name);

	q_n->type  = type;
	q_n->flags = 0;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
 ********************************************************************/

BOOL srv_io_q_net_name_validate(const char *desc, SRV_Q_NET_NAME_VALIDATE *q_n, prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_q_net_name_validate");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name", ps, depth, &q_n->ptr_srv_name))
		return False;

	if(!smb_io_unistr2("", &q_n->uni_srv_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!smb_io_unistr2("", &q_n->uni_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("type", ps, depth, &q_n->type))
		return False;

	if(!prs_uint32("flags", ps, depth, &q_n->flags))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
 ********************************************************************/

BOOL srv_io_r_net_name_validate(const char *desc, SRV_R_NET_NAME_VALIDATE *r_n, prs_struct *ps, int depth)
{
	if (r_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_r_net_name_validate");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_werror("status", ps, depth, &r_n->status))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_q_net_file_query_secdesc(const char *desc, SRV_Q_NET_FILE_QUERY_SECDESC *q_n, prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_q_net_file_query_secdesc");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name", ps, depth, &q_n->ptr_srv_name))
		return False;

	if(!smb_io_unistr2("", &q_n->uni_srv_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_qual_name", ps, depth, &q_n->ptr_qual_name))
		return False;

	if(!smb_io_unistr2("", &q_n->uni_qual_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!smb_io_unistr2("", &q_n->uni_file_name, True, ps, depth))
		return False;

	if(!prs_uint32("unknown1", ps, depth, &q_n->unknown1))
		return False;

	if(!prs_uint32("unknown2", ps, depth, &q_n->unknown2))
		return False;

	if(!prs_uint32("unknown3", ps, depth, &q_n->unknown3))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_r_net_file_query_secdesc(const char *desc, SRV_R_NET_FILE_QUERY_SECDESC *r_n, prs_struct *ps, int depth)
{
	if (r_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_r_net_file_query_secdesc");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_response", ps, depth, &r_n->ptr_response))
		return False;

	if(!prs_uint32("size_response", ps, depth, &r_n->size_response))
		return False;

	if(!prs_uint32("ptr_secdesc", ps, depth, &r_n->ptr_secdesc))
		return False;

	if(!prs_uint32("size_secdesc", ps, depth, &r_n->size_secdesc))
		return False;

	if(!sec_io_desc("sec_desc", &r_n->sec_desc, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_werror("status", ps, depth, &r_n->status))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_q_net_file_set_secdesc(const char *desc, SRV_Q_NET_FILE_SET_SECDESC *q_n, prs_struct *ps, int depth)
{
	if (q_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_q_net_file_set_secdesc");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name", ps, depth, &q_n->ptr_srv_name))
		return False;

	if(!smb_io_unistr2("", &q_n->uni_srv_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_qual_name", ps, depth, &q_n->ptr_qual_name))
		return False;

	if(!smb_io_unistr2("", &q_n->uni_qual_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!smb_io_unistr2("", &q_n->uni_file_name, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("sec_info", ps, depth, &q_n->sec_info))
		return False;

	if(!prs_uint32("size_set", ps, depth, &q_n->size_set))
		return False;

	if(!prs_uint32("ptr_secdesc", ps, depth, &q_n->ptr_secdesc))
		return False;

	if(!prs_uint32("size_secdesc", ps, depth, &q_n->size_secdesc))
		return False;

	if(!sec_io_desc("sec_desc", &q_n->sec_desc, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a structure.
********************************************************************/

BOOL srv_io_r_net_file_set_secdesc(const char *desc, SRV_R_NET_FILE_SET_SECDESC *r_n, prs_struct *ps, int depth)
{
	if (r_n == NULL)
		return False;

	prs_debug(ps, depth, desc, "srv_io_r_net_file_set_secdesc");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_werror("status", ps, depth, &r_n->status))
		return False;

	return True;
}

/*******************************************************************
 Inits a structure
********************************************************************/

void init_srv_q_net_remote_tod(SRV_Q_NET_REMOTE_TOD *q_u, const char *server)
{
	q_u->ptr_srv_name = 1;
	init_unistr2(&q_u->uni_srv_name, server, UNI_STR_TERMINATE);
}
