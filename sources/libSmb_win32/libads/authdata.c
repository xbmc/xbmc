/* 
   Unix SMB/CIFS implementation.
   kerberos authorization data (PAC) utility library
   Copyright (C) Jim McDonough <jmcd@us.ibm.com> 2003   
   Copyright (C) Andrew Bartlett <abartlet@samba.org> 2004-2005
   Copyright (C) Andrew Tridgell 2001
   Copyright (C) Luke Howard 2002-2003
   Copyright (C) Stefan Metzmacher 2004-2005
   Copyright (C) Guenther Deschner 2005
   
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

#ifdef HAVE_KRB5

static BOOL pac_io_logon_name(const char *desc, PAC_LOGON_NAME *logon_name,
			      prs_struct *ps, int depth)
{
	if (NULL == logon_name)
		return False;

	prs_debug(ps, depth, desc, "pac_io_logon_name");
	depth++;

	if (!smb_io_time("logon_time", &logon_name->logon_time, ps, depth))
		return False;

	if (!prs_uint16("len", ps, depth, &logon_name->len))
		return False;

	/* The following string is always in little endian 16 bit values,
	   copy as 8 bits to avoid endian reversal on big-endian machines.
	   len is the length in bytes. */

	if (UNMARSHALLING(ps) && logon_name->len) {
		logon_name->username = PRS_ALLOC_MEM(ps, uint8, logon_name->len);
		if (!logon_name->username) {
			DEBUG(3, ("No memory available\n"));
			return False;
		}
	}

	if (!prs_uint8s(True, "name", ps, depth, logon_name->username, logon_name->len))
		return False;

	return True;
}

#if 0 /* Unused (handled now in net_io_user_info3()) - Guenther */
static BOOL pac_io_krb_sids(const char *desc, KRB_SID_AND_ATTRS *sid_and_attr,
			    prs_struct *ps, int depth)
{
	if (NULL == sid_and_attr)
		return False;

	prs_debug(ps, depth, desc, "pac_io_krb_sids");
	depth++;

	if (UNMARSHALLING(ps)) {
		sid_and_attr->sid = PRS_ALLOC_MEM(ps, DOM_SID2, 1);
		if (!sid_and_attr->sid) {
			DEBUG(3, ("No memory available\n"));
			return False;
		}
	}

	if(!smb_io_dom_sid2("sid", sid_and_attr->sid, ps, depth))
		return False;

	return True;
}


static BOOL pac_io_krb_attrs(const char *desc, KRB_SID_AND_ATTRS *sid_and_attr,
			     prs_struct *ps, int depth)
{
	if (NULL == sid_and_attr)
		return False;

	prs_debug(ps, depth, desc, "pac_io_krb_attrs");
	depth++;

	if (!prs_uint32("sid_ptr", ps, depth, &sid_and_attr->sid_ptr))
		return False;
	if (!prs_uint32("attrs", ps, depth, &sid_and_attr->attrs))
		return False;

	return True;
}

static BOOL pac_io_krb_sid_and_attr_array(const char *desc, 
					  KRB_SID_AND_ATTR_ARRAY *array,
					  uint32 num,
					  prs_struct *ps, int depth)
{
	int i;

	if (NULL == array)
		return False;

	prs_debug(ps, depth, desc, "pac_io_krb_sid_and_attr_array");
	depth++;


	if (!prs_uint32("count", ps, depth, &array->count))
		return False;

	if (UNMARSHALLING(ps)) {
		array->krb_sid_and_attrs = PRS_ALLOC_MEM(ps, KRB_SID_AND_ATTRS, num);
		if (!array->krb_sid_and_attrs) {
			DEBUG(3, ("No memory available\n"));
			return False;
		}
	}

	for (i=0; i<num; i++) {
		if (!pac_io_krb_attrs(desc, 
				      &array->krb_sid_and_attrs[i],
				      ps, depth))
			return False;

	}
	for (i=0; i<num; i++) {
		if (!pac_io_krb_sids(desc, 
				     &array->krb_sid_and_attrs[i],
				     ps, depth))
			return False;

	}

	return True;

}
#endif

static BOOL pac_io_group_membership(const char *desc, 
				    GROUP_MEMBERSHIP *membership,
				    prs_struct *ps, int depth)
{
	if (NULL == membership)
		return False;

	prs_debug(ps, depth, desc, "pac_io_group_membership");
	depth++;

	if (!prs_uint32("rid", ps, depth, &membership->rid))
		return False;
	if (!prs_uint32("attrs", ps, depth, &membership->attrs))
		return False;

	return True;
}


static BOOL pac_io_group_membership_array(const char *desc, 
					  GROUP_MEMBERSHIP_ARRAY *array,
					  uint32 num,
					  prs_struct *ps, int depth)
{
	int i;

	if (NULL == array)
		return False;

	prs_debug(ps, depth, desc, "pac_io_group_membership_array");
	depth++;


	if (!prs_uint32("count", ps, depth, &array->count))
		return False;

	if (UNMARSHALLING(ps)) {
		array->group_membership = PRS_ALLOC_MEM(ps, GROUP_MEMBERSHIP, num);
		if (!array->group_membership) {
			DEBUG(3, ("No memory available\n"));
			return False;
		}
	}

	for (i=0; i<num; i++) {
		if (!pac_io_group_membership(desc, 
					     &array->group_membership[i],
					     ps, depth))
			return False;

	}

	return True;

}

#if 0 /* Unused, replaced using an expanded net_io_user_info3() now - Guenther */
static BOOL pac_io_pac_logon_info(const char *desc, PAC_LOGON_INFO *info, 
				  prs_struct *ps, int depth)
{
	uint32 garbage, i;

	if (NULL == info)
		return False;

	prs_debug(ps, depth, desc, "pac_io_pac_logon_info");
	depth++;

	if (!prs_align(ps))
		return False;
	if (!prs_uint32("unknown", ps, depth, &garbage)) /* 00081001 */
		return False;
	if (!prs_uint32("unknown", ps, depth, &garbage)) /* cccccccc */
		return False;
	if (!prs_uint32("bufferlen", ps, depth, &garbage))
		return False;
	if (!prs_uint32("bufferlenhi", ps, depth, &garbage)) /* 00000000 */
		return False;

	if (!prs_uint32("pointer", ps, depth, &garbage))
		return False;

	if (!prs_align(ps))
		return False;
	if (!smb_io_time("logon_time", &info->logon_time, ps, depth))
		return False;
	if (!smb_io_time("logoff_time", &info->logoff_time, ps, depth))
		return False;
	if (!smb_io_time("kickoff_time", &info->kickoff_time, ps, depth))
		return False;
	if (!smb_io_time("pass_last_set_time", &info->pass_last_set_time, 
			 ps, depth))
		return False;
	if (!smb_io_time("pass_can_change_time", &info->pass_can_change_time, 
			 ps, depth))
		return False;
	if (!smb_io_time("pass_must_change_time", &info->pass_must_change_time,
			 ps, depth))
		return False;

	if (!smb_io_unihdr("hdr_user_name", &info->hdr_user_name, ps, depth))
		return False;
	if (!smb_io_unihdr("hdr_full_name", &info->hdr_full_name, ps, depth))
		return False;
	if (!smb_io_unihdr("hdr_logon_script", &info->hdr_logon_script, 
			   ps, depth))
		return False;
	if (!smb_io_unihdr("hdr_profile_path", &info->hdr_profile_path, 
			   ps, depth))
		return False;
	if (!smb_io_unihdr("hdr_home_dir", &info->hdr_home_dir, ps, depth))
		return False;
	if (!smb_io_unihdr("hdr_dir_drive", &info->hdr_dir_drive, ps, depth))
		return False;

	if (!prs_uint16("logon_count", ps, depth, &info->logon_count))
		return False;
	if (!prs_uint16("bad_password_count", ps, depth, &info->bad_password_count))
		return False;
	if (!prs_uint32("user_rid", ps, depth, &info->user_rid))
		return False;
	if (!prs_uint32("group_rid", ps, depth, &info->group_rid))
		return False;
	if (!prs_uint32("group_count", ps, depth, &info->group_count))
		return False;
	/* I haven't seen this contain anything yet, but when it does
	   we will have to make sure we decode the contents in the middle
	   all the unistr2s ... */
	if (!prs_uint32("group_mem_ptr", ps, depth, 
			&info->group_membership_ptr))
		return False;
	if (!prs_uint32("user_flags", ps, depth, &info->user_flags))
		return False;

	if (!prs_uint8s(False, "session_key", ps, depth, info->session_key, 16)) 
		return False;
	
	if (!smb_io_unihdr("hdr_dom_controller", 
			   &info->hdr_dom_controller, ps, depth))
		return False;
	if (!smb_io_unihdr("hdr_dom_name", &info->hdr_dom_name, ps, depth))
		return False;

	/* this should be followed, but just get ptr for now */
	if (!prs_uint32("ptr_dom_sid", ps, depth, &info->ptr_dom_sid))
		return False;

	if (!prs_uint8s(False, "lm_session_key", ps, depth, info->lm_session_key, 8)) 
		return False;

	if (!prs_uint32("acct_flags", ps, depth, &info->acct_flags))
		return False;

	for (i = 0; i < 7; i++)
	{
		if (!prs_uint32("unkown", ps, depth, &info->unknown[i])) /* unknown */
                        return False;
	}

	if (!prs_uint32("sid_count", ps, depth, &info->sid_count))
		return False;
	if (!prs_uint32("ptr_extra_sids", ps, depth, &info->ptr_extra_sids))
		return False;
	if (!prs_uint32("ptr_res_group_dom_sid", ps, depth, 
			&info->ptr_res_group_dom_sid))
		return False;
	if (!prs_uint32("res_group_count", ps, depth, &info->res_group_count))
		return False;
	if (!prs_uint32("ptr_res_groups", ps, depth, &info->ptr_res_groups))
		return False;

	if(!smb_io_unistr2("uni_user_name", &info->uni_user_name, 
			   info->hdr_user_name.buffer, ps, depth))
		return False;
	if(!smb_io_unistr2("uni_full_name", &info->uni_full_name, 
			   info->hdr_full_name.buffer, ps, depth))
		return False;
	if(!smb_io_unistr2("uni_logon_script", &info->uni_logon_script, 
			   info->hdr_logon_script.buffer, ps, depth))
		return False;
	if(!smb_io_unistr2("uni_profile_path", &info->uni_profile_path,
			   info->hdr_profile_path.buffer, ps, depth))
		return False;
	if(!smb_io_unistr2("uni_home_dir", &info->uni_home_dir,
			   info->hdr_home_dir.buffer, ps, depth))
		return False;
	if(!smb_io_unistr2("uni_dir_drive", &info->uni_dir_drive,
			   info->hdr_dir_drive.buffer, ps, depth))
		return False;

	if (info->group_membership_ptr) {
		if (!pac_io_group_membership_array("group membership",
						   &info->groups,
						   info->group_count,
						   ps, depth))
			return False;
	}


	if(!smb_io_unistr2("uni_dom_controller", &info->uni_dom_controller,
			   info->hdr_dom_controller.buffer, ps, depth))
		return False;
	if(!smb_io_unistr2("uni_dom_name", &info->uni_dom_name, 
			   info->hdr_dom_name.buffer, ps, depth))
		return False;

	if(info->ptr_dom_sid)
		if(!smb_io_dom_sid2("dom_sid", &info->dom_sid, ps, depth))
			return False;

	
	if (info->sid_count && info->ptr_extra_sids)
		if (!pac_io_krb_sid_and_attr_array("extra_sids", 
						   &info->extra_sids,
						   info->sid_count,
						   ps, depth))
			return False;

	if (info->ptr_res_group_dom_sid)
		if (!smb_io_dom_sid2("res_group_dom_sid", 
				     &info->res_group_dom_sid, ps, depth))
			return False;

	if (info->ptr_res_groups) {

		if (!(info->user_flgs & LOGON_RESOURCE_GROUPS)) {
			DEBUG(0,("user_flgs attribute does not have LOGON_RESOURCE_GROUPS\n"));
			/* return False; */
		}

		if (!pac_io_group_membership_array("res group membership",
						   &info->res_groups,
						   info->res_group_count,
						   ps, depth))
			return False;
	}

	return True;
}
#endif

static BOOL pac_io_pac_logon_info(const char *desc, PAC_LOGON_INFO *info, 
				  prs_struct *ps, int depth)
{
	uint32 garbage;
	BOOL kerb_validation_info = True;

	if (NULL == info)
		return False;

	prs_debug(ps, depth, desc, "pac_io_pac_logon_info");
	depth++;

	if (!prs_align(ps))
		return False;
	if (!prs_uint32("unknown", ps, depth, &garbage)) /* 00081001 */
		return False;
	if (!prs_uint32("unknown", ps, depth, &garbage)) /* cccccccc */
		return False;
	if (!prs_uint32("bufferlen", ps, depth, &garbage))
		return False;
	if (!prs_uint32("bufferlenhi", ps, depth, &garbage)) /* 00000000 */
		return False;

	if(!net_io_user_info3("", &info->info3, ps, depth, 3, kerb_validation_info))
		return False;

	if (info->info3.ptr_res_group_dom_sid) {
		if (!smb_io_dom_sid2("res_group_dom_sid", 
				     &info->res_group_dom_sid, ps, depth))
			return False;
	}

	if (info->info3.ptr_res_groups) {

		if (!(info->info3.user_flgs & LOGON_RESOURCE_GROUPS)) {
			DEBUG(0,("user_flgs attribute does not have LOGON_RESOURCE_GROUPS\n"));
			/* return False; */
		}

		if (!pac_io_group_membership_array("res group membership",
						   &info->res_groups,
						   info->info3.res_group_count,
						   ps, depth))
			return False;
	}

	return True;
}



static BOOL pac_io_pac_signature_data(const char *desc, 
				      PAC_SIGNATURE_DATA *data, uint32 length,
				      prs_struct *ps, int depth)
{
	uint32 siglen = length - sizeof(uint32);
	prs_debug(ps, depth, desc, "pac_io_pac_signature_data");
	depth++;
	
	if (data == NULL)
		return False;

	if (!prs_align(ps))
		return False;
	if (!prs_uint32("type", ps, depth, &data->type))
		return False;

	if (UNMARSHALLING(ps) && length) {
		data->signature.buffer = PRS_ALLOC_MEM(ps, uint8, siglen);
		if (!data->signature.buffer) {
			DEBUG(3, ("No memory available\n"));
			return False;
		}
	}

	data->signature.buf_len = siglen;

	if (!prs_uint8s(False, "signature", ps, depth, data->signature.buffer, data->signature.buf_len))
		return False;


	return True;
}

static BOOL pac_io_pac_info_hdr_ctr(const char *desc, PAC_BUFFER *hdr,
				    prs_struct *ps, int depth)
{
	if (NULL == hdr)
		return False;

	prs_debug(ps, depth, desc, "pac_io_pac_info_hdr_ctr");
	depth++;

	if (!prs_align(ps))
		return False;

	if (hdr->offset != prs_offset(ps)) {
		DEBUG(5,("offset in header(x%x) and data(x%x) do not match, correcting\n",
			 hdr->offset, prs_offset(ps)));
		prs_set_offset(ps, hdr->offset);
	}

	if (UNMARSHALLING(ps) && hdr->size > 0) {
		hdr->ctr = PRS_ALLOC_MEM(ps, PAC_INFO_CTR, 1);
		if (!hdr->ctr) {
			DEBUG(3, ("No memory available\n"));
			return False;
		}
	}

	switch(hdr->type) {
	case PAC_TYPE_LOGON_INFO:
		DEBUG(5, ("PAC_TYPE_LOGON_INFO\n"));
		if (UNMARSHALLING(ps))
			hdr->ctr->pac.logon_info = PRS_ALLOC_MEM(ps, PAC_LOGON_INFO, 1);
		if (!hdr->ctr->pac.logon_info) {
			DEBUG(3, ("No memory available\n"));
			return False;
		}
		if (!pac_io_pac_logon_info(desc, hdr->ctr->pac.logon_info,
					   ps, depth))
			return False;
		break;

	case PAC_TYPE_SERVER_CHECKSUM:
		DEBUG(5, ("PAC_TYPE_SERVER_CHECKSUM\n"));
		if (UNMARSHALLING(ps))
			hdr->ctr->pac.srv_cksum = PRS_ALLOC_MEM(ps, PAC_SIGNATURE_DATA, 1);
		if (!hdr->ctr->pac.srv_cksum) {
			DEBUG(3, ("No memory available\n"));
			return False;
		}
		if (!pac_io_pac_signature_data(desc, hdr->ctr->pac.srv_cksum,
					       hdr->size, ps, depth))
			return False;
		break;

	case PAC_TYPE_PRIVSVR_CHECKSUM:
		DEBUG(5, ("PAC_TYPE_PRIVSVR_CHECKSUM\n"));
		if (UNMARSHALLING(ps))
			hdr->ctr->pac.privsrv_cksum = PRS_ALLOC_MEM(ps, PAC_SIGNATURE_DATA, 1);
		if (!hdr->ctr->pac.privsrv_cksum) {
			DEBUG(3, ("No memory available\n"));
			return False;
		}
		if (!pac_io_pac_signature_data(desc, 
					       hdr->ctr->pac.privsrv_cksum,
					       hdr->size, ps, depth))
			return False;
		break;

	case PAC_TYPE_LOGON_NAME:
		DEBUG(5, ("PAC_TYPE_LOGON_NAME\n"));
		if (UNMARSHALLING(ps))
			hdr->ctr->pac.logon_name = PRS_ALLOC_MEM(ps, PAC_LOGON_NAME, 1);
		if (!hdr->ctr->pac.logon_name) {
			DEBUG(3, ("No memory available\n"));
			return False;
		}
		if (!pac_io_logon_name(desc, hdr->ctr->pac.logon_name,
					    ps, depth))
			return False;
		break;

	default:
		/* dont' know, so we need to skip it */
		DEBUG(3, ("unknown PAC type %d\n", hdr->type));
		prs_set_offset(ps, prs_offset(ps) + hdr->size);
	}

#if 0
	/* obscure pad */
	if (!prs_uint32("pad", ps, depth, &hdr->pad))
		return False;
#endif
	return True;
}

static BOOL pac_io_pac_info_hdr(const char *desc, PAC_BUFFER *hdr, 
				prs_struct *ps, int depth)
{
	if (NULL == hdr)
		return False;

	prs_debug(ps, depth, desc, "pac_io_pac_info_hdr");
	depth++;

	if (!prs_align(ps))
		return False;
	if (!prs_uint32("type", ps, depth, &hdr->type))
		return False;
	if (!prs_uint32("size", ps, depth, &hdr->size))
		return False;
	if (!prs_uint32("offset", ps, depth, &hdr->offset))
		return False;
	if (!prs_uint32("offsethi", ps, depth, &hdr->offsethi))
		return False;

	return True;
}

static BOOL pac_io_pac_data(const char *desc, PAC_DATA *data, 
			    prs_struct *ps, int depth)
{
	int i;

	if (NULL == data)
		return False;

	prs_debug(ps, depth, desc, "pac_io_pac_data");
	depth++;

	if (!prs_align(ps))
		return False;
	if (!prs_uint32("num_buffers", ps, depth, &data->num_buffers))
		return False;
	if (!prs_uint32("version", ps, depth, &data->version))
		return False;

	if (UNMARSHALLING(ps) && data->num_buffers > 0) {
		if ((data->pac_buffer = PRS_ALLOC_MEM(ps, PAC_BUFFER, data->num_buffers)) == NULL) {
			return False;
		}
	}

	for (i=0; i<data->num_buffers; i++) {
		if (!pac_io_pac_info_hdr(desc, &data->pac_buffer[i], ps, 
					 depth))
			return False;
	}

	for (i=0; i<data->num_buffers; i++) {
		if (!pac_io_pac_info_hdr_ctr(desc, &data->pac_buffer[i],
					     ps, depth))
			return False;
	}

	return True;
}

static NTSTATUS check_pac_checksum(TALLOC_CTX *mem_ctx, 
				   DATA_BLOB pac_data,
				   PAC_SIGNATURE_DATA *sig,
				   krb5_context context,
				   krb5_keyblock *keyblock)
{
	krb5_error_code ret;
	krb5_checksum cksum;
	krb5_keyusage usage = 0;

	smb_krb5_checksum_from_pac_sig(&cksum, sig);

#ifdef HAVE_KRB5_KU_OTHER_CKSUM /* Heimdal */
	usage = KRB5_KU_OTHER_CKSUM;
#elif defined(HAVE_KRB5_KEYUSAGE_APP_DATA_CKSUM) /* MIT */
	usage = KRB5_KEYUSAGE_APP_DATA_CKSUM;
#else
#error UNKNOWN_KRB5_KEYUSAGE
#endif

	ret = smb_krb5_verify_checksum(context, 
				       keyblock, 
				       usage, 
				       &cksum,
				       pac_data.data, 
				       pac_data.length);

	if (ret) {
		DEBUG(2,("check_pac_checksum: PAC Verification failed: %s (%d)\n", 
			error_message(ret), ret));
		return NT_STATUS_ACCESS_DENIED;
	}

	return NT_STATUS_OK;
}

static NTSTATUS parse_pac_data(TALLOC_CTX *mem_ctx, DATA_BLOB *pac_data_blob, PAC_DATA *pac_data)
{
	prs_struct ps;
	PAC_DATA *my_pac;

	if (!prs_init(&ps, pac_data_blob->length, mem_ctx, UNMARSHALL))
		return NT_STATUS_NO_MEMORY;

	if (!prs_copy_data_in(&ps, (char *)pac_data_blob->data, pac_data_blob->length))
		return NT_STATUS_INVALID_PARAMETER;

	prs_set_offset(&ps, 0);

	my_pac = TALLOC_ZERO_P(mem_ctx, PAC_DATA);
	if (!pac_io_pac_data("pac data", my_pac, &ps, 0))
		return NT_STATUS_INVALID_PARAMETER;

	prs_mem_free(&ps);

	*pac_data = *my_pac;

	return NT_STATUS_OK;
}

/* just for debugging, will be removed later - Guenther */
char *pac_group_attr_string(uint32 attr)
{
	fstring name = "";

	if (!attr)
		return NULL;

	if (attr & SE_GROUP_MANDATORY)			fstrcat(name, "SE_GROUP_MANDATORY ");
	if (attr & SE_GROUP_ENABLED_BY_DEFAULT)		fstrcat(name, "SE_GROUP_ENABLED_BY_DEFAULT ");
	if (attr & SE_GROUP_ENABLED)			fstrcat(name, "SE_GROUP_ENABLED ");
	if (attr & SE_GROUP_OWNER)			fstrcat(name, "SE_GROUP_OWNER ");
	if (attr & SE_GROUP_USE_FOR_DENY_ONLY)		fstrcat(name, "SE_GROUP_USE_FOR_DENY_ONLY ");
	if (attr & SE_GROUP_LOGON_ID)			fstrcat(name, "SE_GROUP_LOGON_ID ");
	if (attr & SE_GROUP_RESOURCE)			fstrcat(name, "SE_GROUP_RESOURCE ");

	return SMB_STRDUP(name);
}

/* just for debugging, will be removed later - Guenther */
static void dump_pac_logon_info(PAC_LOGON_INFO *logon_info) {

	DOM_SID dom_sid, res_group_dom_sid;
	int i;
	char *attr_string;
	uint32 user_flgs = logon_info->info3.user_flgs;

	if (logon_info->info3.ptr_res_group_dom_sid) {
		sid_copy(&res_group_dom_sid, &logon_info->res_group_dom_sid.sid);
	}
	sid_copy(&dom_sid, &logon_info->info3.dom_sid.sid);
	
	DEBUG(10,("The PAC:\n"));
	
	DEBUGADD(10,("\tUser Flags: 0x%x (%d)\n", user_flgs, user_flgs));
	if (user_flgs & LOGON_EXTRA_SIDS)
		DEBUGADD(10,("\tUser Flags: LOGON_EXTRA_SIDS 0x%x (%d)\n", LOGON_EXTRA_SIDS, LOGON_EXTRA_SIDS));
	if (user_flgs & LOGON_RESOURCE_GROUPS)
		DEBUGADD(10,("\tUser Flags: LOGON_RESOURCE_GROUPS 0x%x (%d)\n", LOGON_RESOURCE_GROUPS, LOGON_RESOURCE_GROUPS));
	DEBUGADD(10,("\tUser SID: %s-%d\n", sid_string_static(&dom_sid), logon_info->info3.user_rid));
	DEBUGADD(10,("\tGroup SID: %s-%d\n", sid_string_static(&dom_sid), logon_info->info3.group_rid));

	DEBUGADD(10,("\tGroup Membership (Global and Universal Groups of own domain):\n"));
	for (i = 0; i < logon_info->info3.num_groups; i++) {
		attr_string = pac_group_attr_string(logon_info->info3.gids[i].attr);
		DEBUGADD(10,("\t\t%d: sid: %s-%d\n\t\t   attr: 0x%x == %s\n", 
			i, sid_string_static(&dom_sid), 
			logon_info->info3.gids[i].g_rid,
			logon_info->info3.gids[i].attr,
			attr_string));
		SAFE_FREE(attr_string);
	}

	DEBUGADD(10,("\tGroup Membership (Domain Local Groups and Groups from Trusted Domains):\n"));
	for (i = 0; i < logon_info->info3.num_other_sids; i++) {
		attr_string = pac_group_attr_string(logon_info->info3.other_sids_attrib[i]);
		DEBUGADD(10,("\t\t%d: sid: %s\n\t\t   attr: 0x%x == %s\n", 
			i, sid_string_static(&logon_info->info3.other_sids[i].sid), 
			logon_info->info3.other_sids_attrib[i],
			attr_string));
		SAFE_FREE(attr_string);
	}

	DEBUGADD(10,("\tGroup Membership (Ressource Groups (SID History ?)):\n"));
	for (i = 0; i < logon_info->info3.res_group_count; i++) {
		attr_string = pac_group_attr_string(logon_info->res_groups.group_membership[i].attrs);
		DEBUGADD(10,("\t\t%d: sid: %s-%d\n\t\t   attr: 0x%x == %s\n", 
			i, sid_string_static(&res_group_dom_sid),
			logon_info->res_groups.group_membership[i].rid,
			logon_info->res_groups.group_membership[i].attrs,
			attr_string));
		SAFE_FREE(attr_string);
	}
}

 NTSTATUS decode_pac_data(TALLOC_CTX *mem_ctx,
			 DATA_BLOB *pac_data_blob,
			 krb5_context context, 
			 krb5_keyblock *service_keyblock,
			 krb5_const_principal client_principal,
			 time_t tgs_authtime,
			 PAC_DATA **pac_data)
			 
{
	DATA_BLOB modified_pac_blob;
	PAC_DATA *my_pac;
	NTSTATUS nt_status;
	krb5_error_code ret;
	PAC_SIGNATURE_DATA *srv_sig = NULL;
	PAC_SIGNATURE_DATA *kdc_sig = NULL;
	PAC_LOGON_NAME *logon_name = NULL;
	PAC_LOGON_INFO *logon_info = NULL;
	krb5_principal client_principal_pac = NULL;
	NTTIME tgs_authtime_nttime;
	int i, srv_sig_pos = 0, kdc_sig_pos = 0;
	fstring username;

	*pac_data = NULL;

	my_pac = talloc(mem_ctx, PAC_DATA);
	if (!my_pac) {
		return NT_STATUS_NO_MEMORY;
	}

	nt_status = parse_pac_data(mem_ctx, pac_data_blob, my_pac);
	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(0,("decode_pac_data: failed to parse PAC\n"));
		return nt_status;
	}

	modified_pac_blob = data_blob_talloc(mem_ctx, pac_data_blob->data, pac_data_blob->length);

	if (my_pac->num_buffers < 4) {
		nt_status = NT_STATUS_INVALID_PARAMETER;
		goto out;
	}

	/* store signatures */
	for (i=0; i < my_pac->num_buffers; i++) {
	
		switch (my_pac->pac_buffer[i].type) {
		
			case PAC_TYPE_SERVER_CHECKSUM:
				if (!my_pac->pac_buffer[i].ctr->pac.srv_cksum) {
					break;
				}
				
				srv_sig = my_pac->pac_buffer[i].ctr->pac.srv_cksum;
				
				/* get position of signature buffer */
				srv_sig_pos = my_pac->pac_buffer[i].offset;
				srv_sig_pos += sizeof(uint32);
				
				break;
				
			case PAC_TYPE_PRIVSVR_CHECKSUM:
				if (!my_pac->pac_buffer[i].ctr->pac.privsrv_cksum) {
					break;
				}

				kdc_sig = my_pac->pac_buffer[i].ctr->pac.privsrv_cksum;
				
				/* get position of signature buffer */
				kdc_sig_pos = my_pac->pac_buffer[i].offset;
				kdc_sig_pos += sizeof(uint32);
				
				break;
				
			case PAC_TYPE_LOGON_NAME:
				if (!my_pac->pac_buffer[i].ctr->pac.logon_name) {
					break;
				}

				logon_name = my_pac->pac_buffer[i].ctr->pac.logon_name;
				break;

			case PAC_TYPE_LOGON_INFO:
				if (!my_pac->pac_buffer[i].ctr->pac.logon_info) {
					break;
				}

				logon_info = my_pac->pac_buffer[i].ctr->pac.logon_info;
				break;
			}

	}

	if (!srv_sig || !kdc_sig || !logon_name || !logon_info) {
		nt_status = NT_STATUS_INVALID_PARAMETER;
		goto out;
	}

	/* zero PAC_SIGNATURE_DATA signature buffer */
	memset(&modified_pac_blob.data[srv_sig_pos], '\0', srv_sig->signature.buf_len);
	memset(&modified_pac_blob.data[kdc_sig_pos], '\0', kdc_sig->signature.buf_len);

	/* check server signature */
	nt_status = check_pac_checksum(mem_ctx, modified_pac_blob, srv_sig, context, service_keyblock);
	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(0,("decode_pac_data: failed to verify PAC server signature\n"));
		goto out;
	}

	/* Convert to NT time, so as not to loose accuracy in comparison */
	unix_to_nt_time(&tgs_authtime_nttime, tgs_authtime);

	if (!nt_time_equals(&tgs_authtime_nttime, &logon_name->logon_time)) {
	
		DEBUG(2,("decode_pac_data: Logon time mismatch between ticket and PAC!\n"));
		DEBUGADD(2, ("decode_pac_data: PAC: %s\n", 
			http_timestring(nt_time_to_unix(&logon_name->logon_time))));
		DEBUGADD(2, ("decode_pac_data: Ticket: %s\n", 
			http_timestring(nt_time_to_unix(&tgs_authtime_nttime))));
		
		nt_status = NT_STATUS_ACCESS_DENIED;
		goto out;
	}

	if (!logon_name->len) {
		DEBUG(2,("decode_pac_data: No Logon Name available\n"));
		nt_status = NT_STATUS_INVALID_PARAMETER;
		goto out;
	}
	rpcstr_pull(username, logon_name->username, sizeof(username), logon_name->len, 0);

	ret = smb_krb5_parse_name_norealm(context, username, &client_principal_pac);
	if (ret) {
		DEBUG(2,("decode_pac_data: Could not parse name from incoming PAC: [%s]: %s\n", 
			username, error_message(ret)));
		nt_status = NT_STATUS_INVALID_PARAMETER;
		goto out;
	}

	if (!smb_krb5_principal_compare_any_realm(context, client_principal, client_principal_pac)) {
		DEBUG(2,("decode_pac_data: Name in PAC [%s] does not match principal name in ticket\n", 
			username));
		nt_status = NT_STATUS_ACCESS_DENIED;
		goto out;
	}

	DEBUG(10,("Successfully validated Kerberos PAC\n"));

	dump_pac_logon_info(logon_info);

	*pac_data = my_pac;

	nt_status = NT_STATUS_OK;

out:
	if (client_principal_pac) {
		krb5_free_principal(context, client_principal_pac);
	}

	return nt_status;
}

 PAC_LOGON_INFO *get_logon_info_from_pac(PAC_DATA *pac_data) 
{
	PAC_LOGON_INFO *logon_info = NULL;
	int i;
	
	for (i=0; i < pac_data->num_buffers; i++) {

		if (pac_data->pac_buffer[i].type != PAC_TYPE_LOGON_INFO)
			continue;

		logon_info = pac_data->pac_buffer[i].ctr->pac.logon_info;
		break;
	}
	return logon_info;
}

#endif
